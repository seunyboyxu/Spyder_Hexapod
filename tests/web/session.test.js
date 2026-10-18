import test from "node:test";
import assert from "node:assert/strict";
import { Session } from "../../web/session.js";
import { DemoTransport } from "../../web/demo.js";
test("production acknowledgement deadline exceeds firmware lease", () => {
  assert(new Session().options.timeoutMs > 1500);
});
test("fault aborts a started output and Disconnect cannot hang indefinitely", async () => {
  const d = new DemoTransport();
  let aborted = false;
  const close = d.close.bind(d);
  d.abort = async () => {
    aborted = true;
    await close();
  };
  const s = new Session({
    pollMs: 0,
    heartbeatMs: 0,
    timeoutMs: 20,
    cleanupMs: 30,
  });
  await s.connect(d);
  d.write = () => new Promise(() => {});
  const pending = s.command("ARM", [0, 1500]);
  await assert.rejects(pending, /timeout/i);
  assert(aborted);
  d.close = () => new Promise(() => {});
  await assert.rejects(s.disconnect(), /cleanup timed out/);
});
test("demo session starts detached; calibration, pose and STOP round trip", async () => {
  const s = new Session({ pollMs: 0, heartbeatMs: 0, timeoutMs: 100 });
  await s.connect(new DemoTransport());
  assert(s.ready && s.state.joints.every((j) => !j.attached));
  for (let i = 0; i < 12; i++)
    await s.command("CONFIG", [i, 1400, 1600, 1500, 1, 1]);
  await s.command("RESUME");
  for (let i = 0; i < 12; i++) await s.command("ARM", [i, 1500]);
  await s.command("CAPTURE");
  assert(s.state.standingValid);
  await s.command("MOVE", [0, 1550, 50]);
  await s.stop();
  assert(s.state.stopped);
  await assert.rejects(s.command("MOVE", [0, 1500, 50]), /STATE/);
  await s.command("PING");
  assert(s.state.stopped);
  await s.command("RELEASE");
  assert(s.state.joints.every((j) => !j.attached));
  await s.disconnect();
  assert(!s.ready);
});
test("unexpected boot invalidates pending commands and requires reconnect", async () => {
  const d = new DemoTransport();
  const s = new Session({ pollMs: 0, heartbeatMs: 0, timeoutMs: 50 });
  await s.connect(d);
  d.drop = true;
  const promise = s.command("ARM", [0, 1500]);
  d.emit("BOOT SPYDER 2");
  await assert.rejects(promise);
  assert(!s.ready);
  await assert.rejects(s.command("ARM", [0, 1500]));
  await s.disconnect();
});
test("timeout invalidates authority; stale ACK cannot reenable", async () => {
  const d = new DemoTransport();
  const s = new Session({ pollMs: 0, heartbeatMs: 0, timeoutMs: 20 });
  await s.connect(d);
  d.drop = true;
  await assert.rejects(s.command("ARM", [0, 1500]), /timeout/i);
  assert(!s.ready);
  d.emit("ACK 1");
  assert(!s.ready);
  await s.disconnect();
});
test("STOP rejects queued moves and is sent before further movement", async () => {
  const d = new DemoTransport();
  const s = new Session({ pollMs: 0, heartbeatMs: 0, timeoutMs: 100 });
  await s.connect(d);
  await s.command("RESUME");
  await s.command("ARM", [0, 1500]);
  const a = s.command("MOVE", [0, 1550, 50]);
  const b = s.command("MOVE", [0, 1560, 50]);
  const stopped = s.stop();
  const results = await Promise.allSettled([a, b, stopped]);
  assert.equal(results[1].status, "rejected");
  assert.equal(results[2].status, "fulfilled");
  assert(s.state.stopped);
  await s.disconnect();
});
test("fault before write microtask cancels unsent ARM", async () => {
  const d = new DemoTransport();
  const s = new Session({ pollMs: 0, heartbeatMs: 0, timeoutMs: 100 });
  await s.connect(d);
  await s.command("RESUME");
  let armsWritten = 0;
  const originalWrite = d.write.bind(d);
  d.write = (line) => {
    if (line.includes(" ARM ")) armsWritten++;
    return originalWrite(line);
  };
  const pending = s.command("ARM", [0, 1500]);
  d.emit("BOOT SPYDER 2");
  await assert.rejects(pending);
  await new Promise((resolve) => setTimeout(resolve, 0));
  assert.equal(d.joints[0].attached, 0);
  assert.equal(armsWritten, 0);
  await s.disconnect();
});
test("STOP and RELEASE bypass a pending readback and stale snapshots cannot undo their state", async (t) => {
  for (const verb of ["STOP", "RELEASE"]) {
    const d = new DemoTransport(),
      s = new Session({ pollMs: 0, heartbeatMs: 0, timeoutMs: 500 });
    let release;
    t.after(async () => {
      release?.();
      await s.disconnect();
    });
    await s.connect(d);
    await s.command("RESUME");
    await s.command("ARM", [0, 1500]);
    const original = d.write.bind(d),
      emit = d.emit.bind(d),
      writes = [];
    let block = true,
      started;
    const blocked = new Promise((resolve) => (started = resolve));
    d.write = (line) => {
      writes.push(line);
      if (block && line.includes(" STATUS")) {
        block = false;
        const lines = [];
        d.emit = (line) => lines.push(line);
        const result = original(line);
        d.emit = emit;
        release = () => {
          for (const line of lines) emit(line);
          release = null;
        };
        started();
        return result;
      }
      return original(line);
    };
    const moving = s.command("MOVE", [0, 1550, 50]);
    const outcome = moving.catch((e) => e);
    await blocked;
    const control = verb === "STOP" ? s.stop() : s.release();
    control.catch(() => {});
    await new Promise((resolve) => setImmediate(resolve));
    assert(
      writes.some((line) => line.includes(` ${verb}`)),
      `${verb} waited behind a readback`,
    );
    await control;
    assert(verb === "STOP" ? s.state.stopped : !s.state.joints[0].attached);
    release();
    await outcome;
    assert(
      verb === "STOP" ? s.state.stopped : !s.state.joints[0].attached,
      "stale readback overwrote safety state",
    );
    await s.disconnect();
  }
});
test("sustained foreground commands service heartbeat without clearing hold", async (t) => {
  let now = 0;
  const d = new DemoTransport(),
    writes = [];
  const original = d.write.bind(d);
  d.write = (line) => {
    writes.push({ now, line });
    return original(line);
  };
  const s = new Session({
    clock: () => now,
    pollMs: 0,
    heartbeatMs: 250,
    timeoutMs: 100,
  });
  t.after(() => s.disconnect());
  await s.connect(d);
  await s.command("RESUME");
  await s.command("ARM", [0, 1500]);
  for (let i = 0; i < 20; i++) {
    now += 100;
    await s.command("MOVE", [0, 1505, 50]);
  }
  const pings = writes.filter((w) => w.line.includes(" PING"));
  assert(pings.length >= 6, "continuous traffic starved PING");
  let previous = 0;
  for (const p of pings) {
    assert(p.now - previous <= 300);
    previous = p.now;
  }
  await s.stop();
  now += 300;
  await s.command("PING");
  assert(s.state.stopped);
});

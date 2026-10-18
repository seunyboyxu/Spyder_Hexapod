import test from "node:test";
import assert from "node:assert/strict";
import { spawn, execFileSync } from "node:child_process";
import { mkdtempSync, rmSync, readdirSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { createInterface } from "node:readline";
import { Session, SerialTransport } from "../../web/session.js";
import { parseLine, Snapshot } from "../../web/protocol.js";
test("a started ARM arriving after browser timeout is rejected by real firmware lease", async (t) => {
  const folder = mkdtempSync(join(tmpdir(), "spyder-stalled-"));
  t.after(() => rmSync(folder, { recursive: true, force: true }));
  const core = "firmware/spyder_uno",
    binary = join(folder, "firmware");
  execFileSync("g++", [
    "-std=c++11",
    "-I",
    core,
    "tests/native/firmware_sim.cpp",
    ...readdirSync(core)
      .filter((p) => p.endsWith(".cpp"))
      .map((p) => `${core}/${p}`),
    "-o",
    binary,
  ]);
  const child = spawn(binary);
  t.after(() => child.kill());
  const lines = createInterface({ input: child.stdout });
  t.after(() => lines.close());
  let release;
  const transport = {
    async open(onLine) {
      lines.on("line", onLine);
    },
    write(line) {
      if (line.includes(" ARM "))
        return new Promise((resolve) => {
          release = () => child.stdin.write(line, resolve);
        });
      return new Promise((resolve, reject) =>
        child.stdin.write(line, (e) => (e ? reject(e) : resolve())),
      );
    },
    // Model bytes already below the cancellation boundary: abort cannot retract them.
    async abort() {},
    async close() {},
  };
  const session = new Session({ pollMs: 0, heartbeatMs: 0, timeoutMs: 1600 });
  t.after(() => session.disconnect());
  await session.connect(transport);
  await session.command("RESUME");
  const arm = session.command("ARM", [0, 1500]);
  await assert.rejects(arm, /timeout/i);
  assert(!session.ready);
  release();
  const snapshot = new Snapshot(65535);
  const state = await new Promise((resolve, reject) => {
    const timer = setTimeout(
      () => reject(new Error("Raw readback timeout")),
      1000,
    );
    const receive = (line) => {
      const m = parseLine(line);
      if (m.id !== 65535 || m.type === "ACK") return;
      const state = snapshot.accept(m);
      if (state) {
        clearTimeout(timer);
        lines.off("line", receive);
        resolve(state);
      }
    };
    lines.on("line", receive);
    child.stdin.write("65535 STATUS\n");
  });
  assert(state.stopped && state.joints.every((j) => !j.attached));
});
test("Web Serial adapter and JS session control the actual C++ firmware protocol", async (t) => {
  const folder = mkdtempSync(join(tmpdir(), "spyder-integration-"));
  t.after(() => rmSync(folder, { recursive: true, force: true }));
  const core = "firmware/spyder_uno",
    binary = join(folder, "firmware");
  execFileSync("g++", [
    "-std=c++11",
    "-I",
    core,
    "tests/native/firmware_sim.cpp",
    ...readdirSync(core)
      .filter((p) => p.endsWith(".cpp"))
      .map((p) => `${core}/${p}`),
    "-o",
    binary,
  ]);
  class FirmwarePort {
    async open(options) {
      assert.equal(options.baudRate, 115200);
      this.child = spawn(binary);
      this.child.on("error", (e) => {
        throw e;
      });
      this.readable = new ReadableStream({
        start: (controller) => {
          this.child.stdout.on("data", (chunk) => {
            for (const byte of chunk) controller.enqueue(Uint8Array.of(byte));
          });
        },
      });
      this.writable = new WritableStream({
        write: (bytes) =>
          new Promise((resolve, reject) =>
            this.child.stdin.write(bytes, (e) => (e ? reject(e) : resolve())),
          ),
      });
    }
    async close() {
      this.child.kill();
    }
  }
  const session = new Session({ heartbeatMs: 0, pollMs: 0, timeoutMs: 500 });
  t.after(() => session.disconnect());
  const transport = new SerialTransport(new FirmwarePort());
  transport.settleMs = 0;
  await session.connect(transport);
  assert(
    session.ready &&
      session.state.joints.every((j, i) => !j.attached && j.pin === i + 2),
  );
  for (let i = 0; i < 12; i++)
    await session.command("CONFIG", [i, 1400, 1600, 1500, i % 2 ? -1 : 1, 1]);
  await session.command("POSE", Array(12).fill(1510));
  await session.command("SAVE");
  await session.command("CONFIG", [0, 1400, 1600, 1500, 1, 0]);
  assert(!session.state.standingValid);
  await session.command("LOAD");
  assert(session.state.standingValid && !session.state.joints[0].attached);
  await session.command("RESUME");
  for (let i = 0; i < 12; i++) await session.command("ARM", [i, 1500]);
  await session.command("STAND", [50]);
  assert(session.state.joints.every((j) => j.target === 1510));
  await session.stop();
  assert(session.state.stopped);
  await session.command("PING");
  assert(session.state.stopped);
  await session.command("RELEASE");
  await session.command("RESUME");
  assert(session.state.joints.every((j) => !j.attached));
});

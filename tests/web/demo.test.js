import test from "node:test";
import assert from "node:assert/strict";
import { DemoTransport } from "../../web/demo.js";

async function setup(t) {
  t.mock.timers.enable({ apis: ["Date", "setInterval"], now: 0 });
  const demo = new DemoTransport();
  const replies = [];
  await demo.open((line) => replies.push(line));
  t.after(() => demo.close());
  let id = 0;
  const send = async (command) => {
    await demo.write(`${++id} ${command}\n`);
    return replies.at(-1);
  };
  const advance = (ms) => {
    for (let elapsed = 0; elapsed < ms; elapsed += 20)
      t.mock.timers.tick(Math.min(20, ms - elapsed));
  };
  return { demo, send, advance, replies };
}

test("demo identifies as the corrected firmware protocol revision", async (t) => {
  const { send, replies } = await setup(t);
  await send("HELLO");
  assert.equal(replies[0], "SPYDER 2");
});

for (const speed of [10, 50, 100]) {
  test(`MOVE at ${speed} µs/s uses fractional 20 ms steps`, async (t) => {
    const { demo, send, advance } = await setup(t);
    await send("RESUME");
    await send("ARM 0 1500");
    await send(`MOVE 0 1600 ${speed}`);
    advance(400);
    assert.equal(demo.joints[0].pulse, 1500 + speed * 0.4);
  });
}

test("STAND uses the requested speed and never overshoots its target", async (t) => {
  const { demo, send, advance } = await setup(t);
  for (let i = 0; i < 12; i++) await send(`CONFIG ${i} 1400 1600 1500 1 1`);
  await send(`POSE ${Array(12).fill(1507).join(" ")}`);
  await send("RESUME");
  for (let i = 0; i < 12; i++) await send(`ARM ${i} 1500`);
  await send("STAND 10");
  advance(400);
  assert(demo.joints.every((joint) => joint.pulse === 1504));
  advance(400);
  assert(demo.joints.every((joint) => joint.pulse === 1507));
});

test("a late timer callback makes at most one 20 ms movement step", async (t) => {
  const { demo, send } = await setup(t);
  await send("RESUME");
  await send("ARM 0 1500");
  await send("MOVE 0 1600 100");
  t.mock.timers.tick(1000);
  assert.equal(demo.joints[0].pulse, 1502);
});

test("arming does not renew the lease, and detached operation still expires held", async (t) => {
  const { demo, send, advance } = await setup(t);
  assert.equal(demo.stopped, true);
  await send("RESUME");
  advance(1400);
  await send("ARM 0 1500");
  advance(100);
  assert.equal(demo.stopped, true);
  await send("RELEASE");
  await send("RESUME");
  advance(1500);
  assert.equal(demo.stopped, true);
  await send("PING");
  assert.equal(demo.stopped, true);
});

test("ARM checks an expired lease before the next scheduled tick", async (t) => {
  const { demo, send } = await setup(t);
  await send("RESUME");
  t.mock.timers.setTime(1500);
  assert.match(await send("ARM 0 1500"), /ERR \d+ STATE/);
  assert.equal(demo.joints[0].attached, 0);
});

test("compatible target updates retain fractional progress; reversal cancels it", async (t) => {
  const { demo, send, advance } = await setup(t);
  await send("RESUME");
  await send("ARM 0 1500");
  for (let i = 0; i < 5; i++) {
    await send(`MOVE 0 ${1550 + i} 10`);
    advance(80);
  }
  assert.equal(demo.joints[0].pulse, 1504);
  advance(80);
  await send("MOVE 0 1450 10");
  advance(20);
  assert.equal(demo.joints[0].pulse, 1504);
  advance(80);
  assert.equal(demo.joints[0].pulse, 1503);
});

test("same-direction speed changes preserve fractions; cancelling a target clears them", async (t) => {
  const { demo, send, advance } = await setup(t);
  await send("RESUME");
  await send("ARM 0 1500");
  await send("MOVE 0 1550 10");
  advance(80);
  await send("MOVE 0 1560 20");
  advance(20);
  assert.equal(demo.joints[0].pulse, 1501);
  await send("MOVE 0 1501 10");
  await send("MOVE 0 1560 10");
  advance(80);
  assert.equal(demo.joints[0].pulse, 1501);
  advance(20);
  assert.equal(demo.joints[0].pulse, 1502);
});

test("STOP and RELEASE cancel fractional progress; ARM resets the tick origin", async (t) => {
  const { demo, send, advance } = await setup(t);
  await send("RESUME");
  await send("ARM 0 1500");
  await send("MOVE 0 1550 10");
  advance(80);
  await send("STOP");
  await send("RESUME");
  await send("MOVE 0 1550 10");
  advance(20);
  assert.equal(demo.joints[0].pulse, 1500);
  await send("RELEASE");
  t.mock.timers.tick(10);
  await send("ARM 0 1500");
  await send("MOVE 0 1550 100");
  t.mock.timers.tick(10);
  assert.equal(demo.joints[0].pulse, 1500);
  t.mock.timers.tick(20);
  assert.equal(demo.joints[0].pulse, 1502);
});

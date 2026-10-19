import test from "node:test";
import assert from "node:assert/strict";
import { DemoTransport } from "../../web/demo.js";
import { BenchMinimumPulse, BenchMaximumPulse } from "../../web/protocol.js";

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
  assert.equal(replies[0], "SPYDER 3");
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

test("CENTER requires Resume and attaches only the selected unloaded servo at nominal 90 degrees", async (t) => {
  const { demo, send, replies } = await setup(t);
  await send("CONFIG 4 1700 1800 1750 -1 1");
  demo.pose = true;
  assert.match(await send("CENTER 4"), /ERR \d+ STATE/);
  await send("RESUME");
  assert.match(await send("CENTER 4"), /ACK/);
  assert.equal(demo.benchJoint, 4);
  assert.equal(demo.sweepPhase, 0);
  assert.equal(demo.pose, false);
  assert.equal(demo.joints[4].calibrated, 0);
  assert.deepEqual(demo.joints[4], {
    pin: 6,
    min: 1700,
    max: 1800,
    initial: 1750,
    direction: -1,
    calibrated: 0,
    attached: 1,
    pulse: 1500,
    target: 1500,
    standing: 1500,
    speed: 100,
    remainder: 0,
  });
  assert.equal(demo.joints.filter((j) => j.attached).length, 1);
  await send("STATUS");
  assert(replies.some((line) => /^BEGIN \d+ 0 0 4 0$/.test(line)));
  for (const command of [
    "ARM 0 1500",
    "MOVE 4 1750 50",
    "CENTER 3",
    "CONFIG 0 1400 1600 1500 1 1",
    "SAVE",
    "LOAD",
    "CAPTURE",
    "POSE " + Array(12).fill(1500).join(" "),
    "STAND 50",
  ])
    assert.match(await send(command), /ERR \d+ STATE/, command);
  await send("RELEASE");
  assert.equal(demo.benchJoint, -1);
  assert.equal(demo.sweepPhase, 0);
  assert(demo.joints.every((j) => !j.attached));
  assert.match(await send("CENTER 3"), /ACK/);
});

test("CENTER refuses an ordinary armed output and expired lease", async (t) => {
  const { demo, send } = await setup(t);
  await send("RESUME");
  await send("ARM 0 1500");
  assert.match(await send("CENTER 0"), /ERR \d+ STATE/);
  await send("RELEASE");
  t.mock.timers.setTime(1500);
  assert.match(await send("CENTER 1"), /ERR \d+ STATE/);
  assert(demo.joints.every((j) => !j.attached));
});

test("SWEEP makes one bounded pass at fractional speed without changing leg configuration", async (t) => {
  const { demo, send, advance } = await setup(t);
  assert.match(await send("SWEEP 0 1499 1501 10"), /ERR \d+ STATE/);
  await send("RESUME");
  await send("CENTER 0");
  for (const command of [
    `SWEEP 0 ${BenchMinimumPulse - 1} 1501 10`,
    `SWEEP 0 1499 ${BenchMaximumPulse + 1} 10`,
    "SWEEP 0 1500 1600 10",
    "SWEEP 0 1400 1500 10",
    "SWEEP 0 1400 1600 9",
    "SWEEP 0 1400 1600 101",
  ])
    assert.match(await send(command), /ERR \d+ RANGE/, command);
  assert.match(await send("SWEEP 1 1499 1501 10"), /ERR \d+ STATE/);
  await send("SWEEP 0 1499 1501 10");
  assert.equal(demo.sweepPhase, 1);
  advance(80);
  assert.equal(demo.joints[0].pulse, 1500);
  advance(20);
  assert.equal(demo.joints[0].pulse, 1499);
  assert.equal(demo.sweepPhase, 2);
  advance(200);
  assert.equal(demo.joints[0].pulse, 1501);
  assert.equal(demo.sweepPhase, 3);
  advance(100);
  assert.equal(demo.joints[0].pulse, 1500);
  assert.equal(demo.sweepPhase, 0);
  advance(100);
  assert.equal(demo.joints[0].pulse, 1500);
  assert.equal(demo.joints[0].min, 1400);
  assert.equal(demo.joints[0].max, 1600);
  assert(demo.joints.slice(1).every((j) => !j.attached && j.pulse === 1500));
});

test("a full 1000–2000 sweep stays exclusive and returns to 1500 without repeating", async (t) => {
  const { demo, send, advance } = await setup(t);
  await send("RESUME");
  await send("CENTER 11");
  await send("SWEEP 11 1000 2000 100");
  for (let seconds = 0; seconds < 20; seconds++) {
    await send("PING");
    advance(1000);
    assert(demo.joints[11].pulse >= 1000 && demo.joints[11].pulse <= 2000);
    if (seconds === 4) assert.equal(demo.joints[11].pulse, 1000);
    if (seconds === 14) assert.equal(demo.joints[11].pulse, 2000);
  }
  assert.equal(demo.joints[11].pulse, 1500);
  assert.equal(demo.sweepPhase, 0);
  assert(demo.joints.slice(0, 11).every((j) => !j.attached));
});

test("STOP cancels a sweep, Resume does not restart, and CENTER slowly recenters before a new sweep", async (t) => {
  const { demo, send, advance } = await setup(t);
  await send("RESUME");
  await send("CENTER 2");
  await send("SWEEP 2 1400 1600 50");
  advance(400);
  assert.equal(demo.joints[2].pulse, 1480);
  await send("STOP");
  assert.equal(demo.sweepPhase, 0);
  assert.equal(demo.benchJoint, 2);
  assert.equal(demo.joints[2].target, 1480);
  assert.match(await send("CENTER 2"), /ERR \d+ STATE/);
  await send("RESUME");
  advance(200);
  assert.equal(demo.joints[2].pulse, 1480);
  assert.match(await send("SWEEP 2 1400 1600 50"), /ERR \d+ STATE/);
  await send("CENTER 2");
  assert.equal(demo.joints[2].pulse, 1480);
  assert.equal(demo.joints[2].target, 1500);
  assert.equal(demo.sweepPhase, 0);
  assert.match(await send("SWEEP 2 1400 1600 50"), /ERR \d+ STATE/);
  advance(100);
  assert.equal(demo.joints[2].pulse, 1490);
  advance(100);
  assert.equal(demo.joints[2].pulse, 1500);
  assert.match(await send("SWEEP 2 1400 1600 50"), /ACK/);
  advance(100);
  await send("CENTER 2");
  assert.equal(demo.sweepPhase, 0);
  assert.equal(demo.joints[2].target, 1500);
});

test("lease expiry cancels sweep and a delayed tick cannot make a catch-up jump", async (t) => {
  const { demo, send, advance } = await setup(t);
  await send("RESUME");
  await send("CENTER 5");
  await send("SWEEP 5 1000 2000 100");
  t.mock.timers.tick(1000);
  assert.equal(demo.joints[5].pulse, 1498);
  advance(500);
  assert.equal(demo.stopped, true);
  assert.equal(demo.sweepPhase, 0);
  assert.equal(demo.joints[5].target, demo.joints[5].pulse);
  const held = demo.joints[5].pulse;
  await send("PING");
  await send("RESUME");
  advance(100);
  assert.equal(demo.joints[5].pulse, held);
  assert.equal(demo.benchJoint, 5);
  await send("RELEASE");
  assert.equal(demo.benchJoint, -1);
});

test("CENTER always resets fractional progress and uses the fixed 100 µs/s centering rate", async (t) => {
  const { demo, send, advance } = await setup(t);
  await send("RESUME");
  await send("CENTER 0");
  assert.equal(demo.joints[0].speed, 100);
  await send("SWEEP 0 1400 1600 10");
  advance(180);
  assert.equal(demo.joints[0].pulse, 1499);
  await send("CENTER 0");
  assert.equal(demo.joints[0].remainder, 0);
  assert.equal(demo.joints[0].speed, 100);
  advance(20);
  assert.equal(demo.joints[0].pulse, 1500);
});

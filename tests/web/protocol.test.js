import test from "node:test";
import assert from "node:assert/strict";
import {
  encodeCommand,
  parseLine,
  Snapshot,
  validateProfile,
} from "../../web/protocol.js";
const joint = (i) => ({
  index: i,
  pin: i + 2,
  minimum: 1400,
  maximum: 1600,
  initial: 1500,
  direction: 1,
  calibrated: false,
  attached: false,
  commanded: 1500,
  target: 1500,
  standing: 1500,
});
export const state = () => ({
  stopped: false,
  benchJoint: -1,
  sweepPhase: 0,
  standingValid: false,
  joints: Array.from({ length: 12 }, (_, i) => joint(i)),
});
test("validates commands before transmission", () => {
  assert.equal(encodeCommand(1, "ARM", [0, 1500]), "1 ARM 0 1500\n");
  for (const [v, a] of [
    ["ARM", [-1, 1500]],
    ["MOVE", [0, 1500, 101]],
    ["CONFIG", [0, 999, 1600, 1500, 1, 1]],
    ["STOP", [1]],
    ["POSE", Array(12).fill(999)],
  ])
    assert.throws(() => encodeCommand(2, v, a));
  assert.throws(() => encodeCommand(0, "STOP", []));
  assert.throws(() => encodeCommand(1, "BOGUS", []));
});
test("requires revision 3 firmware and accepts only the supported last-servo A0 alternative", () => {
  assert.equal(parseLine("SPYDER 3").type, "SPYDER");
  assert.throws(() => parseLine("SPYDER 1"), /firmware|Unsupported/i);
  assert.throws(() => parseLine("SPYDER 2"), /firmware|Unsupported/i);
  assert.equal(
    parseLine("JOINT 5 11 14 1400 1600 1500 1 0 0 1500 1500 1500").joint.pin,
    14,
  );
  assert.throws(() =>
    parseLine("JOINT 5 10 14 1400 1600 1500 1 0 0 1500 1500 1500"),
  );
});
test("accepts a complete matching snapshot only", () => {
  const s = new Snapshot(5);
  s.accept(parseLine("BEGIN 5 0 0 -1 0"));
  for (let i = 0; i < 12; i++)
    s.accept(
      parseLine(`JOINT 5 ${i} ${i + 2} 1400 1600 1500 1 0 0 1500 1500 1500`),
    );
  assert.deepEqual(s.accept(parseLine("END 5")), state());
  const short = new Snapshot(6);
  short.accept(parseLine("BEGIN 6 0 0 -1 0"));
  assert.throws(() => short.accept(parseLine("END 6")));
  assert.throws(() =>
    parseLine("JOINT 5 0 99 1400 1600 1500 1 0 0 1500 1500 1500"),
  );
  assert.throws(() => parseLine("BEGIN 5 2 0 -1 0"));
});
test("rejects duplicate rows and mismatched IDs", () => {
  const s = new Snapshot(2);
  s.accept(parseLine("BEGIN 2 0 0 -1 0"));
  const row = parseLine("JOINT 2 0 2 1400 1600 1500 1 0 0 1500 1500 1500");
  s.accept(row);
  assert.throws(() => s.accept(row));
  assert.throws(() => s.accept(parseLine("END 3")));
});
test("profile validation checks all joints, pulse limits, pose and labels", () => {
  const p = {
    version: 1,
    joints: state().joints,
    standing: null,
    labels: ["Front right", "Rear right", "Front left", "Rear left"],
    front: "USB end",
  };
  assert.equal(validateProfile(p).joints.length, 12);
  assert.throws(() => validateProfile({ ...p, joints: p.joints.slice(1) }));
  const bad = structuredClone(p);
  bad.joints[11].maximum = 2001;
  assert.throws(() => validateProfile(bad));
  assert.throws(() =>
    validateProfile({ ...p, standing: Array(12).fill(1500) }),
  );
});

test("encodes unloaded commands and bounds endpoints before transmission", () => {
  assert.equal(encodeCommand(7, "CENTER", [4]), "7 CENTER 4\n");
  assert.equal(
    encodeCommand(8, "SWEEP", [4, 1000, 2000, 50]),
    "8 SWEEP 4 1000 2000 50\n",
  );
  for (const args of [
    [12, 1000, 2000, 50],
    [4, 1500, 2000, 50],
    [4, 1000, 1500, 50],
    [4, 1000, 2000, 101],
  ])
    assert.throws(() => encodeCommand(9, "SWEEP", args));
});
test("unloaded snapshot accepts test pulses outside leg limits only for isolated selected servo", () => {
  const snapshot = (bench, extraAttached = false) => {
    const s = new Snapshot(9);
    s.accept(parseLine(`BEGIN 9 0 0 ${bench} 0`));
    for (let i = 0; i < 12; i++)
      s.accept(
        parseLine(
          `JOINT 9 ${i} ${i + 2} 1400 1600 1450 1 0 ${+(i === 4 || (extraAttached && i === 1))} ${i === 4 ? 1000 : 1500} ${i === 4 ? 1000 : 1500} 1500`,
        ),
      );
    return s.accept(parseLine("END 9"));
  };
  assert.equal(snapshot(4).joints[4].commanded, 1000);
  assert.throws(() => snapshot(-1));
  assert.throws(() => snapshot(4, true));
  assert.throws(() => parseLine("BEGIN 9 0 0 -1 1"));
  assert.throws(() => parseLine("BEGIN 9 1 0 4 1"));
});

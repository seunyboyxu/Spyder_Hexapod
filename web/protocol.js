const integer = (v, min, max) => Number.isInteger(v) && v >= min && v <= max;
const requireValue = (ok, message = "Invalid value") => {
  if (!ok) throw new Error(message);
};
export function validateJoint(j) {
  requireValue(
    j &&
      integer(j.minimum, 1000, 2000) &&
      integer(j.maximum, 1000, 2000) &&
      j.minimum < j.maximum,
    "Invalid pulse limits",
  );
  requireValue(
    integer(j.initial, j.minimum, j.maximum) &&
      [1, -1].includes(j.direction) &&
      typeof j.calibrated === "boolean",
    "Invalid calibration",
  );
  return {
    minimum: j.minimum,
    maximum: j.maximum,
    initial: j.initial,
    direction: j.direction,
    calibrated: j.calibrated,
  };
}
export function encodeCommand(id, verb, args = []) {
  requireValue(integer(id, 1, 65535), "Invalid request ID");
  const counts = {
    HELLO: 0,
    STATUS: 0,
    PING: 0,
    STOP: 0,
    RELEASE: 0,
    RESUME: 0,
    ARM: 2,
    MOVE: 3,
    CONFIG: 6,
    CAPTURE: 0,
    STAND: 1,
    POSE: 12,
    SAVE: 0,
    LOAD: 0,
  };
  requireValue(
    Object.hasOwn(counts, verb) &&
      args.length === counts[verb] &&
      args.every(Number.isInteger),
    "Invalid command",
  );
  if (["ARM", "MOVE", "CONFIG"].includes(verb))
    requireValue(integer(args[0], 0, 11), "Invalid joint");
  if (["ARM", "MOVE"].includes(verb))
    requireValue(integer(args[1], 1000, 2000), "Invalid pulse");
  if (verb === "MOVE" || verb === "STAND")
    requireValue(integer(args.at(-1), 10, 100), "Invalid speed");
  if (verb === "CONFIG") {
    requireValue([0, 1].includes(args[5]));
    validateJoint({
      minimum: args[1],
      maximum: args[2],
      initial: args[3],
      direction: args[4],
      calibrated: args[5] === 1,
    });
  }
  if (verb === "POSE") requireValue(args.every((p) => integer(p, 1000, 2000)));
  return `${id} ${verb}${args.length ? " " + args.join(" ") : ""}\n`;
}
export function parseLine(line) {
  requireValue(
    typeof line === "string" &&
      line.length <= 191 &&
      /^[\x20-\x7e]+$/.test(line),
    "Malformed response",
  );
  const parts = line.split(" "),
    type = parts.shift();
  if (type === "SPYDER" || type === "BOOT") {
    requireValue(
      (type === "SPYDER" && parts.join(" ") === "2") ||
        (type === "BOOT" && parts.join(" ") === "SPYDER 2"),
      "Unsupported firmware: upload Spyder revision 2 and reload this console",
    );
    return { type };
  }
  if (type === "ERR") {
    requireValue(
      parts.length === 2 &&
        /^\d+$/.test(parts[0]) &&
        integer(+parts[0], 0, 65535) &&
        /^[A-Z]+$/.test(parts[1]),
    );
    return { type, id: +parts[0], code: parts[1] };
  }
  const lengths = { ACK: 1, BEGIN: 3, JOINT: 12, END: 1 };
  requireValue(
    parts.length === lengths[type] && parts.every((p) => /^-?\d+$/.test(p)),
    "Malformed response",
  );
  const n = parts.map(Number),
    id = n[0];
  requireValue(integer(id, 1, 65535));
  if (type === "BEGIN") {
    requireValue([0, 1].includes(n[1]) && [0, 1].includes(n[2]));
    return { type, id, stopped: !!n[1], standingValid: !!n[2] };
  }
  if (type === "JOINT") {
    const [
      ,
      index,
      pin,
      minimum,
      maximum,
      initial,
      direction,
      calibrated,
      attached,
      commanded,
      target,
      standing,
    ] = n;
    requireValue(
      integer(index, 0, 11) &&
        (pin === index + 2 || (index === 11 && pin === 14)) &&
        [0, 1].includes(calibrated) &&
        [0, 1].includes(attached),
    );
    const joint = {
      index,
      pin,
      minimum,
      maximum,
      initial,
      direction,
      calibrated: !!calibrated,
      attached: !!attached,
      commanded,
      target,
      standing,
    };
    validateJoint(joint);
    // Detached command history may be outside newly narrowed limits; it is not an active output.
    requireValue(
      [commanded, target, standing].every((p) => integer(p, 1000, 2000)),
    );
    if (attached)
      requireValue(
        integer(commanded, minimum, maximum) &&
          integer(target, minimum, maximum),
      );
    return { type, id, joint };
  }
  return { type, id };
}
export class Snapshot {
  constructor(id) {
    this.id = id;
    this.rows = new Map();
    this.begin = null;
  }
  accept(message) {
    requireValue(message.id === this.id, "Snapshot ID mismatch");
    if (message.type === "BEGIN") {
      requireValue(!this.begin, "Duplicate BEGIN");
      this.begin = message;
      return null;
    }
    requireValue(this.begin, "Missing BEGIN");
    if (message.type === "JOINT") {
      requireValue(!this.rows.has(message.joint.index), "Duplicate joint");
      this.rows.set(message.joint.index, message.joint);
      return null;
    }
    requireValue(
      message.type === "END" && this.rows.size === 12,
      "Incomplete snapshot",
    );
    const state = {
      stopped: this.begin.stopped,
      standingValid: this.begin.standingValid,
      joints: Array.from({ length: 12 }, (_, i) => this.rows.get(i)),
    };
    if (state.standingValid)
      requireValue(
        state.joints.every(
          (j) => j.calibrated && integer(j.standing, j.minimum, j.maximum),
        ),
        "Invalid standing pose",
      );
    return state;
  }
}
export function validateProfile(value) {
  requireValue(
    value &&
      value.version === 1 &&
      Array.isArray(value.joints) &&
      value.joints.length === 12,
    "Expected a version 1, twelve-joint profile",
  );
  const joints = value.joints.map(validateJoint);
  requireValue(
    value.standing === null ||
      (Array.isArray(value.standing) &&
        value.standing.length === 12 &&
        value.standing.every(
          (p, i) =>
            joints[i].calibrated &&
            integer(p, joints[i].minimum, joints[i].maximum),
        )),
    "Invalid standing pose",
  );
  requireValue(
    Array.isArray(value.labels) &&
      value.labels.length === 4 &&
      value.labels.every((s) => typeof s === "string" && s.length <= 40) &&
      typeof value.front === "string" &&
      value.front.length <= 80,
    "Invalid physical labels",
  );
  return {
    version: 1,
    joints,
    standing: value.standing === null ? null : [...value.standing],
    labels: [...value.labels],
    front: value.front,
  };
}

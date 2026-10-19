import {
  BenchMinimumPulse,
  BenchMaximumPulse,
  BenchCenterPulse,
} from "./protocol.js?v=3.2";
// Interface demonstration only. Native tests exercise the real C++ firmware separately.
export class DemoTransport {
  constructor() {
    this.demo = true;
    this.drop = false;
    this.stopped = true;
    this.pose = false;
    this.benchJoint = -1;
    this.sweepPhase = 0;
    this.joints = Array.from({ length: 12 }, (_, i) => ({
      pin: i + 2,
      min: 1400,
      max: 1600,
      initial: 1500,
      direction: 1,
      calibrated: 0,
      attached: 0,
      pulse: 1500,
      target: 1500,
      standing: 1500,
      speed: 50,
      remainder: 0,
    }));
  }
  async open(onLine) {
    this.onLine = onLine;
    this.hold();
    this.heartbeat = this.lastTick = Date.now();
    this.clock = setInterval(() => this.tick(Date.now()), 20);
  }
  tick(now) {
    if (now - this.heartbeat >= 1500) this.hold();
    if (this.stopped || now - this.lastTick < 20) return;
    // Match the firmware's cap: delayed scheduling never causes a catch-up jump.
    this.lastTick = now;
    for (const j of this.joints) {
      if (!j.attached || j.pulse === j.target) continue;
      j.remainder += j.speed * 20;
      const step = Math.min(
        Math.floor(j.remainder / 1000),
        Math.abs(j.target - j.pulse),
      );
      j.remainder %= 1000;
      j.pulse += Math.sign(j.target - j.pulse) * step;
      if (j === this.joints[this.benchJoint] && j.pulse === j.target) {
        if (this.sweepPhase === 1) {
          this.sweepPhase = 2;
          this.move(j, this.sweepMaximum, j.speed);
        } else if (this.sweepPhase === 2) {
          this.sweepPhase = 3;
          this.move(j, BenchCenterPulse, j.speed);
        } else if (this.sweepPhase === 3) this.sweepPhase = 0;
      }
    }
  }
  move(j, target, speed) {
    const oldDirection = Math.sign(j.target - j.pulse),
      newDirection = Math.sign(target - j.pulse);
    if (!oldDirection || oldDirection !== newDirection) j.remainder = 0;
    j.target = target;
    j.speed = speed;
  }
  emit(line) {
    this.onLine(line);
  }
  hold() {
    this.stopped = true;
    this.sweepPhase = 0;
    this.joints.forEach((j) => {
      j.target = j.pulse;
      j.remainder = 0;
    });
  }
  async write(line) {
    if (this.drop) return;
    const [raw, verb, ...rest] = line.trim().split(" "),
      id = +raw,
      a = rest.map(Number),
      j = this.joints[a[0]];
    let error = null;
    const detached = this.joints.every((j) => !j.attached);
    if (verb === "CENTER" || verb === "SWEEP") {
      if (
        a.length !== (verb === "CENTER" ? 1 : 4) ||
        !a.every(Number.isInteger) ||
        !j
      ) {
        this.emit(`ERR ${id} INVALID`);
        return;
      }
      if (Date.now() - this.heartbeat >= 1500) this.hold();
    }
    switch (verb) {
      case "HELLO":
        this.emit("SPYDER 3");
        break;
      case "STATUS":
        this.emit(
          `BEGIN ${id} ${+this.stopped} ${+this.pose} ${this.benchJoint} ${this.sweepPhase}`,
        );
        this.joints.forEach((j, i) =>
          this.emit(
            `JOINT ${id} ${i} ${j.pin} ${j.min} ${j.max} ${j.initial} ${j.direction} ${j.calibrated} ${j.attached} ${j.pulse} ${j.target} ${j.standing}`,
          ),
        );
        this.emit(`END ${id}`);
        break;
      case "PING":
        this.heartbeat = Date.now();
        break;
      case "STOP":
        this.hold();
        break;
      case "RELEASE":
        this.benchJoint = -1;
        this.sweepPhase = 0;
        this.joints.forEach((j) => {
          j.attached = 0;
          j.target = j.pulse;
          j.remainder = 0;
        });
        break;
      case "RESUME":
        this.stopped = false;
        this.heartbeat = this.lastTick = Date.now();
        break;
      case "CENTER":
        if (this.stopped || (!detached && this.benchJoint !== a[0]))
          error = "STATE";
        else {
          if (this.benchJoint === -1) {
            j.attached = 1;
            j.pulse = j.target = BenchCenterPulse;
            j.remainder = 0;
            this.lastTick = Date.now();
          } else j.target = BenchCenterPulse;
          j.speed = 100;
          j.remainder = 0;
          this.benchJoint = a[0];
          this.sweepPhase = 0;
          j.calibrated = 0;
          this.pose = false;
        }
        break;
      case "SWEEP":
        if (
          this.stopped ||
          this.benchJoint !== a[0] ||
          this.sweepPhase !== 0 ||
          j.pulse !== BenchCenterPulse ||
          j.target !== BenchCenterPulse
        )
          error = "STATE";
        else if (
          a[1] < BenchMinimumPulse ||
          a[1] >= BenchCenterPulse ||
          a[2] <= BenchCenterPulse ||
          a[2] > BenchMaximumPulse ||
          a[3] < 10 ||
          a[3] > 100
        )
          error = "RANGE";
        else {
          this.sweepMaximum = a[2];
          this.sweepPhase = 1;
          this.move(j, a[1], a[3]);
        }
        break;
      case "ARM":
        if (Date.now() - this.heartbeat >= 1500) this.hold();
        if (this.stopped || this.benchJoint !== -1 || j.attached)
          error = "STATE";
        else if (a[1] < j.min || a[1] > j.max) error = "RANGE";
        else {
          j.attached = 1;
          j.pulse = j.target = a[1];
          j.remainder = 0;
          this.lastTick = Date.now();
        }
        break;
      case "MOVE":
        if (this.stopped || this.benchJoint !== -1 || !j.attached)
          error = "STATE";
        else if (a[1] < j.min || a[1] > j.max || a[2] < 10 || a[2] > 100)
          error = "RANGE";
        else this.move(j, a[1], a[2]);
        break;
      case "CONFIG":
        if (!detached) error = "STATE";
        else {
          [j.min, j.max, j.initial, j.direction, j.calibrated] = a.slice(1);
          this.pose = false;
        }
        break;
      case "CAPTURE":
        if (
          this.stopped ||
          this.benchJoint !== -1 ||
          this.joints.some(
            (j) => !j.attached || !j.calibrated || j.target !== j.pulse,
          )
        )
          error = "STATE";
        else {
          this.joints.forEach((j) => (j.standing = j.pulse));
          this.pose = true;
        }
        break;
      case "POSE":
        if (!detached) error = "STATE";
        else if (
          this.joints.some(
            (j, i) => !j.calibrated || a[i] < j.min || a[i] > j.max,
          )
        )
          error = "RANGE";
        else {
          this.joints.forEach((j, i) => (j.standing = a[i]));
          this.pose = true;
        }
        break;
      case "STAND":
        if (
          this.stopped ||
          this.benchJoint !== -1 ||
          !this.pose ||
          this.joints.some((j) => !j.attached)
        )
          error = "STATE";
        else if (a[0] < 10 || a[0] > 100) error = "RANGE";
        else
          this.joints.forEach((j) => {
            j.target = j.standing;
            j.speed = a[0];
            j.remainder = 0;
          });
        break;
      case "SAVE":
        if (!detached) error = "STATE";
        else
          this.saved = structuredClone({
            joints: this.joints,
            pose: this.pose,
          });
        break;
      case "LOAD":
        if (!detached) error = "STATE";
        else if (!this.saved) error = "STORAGE";
        else {
          this.joints = structuredClone(this.saved.joints);
          this.pose = this.saved.pose;
        }
        break;
      default:
        error = "INVALID";
    }
    this.emit(error ? `ERR ${id} ${error}` : `ACK ${id}`);
  }
  async close() {
    this.hold();
    clearInterval(this.clock);
  }
}

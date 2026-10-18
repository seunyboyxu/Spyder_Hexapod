# Spyder four-leg Uno calibration console

Status: approved design, implemented in this repository. See `docs/verification.md` for evidence and remaining physical checks.

## Goal and confirmed hardware

Build Arduino firmware and a computer interface to test and calibrate four installed legs, with three SG90 servos per leg. The controller is an Arduino Uno with an Arduino Sensor Shield V5. The user initially connected the twelve servos consecutively, three per leg, on digital connectors 0–11, and has now reported moving the plugs following the proposed D2–D13 mapping.

The first working milestone is supported individual-joint testing followed by gradual movement to a user-calibrated standing pose. Walking and the future six-leg ESP32/PCA9685 conversion are outside this milestone.

The supplied SPYDER-CODEX-HANDOVER.md is reference context. Its earlier ESP32, two-PCA9685 and eighteen-servo instructions do not replace the user's current hardware decisions. Its custom CAD files are referenced Windows paths, not supplied design files in this workspace. Its reported horn/pivot retention issue remains a physical check, not a diagnosed current assembly condition.

## Approach and alternatives

Recommended: Arduino Servo library firmware with a static HTML/JavaScript interface using Web Serial over USB. Run the interface on the computer physically connected to the Uno, using desktop Chrome or Edge at a local HTTP origin. The Uno does not serve a website, and the cloud environment does not access the user's USB device. This avoids installing a Python serial backend and permits a browser demo without motors.

Alternative: a Python serial bridge and browser interface. This supports browsers without Web Serial but adds a local service, dependency management and another communication boundary.

Alternative: Arduino Serial Monitor alone. This has fewer interface dependencies but is less convenient for twelve joint controls, calibration and pose management.

## Wiring contract

The shield's digital connectors expose the Uno pins; it is not an independently addressed servo driver. Reserve D0/RX and D1/TX for USB serial. Move servo signal connections to D2–D13 in the existing sequence:

| Wiring group | Existing connectors | Proposed connectors | Joint identifiers |
|---|---|---|---|
| First installed leg | 0, 1, 2 | 2, 3, 4 | L0/J0, L0/J1, L0/J2 |
| Second installed leg | 3, 4, 5 | 5, 6, 7 | L1/J0, L1/J1, L1/J2 |
| Third installed leg | 6, 7, 8 | 8, 9, 10 | L2/J0, L2/J1, L2/J2 |
| Fourth installed leg | 9, 10, 11 | 11, 12, 13 | L3/J0, L3/J1, L3/J2 |

These are wiring identifiers, not assumed front/rear or clockwise positions. Label the physical positions in the interface. J0–J2 preserve the user's proximal-to-distal sequence; the interface must not silently apply the upstream pitch/knee/yaw ordering.

Use a regulated external servo supply within the actual SG90 specifications, sized for the expected combined loads. Share ground with the Uno. Verify the actual shield revision's external supply terminal, rail-isolation jumper and S/V/G labels before connecting power; do not assume the shield isolates the servo rail from USB. No final current rating is inferred from the handover. The shield does not itself guarantee adequate power for twelve motors.

## Firmware behavior

- Target the Uno/ATmega328P with the Arduino framework and pinned Servo dependency. Use fixed-size buffers and arrays, with no heap-based command parsing. Keep flash and SRAM usage within Uno limits and report build usage.
- Use a transport-independent control core, exercised by native tests, and a thin Arduino adapter for serial, timers, EEPROM and servo outputs.
- Boot/reset leaves all servo outputs detached with hold latched; explicit Resume controls is required before arming. Connecting USB, reconnecting or loading settings never enables motors or runs a startup pose.
- A joint must be deliberately armed at an explicitly selected starting pulse. An unpowered SG90's actual angle is unknown; the first attachment can move abruptly. Initial arming is performed one servo at a time with the robot supported, using an unloaded or mechanically checked joint. A software ramp cannot make an unknown initial physical position safe.
- Calibration uses pulse widths in microseconds, with conservative initial bounds of 1400–1600 microseconds and a 1500-microsecond starting suggestion. These are starting limits, not guaranteed safe travel on an assembled joint. Permit deliberate per-joint widening only within a firmware hard envelope of 1000–2000 microseconds, with endpoints and direction checked physically.
- Store minimum, maximum, initial pulse, direction, and calibrated status per joint. Store a standing target per joint only after calibration. Display logical direction explicitly; pulse endpoints remain ordered regardless of direction.
- Use a nonblocking scheduler with bounded speed and a 20 ms update interval. Apply joint targets and complete-pose commands atomically after validation. Never expand a limit merely to accept a target. No queued gait, inverse kinematics or repeat-motion mode is needed.
- STOP freezes interpolated command positions, keeps attached servos holding and latches out subsequent movement until explicit resume/rearm. A heartbeat timeout of 1500 ms performs the same latched hold, including while detached. Continuing heartbeats cannot release the latch. Reset remains detached and latched. The production browser ACK deadline is 2500 ms, longer than the firmware lease; faults abort queued transport output with bounded cleanup. Already transmitted bytes cannot be recalled.
- RELEASE detaches outputs and may let the robot collapse; label it separately from STOP. A physical servo-power disconnect is still required for electrical or mechanical faults. Software STOP is not a safety-rated emergency stop.
- Use a bounded, newline-delimited serial protocol at 115200 baud, with command acknowledgements, protocol version, explicit errors and authoritative state readback. Validate complete tokens, token count, identifiers, numeric bounds, buffer overflow and duration/speed ranges before changing state. Discard an overlong line through its terminating newline. Rejected commands cause no movement or partial state update.
- Parse a bounded amount of serial input per loop so command flooding cannot starve motion, STOP processing or the watchdog. The UI does not stream an unbounded queue of slider targets.
- Persist versioned, checksummed calibration and the validated standing pose in EEPROM only on explicit save while disarmed. Use a commit marker written last so interrupted writes are rejected on reboot. Corrupt/unsupported data returns to detached, uncalibrated defaults; loading valid data also remains detached. Avoid writing on every slider movement.

## Interface behavior

Provide a clear connection/status panel and four leg cards showing physical labels, joint IDs and Uno pin numbers. Expose individual arming, small pulse nudges, explicit target application, per-joint limits/direction and calibration confirmation. Prefer bounded step controls over continuously sending slider changes.

Provide STOP prominently throughout the interface and RELEASE as a separate, clearly labelled action. Treat command timeout, serial read/write failure and unexpected reset as a loss of authority: disable movement controls, stop pending sends, and require state synchronisation and deliberate rearming before further movement.

The interface sends heartbeats while connected. A firmware watchdog remains authoritative because browsers may suspend background tabs. Demo mode must be visibly labelled and must never claim a physical USB connection, measured pose or hardware validation.

Provide calibration/pose export and import as JSON, validating schema and limits before uploading. Device acknowledgement and state readback establish success; browser storage alone does not mean the device saved settings.

Capture Standing records the current commanded pulses only when all twelve joints are calibrated and attached and the user confirms the supported pose. Move to Standing remains unavailable without a complete validated pose and all twelve joints explicitly armed. It interpolates from the current commanded pose at a bounded speed. Reset/reconnect does not automatically restore an armed condition.

The display calls positions commanded pulses, not measured angles. The servos provide no feedback about slip, stalls, retention or actual foot position.

## Standing and physical checks

No generic 90-degree standing angles are supplied. Record which four of the six mounting stations are occupied and identify a body front during setup. Establish a broad stance with the centre of mass inside the support polygon; foot grip, secure joints and power/torque margin must be checked under staged load. Command interpolation alone cannot guarantee no slipping.

Before load-bearing tests, check the actual horn screws, horn-to-part attachment, backing pivots and free joint travel. The handover records missing backing support and a nominal 0.30 mm interference in one v3 arrangement; it does not prove these faults persist in the current assembly. Begin with the body on a stable support and test one joint, then one leg, before all twelve. Do not flash or initiate physical movement from this cloud task.

## Verification and acceptance

1. Compile the actual Uno firmware and report flash/SRAM usage. Native compilation alone is insufficient.
2. Test the shared control core for disabled startup, unique D2–D13 routing, joint limits, initial arm validation, bounded interpolation, atomic pose acceptance, malformed/overflow input, STOP/resume, heartbeat timeout, timer wraparound, persistence version/checksum and interrupted saves.
3. Test interface state and protocol behavior, including stale acknowledgements, disconnects, rejected commands, reset detection, and a twelve-joint calibration/pose workflow through simulated serial transport. Simulation must be labelled and kept separate from real hardware results.
4. Serve and exercise the actual static interface, including responsive layout, keyboard-accessible controls and the demo workflow. Give local launch instructions for the user's USB-connected computer.
5. Attempt SonarQube analysis if its supported analysis tools or authenticated CLI are available. Report unavailable analysis honestly; do not substitute invented results.
6. Deliver the Arduino sketch, source interface, automated checks and wiring/launch/calibration documentation. List hardware upload, electrical checks, actual joint calibration, loaded standing and slipping checks as unverified until performed on the physical robot.

## Review boundary

This document defines proposed behavior and scope. Review of this design precedes the implementation plan and source changes under the requested Superpowers workflow. No commits, pushes, firmware upload or robot motion are included in this design step.

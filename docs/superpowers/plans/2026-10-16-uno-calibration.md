# Spyder Uno Calibration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver uploadable four-leg Uno firmware and a USB browser interface for individual joint calibration, saved poses and gradual standing tests.

**Architecture:** A portable C++ control core owns twelve joint states and a nonblocking scheduler. A thin Arduino adapter connects that core to Servo, Serial and EEPROM; a static browser application uses a versioned serial protocol and device state readback. Native tests cover the same core compiled into the Uno firmware, and a labelled browser demo exercises the interface without motors.

**Tech Stack:** Arduino Uno/ATmega328P, Arduino Servo library, C++11, PlatformIO, vanilla JavaScript ES modules, Node's built-in test runner, Playwright for browser checks. Use a workspace-local Python virtual environment for PlatformIO; do not install into the system Python.

**Spec:** `docs/superpowers/specs/2026-10-16-uno-calibration-design.md`.

## Global Constraints

- Work in `/workspace/Spyder_Hexapod`, using the existing isolated cloud checkout. Do not create a worktree or change branches. Preserve the uploaded handover and existing work.
- User reports that the servo plugs have been moved. Map L0/J0–J2 to D2–D4, L1 to D5–D7, L2 to D8–D10, L3 to D11–D13. D0/D1 are reserved for USB serial.
- Four legs, three SG90 servos each; Arduino Sensor Shield V5. No PCA9685, ESP32, Bluetooth, walking gait or inverse kinematics in this milestone.
- Boot/reset leaves all servo outputs detached. Connecting USB, reconnecting or loading settings never enables motors or runs a startup pose.
- Initial calibration bounds: 1400–1600 microseconds. Hard pulse envelope: 1000–2000 microseconds. Initial pulse suggestion: 1500 microseconds. None is a physical clearance guarantee.
- Scheduler interval: 20 ms. Heartbeat timeout: 1500 ms. Serial: 115200 baud. Use unsigned elapsed-time arithmetic for timer wraparound.
- STOP and heartbeat timeout latch a hold at the last commanded outputs. Heartbeats alone never resume motion. RELEASE detaches and may cause collapse.
- First attach can jump because the actual servo angle is unknown. Do not promise smooth initial attachment or measured position feedback.
- Persist only on explicit save while all joints are disarmed. Reset/loading valid settings must stay disarmed.
- Browser runs on the user's USB-connected computer using desktop Chrome/Edge; no cloud hardware upload or robot actuation.
- No commits, pushes or deployment without user instruction. Record meaningful verification, including unavailable SonarQube and physical checks.

## Review Focus

1. A USB reset during a pending movement must invalidate the old browser session; it must not replay or rearm queued commands. Test in Task 5.
2. Slow or missing acknowledgements must not leave stale movements queued behind STOP. Test in Tasks 3 and 5.
3. Changing limits/direction or importing calibration must invalidate any incompatible standing pose. Test in Tasks 1, 4 and 5.
4. Power loss during EEPROM save must never load a partially written configuration as valid. Test in Task 4.
5. Background-tab suspension or a stalled serial stream must latch firmware hold and require deliberate recovery. Test in Tasks 2 and 5.

## File Map

| Path | Responsibility |
|---|---|
| `platformio.ini` | Uno target and exact framework/platform/library dependency versions |
| `.gitignore` | Generated build files and local dependency environments |
| `firmware/spyder_uno/spyder_uno.ino` | Arduino setup/loop, Servo adapter, bounded serial service and EEPROM adapter |
| `firmware/spyder_uno/Control.h`, `Control.cpp` | Joint calibration/state, limits, arming, scheduler, STOP/watchdog, standing pose |
| `firmware/spyder_uno/Protocol.h`, `Protocol.cpp` | Strict bounded command parser and serial response formatting |
| `firmware/spyder_uno/Storage.h`, `Storage.cpp` | Versioned explicit-byte EEPROM encoding, validation and committed saves |
| `tests/native/control_test.cpp` | Portable state-machine and timing tests |
| `tests/native/protocol_test.cpp` | Command validation and state-readback tests |
| `tests/native/storage_test.cpp` | Roundtrip, corruption and interrupted-save tests |
| `scripts/test-native.sh` | Compile and execute native tests outside tracked output paths |
| `web/index.html`, `web/styles.css`, `web/app.js` | Accessible calibration console and DOM wiring |
| `web/protocol.js` | Validated commands, authoritative status parsing and import/export schema |
| `web/session.js` | Serial lifecycle, command correlation, bounded sends and recovery |
| `web/demo.js` | Visibly simulated transport, separate from real USB connection |
| `tests/web/*.test.js` | Node unit tests for protocol, schema and serial-session behavior |
| `tests/browser/calibration.spec.js` | Actual browser demo workflow and control gating |
| `package.json`, `package-lock.json`, `playwright.config.js` | Reproducible JavaScript tests and browser test configuration |
| `README.md`, `docs/bring-up.md` | Wiring, installation, flashing, interface launch and supported calibration sequence |

## Shared Interfaces

Use namespace `spyder` and fixed-size types from `<stdint.h>`. `JointConfig` contains `uint16_t minimum`, `maximum`, `initial`; `int8_t direction` restricted to -1 or 1; and `bool calibrated`. `JointState` contains commanded and target pulse widths plus `bool attached`. `Config` contains twelve joint configurations, twelve standing pulses and a `bool standingValid` flag. Physical leg labels remain browser metadata, not fixed geometry.

`enum class Result : uint8_t` distinguishes success, invalid input, range violation, wrong state, missing calibration, missing pose, transport overflow and storage failure. Define the exact enumeration in `Control.h` and map it once to protocol error names.

`class Outputs` exposes `virtual void attach(uint8_t joint, uint8_t pin, uint16_t pulse)`, `write(uint8_t joint, uint16_t pulse)` and `detach(uint8_t joint)`. The adapter must preload the pulse before attachment to avoid briefly emitting a library default. Tests record calls; no native test accesses motors.

`class Controller` exposes `begin(uint32_t now)`, `arm(uint8_t joint, uint16_t pulse, uint32_t now)`, `move(uint8_t joint, uint16_t pulse, uint16_t speed)`, `configure(uint8_t joint, const JointConfig&)`, `stop()`, `release()`, `resume(uint32_t now)`, `heartbeat(uint32_t now)`, `tick(uint32_t now)`, `captureStanding()`, `stand(uint16_t speed)`, `applyConfig(const Config&)`, `config() const`, `joint(uint8_t index) const`, `stopped() const`, `allDetached() const`. Mutators return `Result`, except the unconditionally valid stop/release/heartbeat/tick methods. Readback never mutates state. Constructor accepts `Outputs&`.

Keep speed in integer microseconds per second, range 10–100, default 50. Integrate in 20 ms bounded steps with a remainder accumulator so slow motion does not round to zero. If the event loop stalls, cap per-tick progress to one normal step rather than catching up with a large jump. Firmware checks the heartbeat before progressing motion. Also expose `Result setStanding(const uint16_t (&pulses)[12])`: while detached and fully calibrated, atomically validate and store an imported standing pose without moving or attaching outputs.

`class Protocol` accepts `Controller&`, an output text sink, and a storage service; `feed(char byte, uint32_t now)` accumulates a maximum 191-character command plus terminator and discards overflow through newline. `poll(uint32_t now)` is unnecessary: scheduler/watchdog remains in `Controller::tick`. The Arduino loop processes at most 32 serial bytes before calling `tick` again.

Commands are ASCII `requestId VERB arguments\n`, with positive integer request IDs limited to 65535. Commands: `HELLO`, `STATUS`, `PING`, `ARM joint pulse`, `MOVE joint pulse speed`, `CONFIG joint min max initial direction calibrated`, `STOP`, `RELEASE`, `RESUME`, `CAPTURE`, `POSE p0 p1 p2 p3 p4 p5 p6 p7 p8 p9 p10 p11`, `STAND speed`, `SAVE`, `LOAD`. Joint IDs are 0–11, ordered by leg then joint. Configuration edits and LOAD require all joints detached. POSE calls setStanding and changes saved targets only. Capture requires every joint attached and calibrated and no active transition. Stand requires every joint attached and calibrated, a valid pose, and no STOP latch.

Responses: `ACK id`, `ERR id code`; HELLO additionally identifies `SPYDER 2`. STATUS emits `BEGIN id stopped standingValid`, twelve `JOINT id index pin min max initial direction calibrated attached commanded target standing` rows, then `END id`. The receiver commits state only after a complete matching snapshot. Boot emits `BOOT SPYDER 2`; unexpected boot invalidates any pending session. Invalid request IDs or overflow use `ERR 0 code`. Do not ACK a request whose action failed.

No unsolicited high-rate state streaming. The interface permits one ordinary user command in flight; urgent STOP/RELEASE bypass response waits and cancel superseded work. It sends heartbeats at 250 ms and periodically reads STATUS. Acknowledgements use request IDs and a connection-generation counter; old promises and sends are discarded on reset or disconnect. STOP clears queued movement and is sent ahead of new traffic; wait for a matching acknowledgement and fresh state before reporting held. Failure to deliver STOP is shown as unknown device state, not success; the watchdog is the fallback.

`class ByteStore` exposes `uint8_t read(uint16_t address)` and `void update(uint16_t address, uint8_t value)`. `bool loadConfig(ByteStore&, Config&)` validates explicit bytes, version, checksum and all semantic bounds. `bool saveConfig(ByteStore&, const Config&)` invalidates the marker first, updates payload/checksum and commits the marker last. Avoid serializing raw C++ structs because padding is not a stable storage format. Reserve less than the Uno's 1024-byte EEPROM and check at compile time.

## Task 1: Joint configuration and disabled startup

**Files:** Control.h/Control.cpp, tests/native/control_test.cpp, scripts/test-native.sh, .gitignore.

**Interfaces:** Produces `JointConfig`, `JointState`, `Config`, `Result`, `Outputs` and Controller construction/readback/configuration/arming/release APIs defined above. Later tasks extend the same class.

- [ ] Write native tests: exactly twelve unique output pins equal 2–13; `begin(0)` emits zero attach/write calls; each default config is 1400/1600/1500; invalid joint IDs and pulses outside bounds are rejected with no output calls.
- [ ] Run `bash scripts/test-native.sh` and confirm a missing implementation causes the focused failures.
- [ ] Implement the types and minimum tested Controller behavior; validate configuration as a whole before assignment. Config changes require disarmed outputs, validate -1/+1 direction, and invalidate the standing pose. Never auto-arm when applying settings.
- [ ] Add tests rejecting min >= max, initial outside endpoints, hard bounds outside 1000–2000 and partial invalid configurations; assert state remains unchanged on rejection.
- [ ] Run the native checks; confirm all tests in this task execute and pass.

## Task 2: Motion, STOP, watchdog and standing pose

**Files:** Control.h/Control.cpp, tests/native/control_test.cpp.

**Interfaces:** Consumes Task 1 types; completes the Controller scheduler, watchdog and standing APIs.

- [ ] Write failing tests: 100 microseconds at 50 microseconds/second takes 2000 ms; a zero-distance move emits no drift; low speed 10 advances by one microsecond after five 20 ms ticks; overdue updates cannot make a large catch-up jump. Feed heartbeats throughout the motion timing tests so they independently exercise interpolation rather than watchdog expiry.
- [ ] Add tests for STOP mid-move, no target resumption after RESUME, heartbeat latch after 1500 ms, PING not clearing STOP, no arming while latched, timer rollover around `UINT32_MAX`, and no scheduler progress before heartbeat evaluation.
- [ ] Add tests that CAPTURE fails if any joint is uncalibrated/detached/moving; STAND rejects an absent or out-of-limits pose atomically; rejected poses do not update the other eleven targets. Test setStanding rejects partial calibration or any attached joint and validates all twelve pulses before assignment; accepted imported poses emit no attach/write calls.
- [ ] Run the native test command and confirm these tests fail for the missing behaviors.
- [ ] Implement the bounded scheduler, integer remainder handling and latch behavior. Explicit RESUME refreshes heartbeat but does not restore previous movement targets. RELEASE always detaches all joints and cancels pending transitions; it does not silently clear a STOP latch.
- [ ] Run the native suite and confirm all new timing and state tests pass.

## Task 3: Strict serial control and authoritative readback

**Files:** Protocol.h/Protocol.cpp, tests/native/protocol_test.cpp, scripts/test-native.sh.

**Interfaces:** Consumes Controller; produces Protocol::feed and the exact command/response grammar above. Use a `TextSink::write(const char*)` output interface and `SettingsService::save()/load()` returning Result, with the actual EEPROM service supplied in Task 4.

- [ ] Write failing tests through `feed`: every valid command, ACK/ERR correlation, HELLO version, complete STATUS framing, missing/extra arguments, negative identifiers, nondecimal text, huge integers, invalid direction/calibrated values, CRLF and non-ASCII input.
- [ ] Test overflow recovery: an overlong line containing `ARM` or `RESUME` fragments never executes; the following clean STATUS succeeds. An incomplete command causes no mutation.
- [ ] Test STOP followed by queued MOVE: STOP takes effect and MOVE is rejected; malformed STOP has no unintended side effects. Test SAVE/LOAD dispatch and storage error reporting.
- [ ] Run the native suite to establish expected failures, then implement fixed-buffer token parsing with strict full-token numeric conversion. Do not use unchecked atoi, String, unbounded sprintf or dynamic token containers.
- [ ] Implement readback formatting in bounded pieces and rerun the suite. Ensure firmware state, not UI assumptions, controls command success.

## Task 4: EEPROM and real Uno build

**Files:** Storage.h/Storage.cpp, spyder_uno.ino, platformio.ini, tests/native/storage_test.cpp, scripts/test-native.sh.

**Interfaces:** Consumes Config/Controller/Protocol; produces ByteStore, saveConfig/loadConfig, concrete SettingsService and the Arduino runtime adapter.

- [ ] Write failing storage tests: calibrated configuration/pose roundtrip, checksum corruption, erased EEPROM, unsupported schema, invalid semantic limits, valid checksum with invalid data, and simulated power interruption after every individual byte update.
- [ ] Implement explicit-byte encoding and marker-last save. An interrupted save may discard the old configuration but must never accept a partial new one. A failed LOAD leaves the existing runtime configuration unchanged.
- [ ] Run native storage tests; confirm the interrupted-save tests actually iterate every write boundary.
- [ ] Add a Uno PlatformIO environment with `src_dir = firmware/spyder_uno`, pinned `atmelavr` platform and Servo library. Verify the selected published versions before installation, preserving TLS/checksum verification. Add a workspace-local PlatformIO install instruction and ignore its environment/build paths.
- [ ] Implement setup/loop with Serial at 115200, EEPROM adapter, heartbeat/tick service and bounded serial processing. Preload each explicitly selected pulse before Servo attachment. Boot outputs stay detached, including when saved configuration is valid.
- [ ] Run `.venv/bin/pio run -e uno`; inspect warnings, the full exit status, flash usage and SRAM usage. Fix build/configuration problems and rerun; do not upload. If static SRAM leaves insufficient stack margin, reduce buffers/state before considering the build complete.

## Task 5: Browser calibration console and serial lifecycle

**Files:** web/*, tests/web/*.test.js, tests/browser/calibration.spec.js, package.json/package-lock.json, playwright.config.js.

**Interfaces:** Consumes Task 3 protocol. `encodeCommand(id, verb, args)` validates and returns one line; `parseLine(line)` returns a typed response or parse error; `validateProfile(value)` returns a fully validated profile or throws. `Session` exposes `connect(port)`, `disconnect()`, `command(verb,args)`, `stop()`, and state events. Transport offers write/read/close and has distinct Web Serial and demo implementations.

- [ ] Write Node tests for command bounds, full STATUS assembly, wrong/missing/duplicate joint rows, stale request IDs, partial responses, resets, connection generations, ACK timeout, rejected commands, and complete profile validation before any writes. Run `node --test tests/web/*.test.js` and confirm expected missing-behavior failures.
- [ ] Implement protocol/profile validation and Session. Require matching ACK plus STATUS before reporting state-changing success. Invalidate unsent queued moves on STOP/disconnect/reset; serialize all writes and prioritise STOP. Heartbeats cannot release firmware hold. Unexpected reconnect never replays actions.
- [ ] Implement the labelled demo transport and visible demo/USB status distinction. Simulated attachment and readback must not be presented as hardware evidence. Test the demo through the same Session API.
- [ ] Implement the page: connection panel; four named leg cards and pin labels; explicit per-joint starting pulse/arming; bounded nudges; configurable endpoints/direction/calibrated status; STOP and separate RELEASE; explicit device SAVE/LOAD; validated JSON import/export; standing capture and slow playback gated by device state. Add profile metadata for leg physical positions and body front.
- [ ] Implement import while disarmed: validate all twelve configs and pose first, then upload configs in order, read back, and show incomplete import if any request fails. Do not claim the previous configuration is preserved after a mid-transfer disconnect. A profile's imported pose is sent through POSE only after all settings are confirmed and the user explicitly confirms the profile came from a calibrated supported pose; never activate movement on import.
- [ ] Write browser tests for demo connection, one-joint calibration and arming, twelve-joint standing capture, STOP during transition, RELEASE, invalid imports, control gating after disconnect/reset, keyboard accessibility and narrow-screen controls. Test actual Web Serial session logic with a fake port in unit tests, separately from the demo UI.
- [ ] Run `npm test` and `npm run test:browser`. Confirm the expected test counts and inspect screenshots for desktop/mobile overflow and prominent STOP. Run the actual static server and verify its page/module requests; do not present localhost as a cloud preview link.

## Task 6: Bring-up instructions and integrated verification

**Files:** README.md, docs/bring-up.md, any narrowly necessary fixes from integration.

**Interfaces:** Consumes the finished firmware, console and build/test commands; produces a reproducible user handoff.

- [ ] Document the exact signal map and S/V/G orientation checks; regulated external power/common ground and shield rail jumper uncertainty; first-attach jump; mechanical retention checks from the handover. Record which four stations are occupied before loaded standing. Do not invent a power rating or a fitted CAD assessment.
- [ ] Document Arduino IDE sketch opening/Servo dependency and PlatformIO build/upload as user-executed commands. Explain that Arduino Serial Monitor must be closed before Web Serial connects and that opening the port can reset the Uno.
- [ ] Document starting the static interface on the USB-connected computer, Chrome/Edge requirements and demo mode. Provide a supported one-servo, one-leg, all-joint calibration and standing sequence, with no guessed pose angles and no guarantee against slipping.
- [ ] Rerun `bash scripts/test-native.sh`, `npm test`, `npm run test:browser`, `.venv/bin/pio run -e uno` and `git diff --check` after integrated changes. Record commands, counts, exit statuses and flash/SRAM usage. Inspect the final diff for unrelated files, generated output and secrets.
- [ ] Run supported SonarQube analysis if accessible. If no MCP tools or authenticated CLI exist, report it as unrun; do not install or configure an unrelated external account without authorization.
- [ ] Map each design acceptance criterion to its fresh check. Explicitly list real USB upload, electrical measurements, joint retention, real servo response and loaded standing/slip tests as unverified in this cloud environment.

## Plan Self-Review

- Specification coverage: Tasks 1–4 cover firmware, persistence and Uno build; Task 5 covers interface/serial/demos/import; Task 6 covers wiring, physical checks, local launch and evidence.
- Interface consistency: Controller types live in Control.h; Protocol depends on Controller and SettingsService; EEPROM implements SettingsService using the same Config schema. Browser follows the same IDs and framed state grammar.
- Failure modes: all five review-focus items have explicit tests above. Physical stability and supply adequacy require hardware evidence and are not substituted by native tests.
- Scope: no upstream ESP32 build or six-leg conversion; no hardware actuation; no worktree/commit/push. This is a coordinated firmware/interface milestone with one protocol contract.

## Execution Handoff

Proposed execution: native, in this session. The tasks share a small protocol and state model, so one implementer can keep integration consistent without per-task delegation overhead. A fresh final review can be delegated if the user selects an execution method that includes it. No agents have been started.

Review this plan and choose native execution or subagent-driven execution before implementation, as required by the requested Superpowers workflow.

## Execution result

Implemented following user approval. All six tasks are complete using the recorded build substitution: PlatformIO was attempted but proxy-blocked; the actual Uno image compiled with the verified Debian AVR toolchain instead. See `docs/verification.md` for fresh test counts, memory usage, review fixes, deferred demo timing and hardware limitations. Startup/idle hold was tightened during the independent review and is reflected in the updated specification. Work remains uncommitted; no hardware was flashed.

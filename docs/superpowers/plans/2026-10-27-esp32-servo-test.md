# ESP32 PCA9685 single-servo implementation plan

> **For agentic workers:** Use superpowers:executing-plans to implement task by task, with focused failing checks before implementation and a fresh final review.

**Goal:** Deliver the approved unloaded, one-channel Bluetooth/USB servo bench as an Arduino IDE sketch and ZIP.

**Architecture:** A hardware-independent controller and two-source line parser own all commands and motion. A small ESP32 adapter owns checked PCA9685 transactions, OE, Bluetooth events and nonblocking reply handoff.

**Tech stack:** C++11-compatible core, Arduino ESP32, BluetoothSerial, Wire, Adafruit PWM Servo Driver, native C++ sanitizer tests, Arduino CLI compilation.

**Spec:** `docs/superpowers/specs/2026-10-27-esp32-pca9685-servo-test-design.md` (owner approved in chat).

## Global constraints

- New files only; preserve Uno code, existing tests and untracked one-leg work.
- Original ESP32; GPIO21 SDA, GPIO22 SCL, GPIO23 OE with verified external pull-up (10k only without a conflicting board pull-down; see spec wiring clarification); PCA9685 address 0x40/channel 0.
- All outputs off at boot; explicit `arm`; electrical envelope 1000–2000 us; 90 nominal degrees = 1500 us.
- Motion speed 50 us/s, nonblocking, no delayed catch-up; finite tests/sweep; no storage or walking.
- USB 115200 and Bluetooth Classic SPP `Spyder-Servo-Test`; independent lines and source-specific replies.
- I2C errors latch fault and disable OE; Bluetooth loss releases and cancels pending input.
- Local delivery only, no hardware upload or GitHub push before owner test.

## Review focus

- Reset while PCA9685 retains register contents: OE must remain high until confirmed preload.
- Disconnect racing with arming: callback and output-enable must share a guard.
- Stalled reply transport: command and safety servicing must continue.
- Endpoint tick rounding and real prescale: nominal electrical endpoints must round inward.
- Partial/overflow input on one port followed by commands on the other: cancel motion and partial input consistently.

## Task 1: Controller and input contract

Files: create `firmware/spyder_esp32_servo_test/ServoBench.h`, `ServoBench.cpp`, `tests/esp32_servo/bench_test.cpp`, `scripts/test-esp32-servo.sh`.

Interfaces: `Output::enable(uint16_t)`, `write(uint16_t)` return confirmed success; `disable()` immediately inhibits output. `Bench::command(line, now)`, `tick(now)`, `release()`, `hold()`, `fault()` control state. `Inputs::feed(source, byte, now)` returns command/reply results; `expire(now)` and `clear()` handle integrity and transport resets.

- [x] Write behavior checks for released boot, arming, rejected input/ranges, degree mapping, finite sweep, two-second type tests, STOP/OFF, timing wrap/delays, faults and independent input/safety cancellation.
- [x] Run the new test script; demonstrate missing functionality, then implement the smallest controller/parser satisfying the checks.
- [x] Run sanitizer checks and inspect outputs; preserve prior suites.

## Task 2: ESP32/PCA9685 adapter and delivery

Files: create `firmware/spyder_esp32_servo_test/PcaOutput.h`, `PcaOutput.cpp`, `spyder_esp32_servo_test.ino`, `README.md`, `scripts/build-esp32-servo.sh`, focused adapter harness if needed, and a verification note.

Interfaces: consume Task 1 controller/parser; `PcaOutput::begin()` initializes with OE high; confirmed `enable/write` and immediate `disable` implement Output. Main loop services bounded input/motion; a separate transmit task prevents congested Bluetooth replies blocking safety. Bluetooth close callback inhibits OE under the same critical-section guard used to enable.

- [x] Check driver return semantics and configure/read back MODE1/MODE2/PRESCALE under disabled OE. Check each channel-off write and channel-0 write. Add focused harness assertions for rejected initialization/write and enable/disconnect ordering.
- [x] Implement the adapter and complete Arduino IDE sketch; compile for original ESP32 using a local toolchain if available.
- [x] Document exact wiring, power branches, IDE libraries/board/port, pairing and incremental test commands; distinguish nominal angle/pulse from measured position.
- [x] Run native checks, actual ESP32 compile, existing npm suite and diff checks. Obtain one independent whole-change review; resolve material findings with regression evidence.
- [x] Package complete sketch, instructions and verification results into `/workspace/Spyder-delivery/Spyder-ESP32-PCA9685-Servo-Test.zip`; verify ZIP contents and hashes. Leave implementation unpushed pending physical test.

## Execution record

Use the existing cloud checkout and feature branch. The owner explicitly approved implementation and checks; proceed inline without another approval question about execution mechanics. No new worktree, publication or hardware action is required.

> Historical audit of revision 1. Its software findings were fixed in revision 2; see [verification](../verification.md). Referenced raw reproduction files belonged to the audit workspace and are not all part of this repository.

# Spyder calibration audit — 16 October 2026

**Verdict: the delivered version is not ready for further powered robot calibration.** USB-only servo power is unsuitable, and additional software/signal faults were reproduced. The earlier test suite passed but did not cover the relevant driver waveforms and interaction sequences. Those earlier results were too narrow to establish reliable calibration.

This audit did not change production code or the delivered ZIP. All 34 source files still match the ZIP's SHA-256 manifest. Reproductions, traces and this report are outside the project checkout.

## Confirmed hardware setup and limits of diagnosis

The user reports twelve SG90 servos powered from the Arduino's USB supply, and unwanted movement while connecting, resetting or releasing outputs. USB/Uno power is not an appropriate twelve-servo supply. Rail sag, servo electronics restarting and controller resets are plausible mechanisms for the observed symptoms; no real voltage, current or reset waveform was measured here, so their exact contribution is unverified.

Before further powered testing, use a separate regulated supply at the actual servos' supported voltage, sized for their combined load/startup current. SG90 bench setups normally use a regulated 5 V servo rail, subject to the actual servo specification. Share ground with the Uno. Verify the particular Sensor Shield V5's external-power terminal and selection/isolation jumper before connecting it: a shield can otherwise join its servo rail to Uno/USB 5 V. Do not route the combined servo load through the Uno. Start with one unloaded/supported servo. Exact current rating, distribution and protection must be selected for the actual motors and wiring; no final whole-robot power budget is established.

RELEASE also removes the commanded holding signal. Mechanically loaded joints may move or the body may drop even with correct software and adequate power. This is separate from unwanted electrical pulses.

## What was executed

- Existing native C++ sanitizer checks: passed, including controller, protocol and EEPROM checks.
- Existing JavaScript/native integration suite: 13 passed.
- Existing Chromium workflow suite: 2 passed.
- Actual Uno compilation: passed using the previously verified Debian AVR toolchain; 11,298 bytes flash and 1,015 bytes static RAM.
- Actual compiled AVR firmware emulated using simavr 1.6 on an ATmega328P at 16 MHz, ideal 5 V: UART commands injected, D2–D13 GPIO edges recorded. This executes the real Arduino core and Servo 1.2.2 interrupt code, not the browser demo.
- Independent browser review and reproductions, subsequently rerun by the coordinator: Chromium plus a native real-C++ firmware bridge for serial/heartbeat behavior.
- Verified simulator packages against the previously signed Debian package index; no TLS, package-signature or checksum verification was disabled.

The emulator does not model USB supply impedance, real SG90 internals, actual joint position, mechanical load, horn slip, foot friction or balance. No hardware was flashed or operated. SonarQube remains unavailable. A reference-bootloader emulation experiment was inconclusive and is excluded from positive simulation claims.

## Confirmed findings

### 1. High: RELEASE can leave a servo signal continuously HIGH

Location: `firmware/spyder_uno/spyder_uno.ino:19`, and Servo 1.2.2 `src/avr/Servo.cpp:255–261` / its interrupt handler.

The adapter only calls `Servo::detach()`. The library clears its active flag; it does not drive the signal LOW. Its interrupt handler then skips the falling edge for inactive channels. Releasing during a pulse therefore leaves that pin HIGH.

**Actual AVR reproduction:** prepare `50 RELEASE` without its newline; wait for D2 to rise, then inject the newline. D2 remained HIGH for over 100 ms. In a release/rearm run, the recorded HIGH interval was **68,448.56 µs**, instead of approximately 1,500 µs. A sweep of twenty 1 ms phase offsets with all twelve channels active left one signal HIGH in **18 of 20** cases. The affected channel depended on release timing.

Evidence: `release-high.log/.csv`, `reattach.log/.csv`, `release-sweep.csv`, and `release-offset-*.log/.csv`. This establishes malformed signal behavior, not a measured physical servo response to it.

Correction direction: atomically detach and drive the corresponding GPIO LOW; ensure a defined LOW application startup state. Verify the actual pin waveform, not just Controller's attached flag, across release/rearm phases.

### 2. High: the enabled Release button can silently do nothing

Locations: `web/app.js:32`, `:140`, `:226–232` in the reviewed version.

The button stays enabled during another action, but its handler goes through `action()`, which immediately returns when busy. In a browser reproduction with a 500 ms pending status readback, clicking and confirming Release sent **no RELEASE command**; the joint remained attached and no error was shown. The delay widens an existing real pending-operation window.

Correction direction: give RELEASE a reliable urgent-control path, independent of the ordinary-action busy guard, with acknowledgement and fresh state readback.

### 3. Medium: frequent target changes can prevent slow movement

Location: `firmware/spyder_uno/Control.cpp:66`.

Every MOVE resets the fractional progress accumulator. With real Controller ticks/heartbeats every 20 ms at 10 µs/s:

| Test | Commanded pulse after 400 ms |
|---|---:|
| One MOVE from 1500 toward 1550 | 1504 µs |
| New increasing target every 80 ms | 1500 µs |
| Stop changing the target, allow another 400 ms | 1504 µs |

The repeated commands erase progress before it reaches one whole microsecond. This directly establishes inconsistent response based on click cadence.

Evidence: `nudge.cpp`, `nudge.log`. Correction direction: retain fractional progress across compatible target updates, resetting it only when cancellation/direction semantics require it; test speed changes and reversal explicitly.

### 4. Medium: continuous operation can starve the heartbeat

Location: `web/session.js:49–56`; only PING/RESUME renew the firmware lease.

The periodic PING is sent only when no command, queue or readback is busy. Repeated foreground nudges with 200 ms readbacks kept the session busy for more than 1500 ms. The actual native firmware entered latched hold and cancelled the target, despite ongoing valid command traffic. The first PING arrived after the lease expired. The UI continued to show the old arm notice rather than explaining the newly latched hold.

Evidence: `heartbeat-reproduction.log`, browser/native bridge `real-ui.cjs`. This bridge ticks the native Controller on input; its position numbers are not used as physical/timing evidence. Its command chronology and hold state directly establish the lease fault.

Correction direction: service PING with a bounded interval during ordinary traffic, including between command and readback phases. Do not clear an explicit STOP automatically or casually renew the lease from stale motion commands.

### 5. Medium: Initial pulse and the actual arming value disagree after editing

Location: `web/app.js:90–104`, `:175–183`, `:68–72`.

Apply calibration with Initial pulse = 1450. Device configuration changes to 1450, but the separate Target/starting input remains 1500. Subsequent Arm sends `ARM 0 1500`. The firmware obeys that explicit argument; the surprising UI values are the defect.

Correction direction: make the calibrated initial pulse populate the arming input, or clearly separate arming and movement values so the selected start is unambiguous.

### 6. Medium: blocking status output changes effective movement speed

Locations: `spyder_uno.ino:41`; `Control.cpp:105–108`.

A full twelve-joint STATUS is printed synchronously at 115200 baud. While the transmit buffer drains, the main loop does not call Controller::tick. The scheduler intentionally counts at most one 20 ms step when it resumes, discarding additional elapsed time. This protects against a large catch-up jump but means serial load affects speed.

In the actual AVR serial-stress run, a 50 µs/s request moved about 28 µs during approximately 1.14 s of waveform observation, considerably below the nominal roughly 57 µs. Existing host tests with fixed 20 ms ticks could not expose UART blocking.

Correction direction: service motion timing while serial output drains, or implement bounded nonblocking output. Preserve STOP/watchdog responsiveness and the limit on unexpected catch-up motion.

### 7. Confirmed latency limitation: STOP waits for prior readback

Location: `web/session.js` queue/drain/stop methods.

Priority only reorders queued jobs; it does not interrupt the active readback. With an 800 ms delayed readback, STOP transmission occurred about **771 ms after the click**, and the demo continued moving before hold. This is a real host serialization issue; the demo's position change is not a physical measurement. Normal actual UART snapshot duration is much shorter, but stalled responses expose the delay.

Correction direction: allow urgent STOP/RELEASE to be transmitted while an earlier response is pending, preserving complete-line write ordering, request correlation and fresh safety-state readback. Already transmitted bytes cannot be recalled, and the physical power switch remains necessary.

## Other relevant behaviors and limitations

- At a steady 1500 µs command, the actual AVR simulation produced approximately **1499.69–1500.75 µs** pulses across D2–D13, with approximately **20.00 ms** periods under the tested ideal conditions. It did not show arbitrary other-channel writes during those steady tests.
- The application-only startup observation produced no servo pulse before intentional arming. That excludes neither power-on servo behavior nor bootloader pin activity.
- D13 is also the Uno's built-in LED pin. The supplied reference Optiboot source configures/toggles PB5/D13 during external reset before the sketch runs. The actual user's bootloader was not read. Treat D13 reset activity as a hardware-integration risk; moving that one servo signal to an otherwise free A0 is an option requiring an explicit wiring/code change, not something this audit silently applied.
- A minus nudge reduces the outstanding target, not the current commanded pulse. At commanded 1508 / target 1600, minus sets target 1595; motion still goes upward. Other armed joints also finish their own outstanding moves. These can feel involuntary but are current control semantics, not random channel selection.
- Arming an unloaded servo at a pulse different from its unknown actual position can jump immediately; software has no position feedback.
- SG90 deadband, backlash and load can make 5 µs changes physically intermittent even when the electrical pulse changes correctly. No exact deadband is assumed for the user's motors.
- The demo always animates at approximately 50 µs/s and also differs from firmware's ARM lease behavior. It is not a reliable timing or electrical simulation.

## Recommended order

1. Correct the servo supply/ground/rail isolation and keep the robot supported; do not resume twelve-servo USB-only tests.
2. Correct the release waveform and ignored Release action, then watchdog scheduling and movement timing/UI issues.
3. Add the reproduced failing interaction and AVR waveform cases to the retained regression suite, verify the Uno build and issue an explicitly revised ZIP. The existing ZIP has not been replaced by this review.
4. Check reset behavior of the actual D13-connected servo and choose whether to move its signal to A0. Start with servo power off during USB connection/reset.
5. With an external supply, test one unloaded servo first; record its rail voltage, supply current, relevant pin and whether it was armed. Move to supported legs only after consistent single-servo behavior.

This is a review and simulation report. Production corrections have not been applied in this pass.

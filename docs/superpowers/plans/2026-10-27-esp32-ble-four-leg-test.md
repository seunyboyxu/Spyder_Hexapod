# Four simultaneous BLE leg tests

The user requests the existing ESP32/PCA9685/iPhone BLE leg tests on four legs at the same time, with hip/knee/foot channels **0/1/2**, **3/4/5**, **6/7/8**, **9/10/11**. Implement in a new complete Arduino folder, preserving the previous candidates, all existing source/tests and downloads. Existing Arduino2.0.17, NUS BLE, USB115200, SDA21/SCL22/OE23,1472us nominal90° leg scale,1000–2000us envelope, bounds/calibration/geometry remain applicable.

## Contract

- `leg 1` through `leg 4` select one leg for the earlier `cal/move/zero/test/flip/check/limits/speed` commands, using local joint0/1/2. Start selected on leg1. `leg all` selects coordinated `stand`, `straight`, `step [1..3]` and `speed N`. Calibration changes in all mode are rejected. `show` reports selection; `show all` reads all12 joints without changing selection.
- Reuse four copies of the existing three-joint controller behind per-leg channel mapping. Add only pure motion preflight, a shared tick origin, an at-target predicate and optional externally controlled step advancement to the new controller copy. Preserve its default single-leg behavior.
- Before a group pose/step, require all12 joints attached, checked, stationary and all waypoints within their own limits. Rejection changes no target/output. Start all four at the same tick origin; during group steps advance a phase only after every leg reaches it. Different distances/speeds can give different arrival times; faster legs wait. This is a simultaneous supported bench test, not a walking gait or a balance controller.
- `armall` is an explicit convenience after all12 are checked and all released: attach one stored-reference joint per100ms, nonblocking, with the body supported. STOP cancels remaining attachments and holds existing ones; OFF/disconnect clears all. Other mutations/selections are rejected during staged activation/group motion. Read-only status and safety remain available.
- STOP holds all current commands and cancels every sequence. OFF/urgent! inhibits all12 through OE before register cleanup. BLE disconnect/unsubscription releases all and never resumes automatically. I2C failures latch all-off; transport/input integrity errors hold all and cancel partial data.
- Only PCA channels0–11 may activate;12–15 remain off. Use a16-bit active mask. Per-joint release must preserve other active joints; global cleanup clears all12. USB/BLE replies remain nonblocking; allocate room for a13-line all-joints status without falsely triggering queue overflow.
- Store four independent calibrations in one distinct versioned/CRC-checked Preferences blob (`spyder-four` namespace), never import/replicate single-leg calibration automatically. Save/load only with every output off; validate all legs before applying; boot loading never activates motors.
- Advertise `Spyder-4Leg-BLE` with the existing NUS service. Keep the vetted BLE transport unchanged. Use the earlier corner map only as historical context; directions must be physically checked per joint. Simultaneous local stepping is not global forward walking.
- No upload, motor movement or GitHub push. Explicitly document external regulated5V power sized for12 motors' combined stall demand, driver/terminal limits and supported-body testing. USB/ESP32 power is not a servo supply.

## Implementation and evidence

- [x] Write failing portable coordinator and12-channel adapter checks; copy existing controller and add the minimal coordination hooks. Keep earlier unit tests running against the new controller copy.
- [x] Implement all-leg preflight/barriers, independent calibration/selection, staged armall, global safety, four-leg storage codec and the mapped PCA adapter. Cover channels8–11, unused channels, interrupted arming, timing, rejected fourth-leg preflight and unequal-speed phase waiting.
- [x] Reuse the BLE UART/parser and adapt the sketch/status/Preferences. Add actual-sketch checks for all12 boot-off, BLE routing/control, status volume, storage, disconnect/input/I2C failures and no stale staged attachments.
- [x] Compile the actual original-ESP32 Arduino target, run sanitizer checks plus the existing npm suite, and obtain one independent read-only review; fix material findings with regression evidence.
- [x] Package a new complete Arduino ZIP with channel/wiring map, per-leg calibration and supported simultaneous test instructions, plus fresh verification and hashes. Preserve all prior packages pending owner's physical test.

Additional authorized fix: reproduce the user's newer-core `getValue()` String conversion failure; use auto plus c_str()/length() in both BLE adapters, add a String-API regression build, refresh the one-leg package, and compile the new four-leg sketch against real ESP32 core3 when available. Existing one-leg changes are limited to this explicitly requested compatibility fix/documentation.

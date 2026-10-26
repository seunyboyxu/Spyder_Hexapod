# Four-leg Bluetooth test controller

**Design for owner review — 26 October 2026.** Approach and physical leg numbering selected by the owner; this document is not yet implementation approval. No robot upload or hardware movement is authorised from the cloud.

## Intent and confirmed decisions

Provide a standalone Arduino sketch to bring up all four installed legs of Spyder using only an HC-05 terminal after upload. Start with individual servo calibration and supported leg tests, establish a standing pose, then try finite slow creep cycles. Keep the existing calibration, browser, single-leg and walking sketches intact.

The owner selected a **new test controller with calibration, standing and finite creep cycles**, and confirmed the following map, viewed from above with the chosen front:

| Leg | Position | Hip | Knee/thigh | Foot | Global joint IDs |
|---|---|---|---|---|---|
| 1 | Front-left | D2 | D3 | D4 | 0, 1, 2 |
| 2 | Front-right | D5 | **D6** | D7 | 3, 4, 5 |
| 3 | Rear-left | D8 | D9 | D10 | 6, 7, 8 |
| 4 | Rear-right | D11 | D12 | D13 | 9, 10, 11 |

The owner's D6 decision supersedes the fetched `CLAUDE.md` entry assigning that knee to D1. The old D6 problem is not claimed to be repaired: test joint 4 individually before enabling its whole leg. Physical numbering supersedes earlier guessed corner/side arrays. Left-side legs are **1 and 3**; right-side legs are **2 and 4**.

Hardware: Arduino Uno, Sensor Shield V5, twelve positional SG90/MG90D servos, plain `Servo.h`, no PCA9685. D0/RX and D1/TX are reserved for the HC-05 COM connection at **9600 baud**, with replies. No button or USB terminal is required to operate the test.

## Approaches considered

1. **Selected: new joint-angle test controller.** Reuse the single-leg controller's deliberate arming, calibration, limits, strict input and immediate cancellation; adapt the newer simple walking sketch's standing-relative creep idea. Calibrate through Bluetooth and capture the actual stance, avoiding dependence on unmeasured link geometry. A joint-space gait does not itself establish balance or foot contact.
2. **Not selected: extend the older IK walker.** Its foot-space interpolation and body lean are useful later, but depend on fitted dimensions, mount frames and calibrated angle transforms. Its default leg-corner map differs from the owner's confirmed map. Do not copy its simulated stability margin onto this new controller.

Baseline fetched and fast-forwarded: `walking-gait` at **4930debd4ff43dd1264d0049343b928d9903bd70**, including the Arduino prototype/type fix and the newer standing/live-adjust sketch. Preserve source attribution in the new README. The local `firmware/spyder_leg_test/` COM edition is additional, uncommitted reference work; it is not represented as part of that GitHub commit.

## Important range decision for this design

Keep the previously agreed **1000–2000 microsecond command envelope**. Expose nominal Arduino Servo-library degrees, using its default 544–2400 microsecond angle scale, then reject any command outside the allowed electrical envelope. Never clamp an invalid pose into a different pose silently.

Initial per-joint command limits are **60–120°**, nominal reference **90°**, speed **10°/s**. An operator can narrow or adjust joint limits while outputs are released, inside the hard nominal **45–140°** range, which stays within 1000–2000 microseconds. Limits must contain the stored reference. The degree scale is not a measured angle calibration; its nominal 90° is approximately 1472 microseconds.

The latest simple-walk stance includes foot commands **0° and 25°**, approximately **544 and 802 microseconds**, outside this envelope. These values are **historical references only**: do not automatically command/import them. The leg-2 knee value 140° was also specifically reported to have overdriven its joint; do not adopt it as a known standing value.

Teach and capture a fresh standing pose within checked joint limits. If the fitted assembly cannot reach a usable stance within the envelope, stop that stage and refit/re-index the horn with power off, or request a separate range-design change. This version does not silently widen travel to reproduce the old stance. **This range choice is an explicit item for owner approval.**

## Behaviour and state ownership

- Boot/reset: defined LOW application outputs, all twelve detached, motion held. Loading a valid profile never attaches or moves a servo. No startup direction check, standing sequence or automatic walking.
- `resume` explicitly clears hold, without attaching outputs or restarting cancelled targets. It is required before a motion/attachment command after boot, STOP or release.
- Individual calibration/attachment affects only the addressed joint. Already attached joints stay attached; no hidden bulk attach.
- A single controller owns all commanded positions, targets, attachment state and sequence state. No busy-wait motion loops, `delay()` motion scripts, dynamic allocation or Arduino `String` command construction.
- Movement updates run at 20 ms intervals with fractional progress, per-joint bounds and a nominal adjustable 1–20°/s rate. A late loop makes at most one normal update, not a catch-up jump. Same-direction target changes must not continually discard fractional progress.
- Validate a whole multi-joint pose before changing any target. Gait preflight covers the entire requested offset envelope before its first move.
- `stop` freezes current **commanded** angles, cancels all targets/queued sequence stages and latches hold. It does not finish a step or return to standing. This is distinct from the old simple walk's graceful stop.
- `off` or `!` cancels everything, detaches every attached servo and drives its signal LOW with interrupt-safe AVR handling. Release latches hold. It removes holding torque, not supply power; a loaded robot can drop.
- Cancellation during staged arming also cancels remaining attachments. `resume` cannot restart an interrupted arm/gait sequence.
- There is no encoder, contact sensing, balance controller or radio-disconnect feedback. Finite cycles limit autonomous duration. Losing the radio link does not automatically remove holding torque; automatic release on disconnect would risk dropping the body.

While a group pose, `legtest`, staged `armall` or gait is active, accept only `help`, `show`, `stop`, `off` and `!`. Reject every other command with `ERR BUSY`, preserving the existing sequence. This includes `resume`, manual `arm`/`move`/`test`/`cal`, calibration/configuration changes, capture and storage commands. After STOP/release cancels the sequence, its hold rules apply. Group pose/test starts also require the controller to be stationary, so they do not overlap outstanding manual moves. Outside group sequences, bounded manual target updates may replace a manual target, while `zero`/`flip`/`check` require their addressed joint to be stationary.

## Bluetooth commands

Use lowercase ASCII commands, one per newline/CRLF; global joint IDs are 0–11, leg numbers are 1–4. No ambiguous numeric shorthand between these schemes. All commands return a short acknowledgement/error; moving commands produce a completion notice when their commanded motion finishes. Settings can be inspected and changed without recompiling.

| Command | Contract |
|---|---|
| `help`, `show` | Instructions and profile/current state, including the fixed leg/corner/pin map, checked flags, saved-stance validity, hold, attachments and commanded angles. No movement. |
| `resume` | Clear hold only. |
| `cal J` | Attach only joint J at its stored reference, or slowly return that already attached joint to reference. Requires resumed state, not a running group sequence. First attachment can jump from unknown physical position. |
| `arm J ANGLE` | Explicit first attachment at a selected raw angle within that joint's limits. Does not change its reference or certify calibration. |
| `move J ANGLE` | Slowly move an attached joint to a raw angle inside its limits. |
| `zero J` | At rest, record that joint's current command as its reference; clear its checked flag and affected saved-stance validity. |
| `test J OFFSET` | Bounded relative movement from reference using its direction. Recommend +5° and return to 0 during calibration. |
| `flip J`, `check J` | Reverse future positive direction, or record the owner's confirmation of reference/direction/limits at reference. Flip invalidates that joint's checked flag/affected stance. Check is not a sensor measurement. |
| `limits J MIN MAX` | Detached-only limit change; retain the hard envelope. Invalidate that joint's checked flag/affected stance. |
| `speed N` | Nominal 1–20°/s, only when stationary and no sequence is active. |
| `captureleg L`, `capture` | Save the three current commands for a checked/attached/stationary leg, or all twelve for the whole stance. Require resumed state. Four per-leg validity bits avoid treating a partial capture as a whole standing pose. |
| `legstand L`, `legtest L` | Require a valid stance and three checked/attached joints for that leg, with the whole controller stationary. Approach its stance or perform the finite per-leg sequence defined below. Other attached legs hold their current commands. No hidden arming. |
| `armall` | Requires resumed state, all twelve checked, all four stance records valid, all outputs initially released. Attach at the recorded standing commands one joint per 100 ms; operator supports the body. First attachment is still open-loop. |
| `stand` | Approach the captured twelve-joint stance; require all twelve checked/attached, a valid complete stance, resumed state and a stationary controller. |
| `forward [N]`, `back [N]`, `left [N]`, `right [N]` | Default **one cycle**, accept only 1–3. Require the complete checked/attached stance and being stationary at it. Return to standing and hold at completion. No indefinite walk command. |
| `gait SPAN LIFT PERIOD` | Set hip full swing span 1–20°, knee lift 1–10°, nominal cycle period 8000–30000 ms; only stationary, with no sequence active. Default 10°, 5°, 8000 ms. Persist these bounded values. Preflight every subsequent gait against the current profile. |
| `stop`, `off`, `!` | Safety behaviour above; `!` requires no newline. Clear queued/partial parsed input and cancel the current reply. Send a newline after `!` before another command. |
| `save`, `load` | Released-only profile storage/load; neither resumes, attaches nor moves anything. |

Positive direction during this joint-space test means **hip swings towards the chosen robot front**, **knee lifts the foot**, and **foot folds towards its body mount**. These definitions must be physically checked per joint. Hip direction is deliberately different from the single-leg IK reference's generic counter-clockwise yaw; do not automatically import old direction flags.

Reject malformed, non-finite, missing/extra and out-of-range complete commands without changing targets/attachments. Unknown commands and incompatible-state requests also return an error without changing an active sequence. Rejected input never arms a servo. A single UART line buffer with a bounded pending command is sufficient. Transport/line-integrity errors (non-ASCII, overlong/overflowed or stale partial input, or pending-command overflow) cancel motion and latch hold, discarding input through newline; partial-line timeout is two seconds. Do not support pasted movement batches/repeating macros. Bytes already in a transport or in flight cannot be recalled.

Long `show`/`help` replies must service input and the controller between output bytes, so STOP/`!` remain actionable. Errors/text live in flash where possible to meet the Uno RAM budget. Replies never depend on an HC-05 connection being present.

## Finite creep behaviour

`legtest L` first approaches that leg's captured stance, lifts its knee by the configured lift, swings its hip to +half-span and then -half-span, returns its hip to stance, and finally lowers its knee to stance. Its foot stays at the captured standing command. Apply the leg's checked direction signs and preflight every target before starting. Use the configured slew rate throughout; finish once and hold. This makes the individual bench test predictable without assuming foot-space geometry.

Use the newer simple sketch's standing-relative joint-space idea, with the confirmed crawl order **3 → 1 → 4 → 2**: rear-left, front-left, rear-right, front-right. Left/right side assignments are fixed from the owner's map. Positive hip-forward and knee-up signs come from checked calibration, not guessed mirror signs.

Hip targets oscillate smoothly within half of the configured full swing span around each saved standing angle. Only the selected leg receives a positive knee-lift command during its swing slot; its foot servo holds its saved standing angle. Leave an all-knees-at-stance interval between successive lift slots. Ramp the stride and lift amplitudes in and out so the first/last cycle starts/ends in the captured stance. Backward reverses hip travel; a turn drives the left/right sides in opposing directions.

Phase advancement must respect joint slew limits: if necessary stretch the cycle rather than exceed the rate or advance to another leg before the previous knee has returned. The configured period is nominal/minimum, not a promise of wall-clock duration under every profile. Reject an unsafe full offset envelope before starting. A non-finite/invalid runtime calculation holds motion; do not silently skip/clamp an affected leg.

This version deliberately does not add guessed body lean or claim stability from the old IK simulation. Begin with the body supported/tethered. A joint-space creep may require later lean/geometry work before independent loaded walking. At most one **commanded** knee-lift stage is active; actual contact/foot height cannot be verified by software alone.

## Storage and hardware integration

Use a distinct versioned, explicit byte encoding with CRC and semantic validation. Reserve a maximum **192-byte record at EEPROM 256–447**, with bounds/static-size checks. This is separate from browser calibration at 0–125, old walking trims at 512+, and the single-leg record at 768–799. Include twelve references/limits/directions/checked flags, twelve standing angles, stance-validity mask and gait settings. Fixed numbering/pins are firmware constants. Write the commit marker last; an interrupted save can invalidate the previous record. A two-bank transactional store is outside this version.

Do not automatically import old EEPROM layouts, legacy stance constants or a one-leg profile into a twelve-joint calibrated profile. Retain old records untouched. Boot accepts only a complete valid new record, otherwise uses detached/unconfirmed defaults.

COM wiring remains HC-05 TXD to the shield net connected to Uno D0/RX; Uno D1/TX through the 1 kΩ/2 kΩ divider to HC-05 RXD; common ground; COM + only after verifying voltage against the module's rating. Check clone header routing rather than assuming printed RX/TX direction. Disconnect both module signal wires for upload or USB debugging; USB and Bluetooth are one shared UART, not independent inputs. No software serial dependency.

D13 remains the leg-4 foot signal; keep servo power off through bootloader/reset/upload. Do not use the onboard LED as a status output. Supply/load checks are physical prerequisites: a previously mentioned 5 V/3 A regulator is not established as sufficient for twelve servos. Do not power the servo bank from Uno USB or assume a supply upgrade has happened.

## New files, resource limits and verification

New standalone folder: `firmware/spyder_four_leg_test/`, including `.ino`, configuration/map, a hardware-independent controller/gait, a console/storage module as useful, and README. Keep the extracted Arduino folder self-contained; header-declare types needed by Arduino IDE-generated prototypes. New build/native/AVR test helpers live under distinct names and `tests/four_leg_test/`; leave all existing code and tests untouched.

Implementation choices such as splitting the parser/controller files and fixed-point/frame representation are left to the builder. Preserve the observable contracts above. Target Uno flash ≤32,256 bytes and static RAM ≤1,536 bytes, with at least 512 bytes reserved for runtime/stack; verify actual build and relevant simulator stack behaviour. If these targets cannot be met, report the measured constraint before changing board, dropping functions or weakening checks.

Acceptance evidence before delivery:

1. Native sanitizer tests: correct twelve-pin/corner map; no boot attach; individual isolation; direction/limits/stance flags; atomic pose rejection; slew/fractional/wrap timing; staged arming; finite completion; safe cancellation in every stage; strict/overflow/stale input; CRC/semantic corruption and EEPROM isolation.
2. Gait tests for all four directions: correct order/sides, one commanded lift slot, inter-slot stance interval, all targets in limits, checked direction signs, bounded cycles, rate-capped phase progress, and return to the recorded stance. Do not call this a real balance test.
3. Actual Uno compilation with Servo 1.2.2 and Arduino AVR core, including EEPROM/library boundaries and Arduino sketch type/prototype compatibility. No application warnings hidden.
4. Actual-ELF UART/GPIO simulation: all twelve expected outputs, no servo pulses on D0/D1, protocol replies during motion, STOP/release during long replies/arming/gait, LOW signal termination and no abnormally long HIGH pulse. Include all twelve channels near the upper allowed pulse limit: Servo's sequential refresh frame can exceed 20 ms in that case; do not confuse the controller's 20 ms update interval with a guaranteed 50 Hz twelve-channel PWM frame. Serial load must not cause catch-up motion.
5. Relevant existing checks run without editing them; distinguish unrelated failures from the new work. Independent code review of the finished candidate.
6. A fresh-folder Arduino ZIP containing all sketch files, a quick start, pin/command tables, verification scope and checksums. Physical Bluetooth pairing, D6 header function, alignment, binding, power, standing and loaded walking remain explicitly unverified.

Physical test order: verify Bluetooth while all servo power is off → individual joints, especially D6 → individual supported legs → all legs with the body suspended → captured stance with staged support → one finite creep cycle with support/tether. Report `show`, direction/limit changes, supply observations and any binding before progression.

Keep local candidates separate from historical downloads. Do not upload firmware or move motors from the cloud. Publish only when the owner asks after testing; use a feature branch and PR, never push this directly to `main`. Preserve the existing uncommitted single-leg files.

# ESP32/PCA9685 single-servo candidate — verification

Historical verification snapshot. For current publication status, retained tests and source layout, see [project state](../project-state.md) and [publication checks](2026-10-28-esp32-publication.md).
27 October 2026. Local branch: `feat/esp32-pca9685-servo-test`.

Implementation: `firmware/spyder_esp32_servo_test/`. The owner approved the [design](../superpowers/specs/2026-10-27-esp32-pca9685-servo-test-design.md). Existing Uno source, web console and previous local one-leg work were left intact. No firmware upload, physical motor movement, merge or GitHub push was performed.

## Fresh software evidence

| Check | Result |
|---|---|
| `bash scripts/test-esp32-servo.sh` | Passed all six reported groups under AddressSanitizer and UndefinedBehaviorSanitizer, with `-Wall -Wextra -Werror`. |
| `bash scripts/build-esp32-servo.sh` | Original ESP32 Arduino-framework compile/link succeeded. Flash **1,123,221 / 1,310,720 bytes**; static RAM **40,356 / 327,680 bytes**. This is not a runtime heap/stack measurement. |
| `npm test` | Existing console suite: **39 passed, zero failures**. |
| Whitespace/diff checks | Passed for tracked changes and all new source/test/document files. |
| Independent code review | One fresh reviewer found missing UART/Bluetooth transport-overflow handling; fixed with regression evidence. No other material source findings reported on the reviewed snapshot. No second review of the fix claimed. |

Verified build versions: PlatformIO Core 6.1.18, espressif32 platform 6.12.0, Arduino ESP32 **2.0.17**, Xtensa GCC 8.4.0, Adafruit PWM Servo Driver **3.0.2**, Adafruit BusIO **1.17.4**. The PlatformIO build compiles the actual `.ino` and companion sources with the real ESP32 Arduino framework. Arduino CLI's index download failed in this environment before compilation; a GUI Arduino IDE session was not run. The README specifies the verified core/library versions for Arduino IDE.

## What the tests establish

- Released boot state, explicit arming at 1500 us, strict integer commands/ranges, degree-to-pulse mapping and gradual 50 us/s motion.
- Late updates take one step, including across the millisecond timer wrap. STOP cancels the target and remaining sequence; release requires fresh arming.
- Two-second 1300/1500/1700 tests release afterwards; one finite sweep reaches both nominal endpoints and returns to centre without repeating.
- Separate USB/Bluetooth lines; urgent release; partial/overlong/invalid-line handling; disconnect cancellation; fresh Bluetooth session input reset.
- Checked PCA9685 configuration/write handling, nominal inward endpoint rounding, channel-0 isolation and fault latching.
- A deterministic disconnect injected between pulse preload and OE activation prevents activation and permits normal later re-arming.
- The actual sketch runs through host-supplied UART/radio/I2C/GPIO/scheduler boundaries to check source-specific replies, Bluetooth overflow, UART errors, disconnect/rearm and periodic I2C faults. Its main loop hands replies to a separate worker rather than writing transport replies itself.

The review identified that stock BluetoothSerial silently drops bytes after its 512-byte RX queue fills. A regression packet containing 512 newline bytes followed by `!` initially left motion running. The sketch now uses `onData` with an application-owned bounded queue and explicit overflow detection; overflow holds motion and discards input. USB receive errors are registered through `Serial.onReceiveError` and follow the same hold path. The actual-sketch checks passed after the fix. A temporary build omitting UART error registration independently failed the expected hold assertion, confirming that regression check detects the missing handler.

## Decisions and remaining limits

- Executed inline in the existing dedicated cloud checkout and feature branch after explicit design/implementation approval; no further plan/worktree approval was requested. This trades a separate filesystem worktree and per-task reviewers for one final independent review; the old files remain untouched.
- Added an interrupted-enable result to distinguish Bluetooth cancellation from I2C failure. A disconnect at the arming boundary releases without falsely requiring a reset; genuine I2C failures remain latched. Error misclassification would affect recovery, so both paths are checked.
- Clarified the approved OE pull-up wiring for boards with an onboard pull-down. **10 kΩ up against 10 kΩ down produces about 1.65 V and is insufficient.** With a confirmed 10 kΩ board pull-down, a 1 kΩ external pull-up gives about 3.0 V idle and draws about 3.3 mA when GPIO drives LOW. Identify the actual board circuit and verify idle OE near 3.3 V before applying servo power. Incorrect board identification could leave reset inhibition unreliable.
- The review explicitly left physical OE voltage/reset behavior, PCA9685 retained-register behavior, oscillator accuracy, radio pairing/loss timing, supply adequacy and mechanical motion unverified. Boundary doubles and deterministic injection do not reproduce real electrical behavior, FreeRTOS scheduling or congested radio timing. Incorrect assumptions can affect the real assembly; an unloaded hardware test remains necessary.

No minor review findings were deferred. Preserve the local candidate for the owner's physical test before publication.

## Owner's first checks

With servo power off, inspect wiring and OE bias, upload using the documented Arduino IDE configuration, and check `show` reports OFF. Test USB and Bluetooth commands with no motor powered. Then connect one unloaded motor to channel 0 and test `arm`, a small command such as `angle 95`, `center` and `off`, one command at a time. Verify release on Bluetooth disconnect. Only then consider the two-second pulse tests and one finite sweep. Stop and remove power if the motor binds, buzzes or slips. Requested pulses/nominal degrees are not measured shaft positions.

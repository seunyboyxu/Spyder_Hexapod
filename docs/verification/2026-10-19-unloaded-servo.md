# Revision 3 unloaded-servo verification — 19 October 2026

Revision 3 adds nominal individual centering and an isolated one-pass range test. The owner identified SG90 (9 g) and some MG90D motors, then chose the existing **1000–2000 µs defaults** with **1500 µs nominal center** because no datasheets are available. No wider endpoint or measured angle claim is made.

## Fresh verification

| Check | Result |
|---|---|
| `bash scripts/test-native.sh` | All five ASan/UBSan executables passed, including new centering/sweep sequencing, exclusive output, unchanged leg limits, blocked normal commands and STOP/watchdog/reset cases. |
| `npm test` | 36 passed, none skipped. Includes demo timing/lease behavior, malformed command/snapshot validation and new commands through the actual native C++ firmware and Web Serial adapter. |
| `npm run test:browser` | 14 Chromium workflows passed: all ten earlier cases plus unloaded confirmation/Resume gates, selected nominal center, limits/progress/STOP/recenter and recovery of the selected bench joint on reconnect. |
| `python3 scripts/build-avr.py` | Actual ATmega328P image built: **12,494 bytes flash, 1,041 bytes static RAM**, leaving 1,007 bytes for stack/runtime. Application compiled without warnings; the previously documented upstream Arduino-core warnings remain. |
| `python3 scripts/test-avr.py --simavr-root /workspace/Spyder-audit/tools` | **25 actual-ELF scenarios passed.** Includes the earlier 21 release phases LOW, bounded re-arm, twelve steady outputs and serial ramp at 1555 µs. New waveform test reaches 1000 and 2000 µs within ±15 µs, attaches only D6 (joint 4) during the isolated test, completes one cycle, returns to 1500 µs and releases LOW. |
| Independent read-only code review | No must-fix issues. Reviewer independently ran all five native sanitizer executables and all 36 JavaScript tests. Reviewed firmware/session isolation, bounds, cancellation, preserved settings, SPYDER 3 readback validation and UI recovery. |
| UI inspection | Desktop and 390 px mobile layouts inspected; the new panel fits and STOP/Release stay visible. |

New native, protocol, demo and UI checks demonstrated missing behavior before their production implementations. A reconnect regression demonstrated a locked selector pointing at the wrong joint before the selection-sync fix. The original calibration safety regressions remain present and passing. No hardware was flashed or moved.

## Compatibility and scope

- Update the **whole console and Uno sketch together**: handshake and boot are `SPYDER 3`. Old firmware is rejected; EEPROM/JSON schema remains version 1.
- D2–D13 stay unchanged. Keep servo power off during connection/reset/upload, retain the external servo supply/common ground/verified shield-isolation procedure, and secure the unloaded motor.
- The hardware adapter's signal limits match the test envelope; ordinary assembled-joint bounds remain separate and are never widened by a bench test.
- Centering clears the selected travel-check flag and standing record in RAM, preserves min/max/initial/direction and never automatically saves EEPROM. Release exits the mode. Recheck the assembled leg after refitting.
- SG90/MG90D manufacturer pages were inaccessible from this environment. The owner subsequently selected default pulses. Model-specific full-travel specifications and actual per-pin model assignments are not inferred.
- simavr runs the actual application image at an ideal 5 V/16 MHz. It does not model the user's bootloader, supply dips, motor internals, actual angle, mechanical stops, horn slip or balance. A nominal 90° command is not a measured right angle.

The prior [revision 2 record](../verification.md) and [repository handover verification](2026-10-18-handover.md) remain historical evidence. The [unloaded instructions](../unloaded-servo-calibration.md) describe operation; no claim of physical calibration, full angular travel or stable standing is made.

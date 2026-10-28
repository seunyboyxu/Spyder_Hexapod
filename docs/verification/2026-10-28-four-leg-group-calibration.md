# Four-leg BLE group calibration verification — 28 October 2026

Scope: firmware/spyder_esp32_ble_four_leg_test v1.1. Extends only the four-leg candidate; previous one-leg/Uno sketches and the BLE String compatibility fix are preserved. This report describes checks before the owner requested GitHub publication. See [publication checks](2026-10-28-esp32-publication.md) for the current repository state. Nothing was uploaded or physically moved.

## New behavior

- `leg all` broadcasts cal/move/test/zero/flip/check/limits to the four matching joints. Each keeps its own reference, direction, limits and flags.
- All recipient commands validate against full state copies with silent outputs before any live state/output change. A late rejection cannot partially move or confirm earlier legs.
- Group moves use one tick origin and block calibration/selection changes until all legs finish, or STOP/OFF cancels.
- `cal all`, in all mode and with every output off, stages unchecked calibration activation at individual references, one joint every100ms. It does not set checked flags. Existing `armall` still requires all12 checked.
- `check all` explicitly confirms all12 only when attached, stationary and at individual references. It is a human assertion, not measured calibration.
- STOP cancels remaining staged attachments and holds active joints. OFF, urgent ! and BLE unsubscribe/disconnect release all twelve; no replay on reconnect.
- Wiring, pulse envelope, NVS format/namespace and channels12–15 off are unchanged. Status now identifies v1.1.

## Evidence

The new controller test was first run against the previous implementation. It failed at `cal 0` in leg-all mode, confirming the missing behavior. After implementation, the new batch suite passed with AddressSanitizer/UndefinedBehaviorSanitizer.

`bash scripts/test-esp32-four-leg.sh`: passed controller, new batch calibration, twelve-channel PCA adapter, five existing leg regressions and actual BLE sketch checks. Includes per-leg reference/direction preservation, late-recipient atomic rejection, invalid input, shared movement completion, explicit check-all, pre-first and mid-stage cancellation, unchanged checked arming, and notification loss during unchecked arming.

`bash scripts/test-esp32-ble-compat.sh`: passed both one-leg/four-leg transports against the Arduino String API model, including subscription epochs and bounded callbacks.

`npm test`: 39 tests passed, zero failures.

Actual original ESP32 Dev Module compilation, without upload:

| Toolchain | Flash bytes / 1,310,720 | Static/global RAM bytes / 327,680 | Result |
|---|---:|---:|---|
| PlatformIO espressif32 6.12.0, Arduino ESP32 2.0.17 | 1,171,233 | 40,028 | PASS |
| Arduino CLI, Arduino ESP32 3.3.2 | 1,132,975 | 41,448 | PASS |

Dependencies: Adafruit PWM Servo Driver Library3.0.2, Adafruit BusIO1.17.4. Build logs are local ignored files `.tools/esp32-four-leg-group-core2.log` and `.tools/esp32/group-core3.log`. Static/global RAM figures do not include runtime BLE heap/stack requirements.

An independent read-only reviewer inspected group atomicity, silent output redirection, staged interruption, per-leg settings and instructions, and freshly ran the sanitizer suite: no material findings.

## Remaining physical checks

The user's PP3 9V battery feeding a5V/3A buck is inadequate for powered multi-servo testing. Replace the source/converter/distribution with equipment sized for combined starting/stall current before using these group commands with servo power applied.

No real ESP32, PCA9685, servos or iPhone are attached here. BLE radio timing, OE reset bias, supply adequacy, measured pulses, mechanical clearance/directions and shaft arrival remain unverified. First attachment can jump from an unknown physical position. Support the body: simultaneous step lifts all feet and is not a balanced gait. Confirm every joint physically before `check all`; use single-leg overrides where mirrored geometry differs.

# Current project state — 28 October 2026

Current electronics: original ESP32 development board, PCA9685 at0x40, SG90/MG90D positional servos, four legs with three joints each. SDA21/SCL22/OE23; consecutive hip/knee/foot channel triples0–11. Intended physical corners are front-left1, front-right2, rear-left3, rear-right4; orientation and direction signs still need physical verification. The six-leg CAD project name does not imply six-leg firmware exists.

## Implemented

- Four independently uploadable ESP32 sketches: unloaded single motor with Classic SPP, one leg with Classic SPP, one leg with BLE, and current four-leg BLE v1.1. See [sketch selection and functions](firmware-guide.md).
- Explicit arming, default-off startup/load/reconnect, bounded movement, checked I2C writes and OE inhibition, global hold/release, independent USB/radio command buffers and fault handling.
- Per-joint references, directions, limits, human check flags and Preferences storage for leg sketches. Supported stand, partial extension and finite local step demonstrations.
- Four-leg group calibration with atomic preflight, staged unchecked `cal all`, explicit `check all`, checked `armall`, individual overrides and shared step-phase barriers.
- BLE core2/core3 `String` compatibility fix included in both BLE sketches; Nordic UART service for iPhone apps.
- Seven selected STLs, two native Fusion archives, part mapping and exact checksums in [cad/](../cad/README.md).

## Verification and limitations

Host controller/input/adapter/sketch tests and actual original-ESP32 builds are documented in the [verification index](README.md). Compiled builds do not establish mechanical motion, real iPhone/radio operation, reset/OE voltage, pulse accuracy or supply integrity. These require owner bench results. No hardware was uploaded or actuated by the cloud agent.

The confirmed PP3 9V battery and5V/3A buck are inadequate for multi-servo testing. Replace and verify source/converter/distribution before powered group tests. Geometry27.5/55/74mm is estimated; directions, references, safe travel and load capacity are not measured here.

All-four step tests lift feet together and require body support. Balanced walking, body shift, terrain/contact feedback, six-leg gait and phone app development are future work. Servos provide no position telemetry; Ready/check flags refer to completed command schedules and human observations.

## Repository transition

The owner requested publication of the ESP32 work and removal of legacy board code on28 October2026. The active tree now contains ESP32 firmware only; legacy Uno/Sensor Shield sketches, browser console, AVR tooling and their specific docs/tests have been removed. Git history preserves those previous versions. Arduino IDE/framework support remains because it is the ESP32 toolchain. The earlier pause on GitHub publication is superseded by the owner's explicit request; physical-test limitations remain.

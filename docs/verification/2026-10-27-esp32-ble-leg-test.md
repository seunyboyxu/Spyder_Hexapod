# iPhone BLE one-leg candidate — verification

Historical verification snapshot. For current publication status, retained tests and source layout, see [project state](../project-state.md) and [publication checks](2026-10-28-esp32-publication.md).
27 October 2026. Local candidate `firmware/spyder_esp32_ble_leg_test/`; no hardware upload, motor movement or GitHub push performed.

## Problem and change

The previous sketch implements Bluetooth Classic SPP through BluetoothSerial. Ordinary iPhone serial apps cannot use that profile. This separate candidate implements the Nordic UART Service over BLE, advertising **Spyder-Leg-BLE**, with RX Write/Write Without Response and TX notifications. Connect inside a compatible BLE app, not iOS Bluetooth Settings. The included README covers Bluefruit Connect and generic nRF Connect characteristic/hex-write instructions.

Channels0/1/2, GPIO21 SDA/GPIO22 SCL, GPIO23 OE, joint calibration/geometry, stored settings and USB115200 remain the same as the previous leg candidate. All nine copied controller/parser/PCA/storage/safety files are byte-for-byte unchanged. Existing candidates/downloads are retained.

## Fresh software evidence

| Check | Result |
|---|---|
| `bash scripts/test-esp32-ble-leg.sh` | **Eleven reported checks/groups passed**, ASan/UBSan, compiler warnings treated as errors. Includes two concurrent transport regressions. |
| `bash scripts/build-esp32-ble-leg.sh` | Actual original ESP32 Arduino-framework compile/link succeeded; flash **1,166,869 / 1,310,720 bytes**; static RAM **39,796 / 327,680 bytes**. |
| `npm test` | Existing console suite: **39 passed**, no failures. |
| Independent review | Found a stale-reply subscription race; addressed with a failing-then-passing deterministic interleaving test. Review ran the original nine-group suite and checked the copied core. No second review of fixes claimed. |
| Package | ZIP CRC, source equivalence and SHA256 manifest checked at delivery. |

Verified dependencies: ESP32 Arduino **2.0.17**, bundled BLE library, Adafruit PWM Servo Driver **3.0.2**, Adafruit BusIO **1.17.4**, PlatformIO Core6.1.18/espressif32 platform6.12.0. No external BLE library is needed. The real Arduino framework and sketch are compiled via PlatformIO; the Arduino IDE GUI was not run. Static RAM totals do not measure Bluetooth heap/task-stack use.

## Coverage and review fix

The reused leg tests cover reference/direction calibration, bounds, IK with independent forward kinematics, complete-path preflight, supported standing/partial extension, finite cycles, STOP/OFF, fractional speed, timer wrap and calibration packet validation. Adapter/sketch checks cover output isolation, all-off boot including valid stored calibration, release-only save/load, corruption/failure rejection, UART/BLE RX errors, source-specific replies, disconnect/attach races and latched I2C faults.

BLE checks exercise NUS UUID/properties, subscription gating, fragmented command reception, default-MTU20-byte chunks, advertising restart, stale-session rejection, notification-error handling and immediate release on disconnect/unsubscription. Wireless movement/configuration commands require notification subscription; safety release remains available without it. Reconnecting/subscribing never activates output.

The reviewer found that an epoch check before `setValue`/`notify` could allow an old reply chunk to cross a rapid unsubscribe/resubscribe transition. A deterministic two-thread host test paused dispatch after the guard, switched subscriptions, then resumed it; the original code failed. Dispatch/session synchronization and publication ordering make the check pass. Disable/disconnect callbacks inhibit OE before waiting.

Further source inspection of the bundled IDF/Bluedroid API showed notification dispatch can wait for the Bluetooth task's queue. An unlimited wait in that task's session callback could deadlock under congestion. A second concurrent regression held notification dispatch while switching subscriptions and failed against an unlimited callback wait. Session callbacks now use a short bounded wait and fail closed on a busy subscription; the test passes. The app can retry enabling notifications once the send clears. Reply pacing remains in the worker, not the motor loop. Packets already accepted by the stack or in flight cannot be recalled.

## Physical and runtime limits

No iPhone, physical ESP32, PCA9685 or motors are connected to this cloud workspace. Host doubles model BLE/GPIO/I2C/UART/scheduler boundaries; they do not run the complete FreeRTOS reply worker or reproduce CoreBluetooth interoperability, radio timing, SDK congestion, oscillator tolerance, OE electrical bias/reset, supply integrity or mechanics. The concurrent checks use host threads, not the real Bluetooth task. **Actual iPhone connection and hardware behavior remain unverified.**

Use the README's power/OE and supported-leg calibration procedure. Initially upload with servo power off, connect to Spyder-Leg-BLE inside the app, enable TX notifications, then send help/show with newline and verify OFF status. Only then power the externally supplied supported leg and perform individual calibration before poses/steps. Existing saved calibration uses the same namespace and angle scale; boot loading never attaches outputs. The owner reports results before publication.

# Four-leg BLE test and String API correction

Historical verification snapshot. For current publication status, retained tests and source layout, see [project state](../project-state.md) and [publication checks](2026-10-28-esp32-publication.md).
27 October 2026. Local candidate `firmware/spyder_esp32_ble_four_leg_test/` on the existing ESP32 feature branch. No firmware upload, physical motor movement or GitHub push performed. Prior candidates/downloads remain available.

## Delivered behavior

Leg1 uses PCA9685 channels0/1/2, leg2 uses3/4/5, leg3 uses6/7/8 and leg4 uses9/10/11. Joint order is hip, knee/thigh, foot. Channels12–15 stay off. Each leg has independent references/directions/limits/check flags. Select `leg 1..4` for calibration or `leg all` for coordinated standing, partial straightening and finite1–3 step cycles. Global STOP/OFF/disconnect acts on every leg.

Group motion preflights all12 joints and every waypoint before changing targets. Group steps wait for every leg at a waypoint before advancing; faster legs wait for slower ones. This is simultaneous supported testing, not a stable walking gait. Explicit checked `armall` activates one reference joint per100ms and is cancellable. Stored calibrations use a distinct134-byte whole-record/per-leg CRC format and Preferences namespace `spyder-four`; loading never activates outputs.

The reported `String` to `std::string` conversion error was reproduced with a newer-API model. Both BLE variants now use `const auto value=characteristic->getValue()` and the shared `c_str()/length()` API, preserving the complete received payload for either return type. The previous one-leg candidate is also refreshed as v1.1.

## Fresh verification

| Check | Result |
|---|---|
| `bash scripts/test-esp32-four-leg.sh` | Passed all8 reported checks/groups under AddressSanitizer/UndefinedBehaviorSanitizer and `-Wall -Wextra -Werror`. |
| `bash scripts/test-esp32-ble-compat.sh` | Passed the BLE transport and concurrent/session checks against an Arduino String return-type model for both one-leg and four-leg adapters; the original code failed with the same conversion error. |
| One-leg BLE regression suite | All11 reported checks/groups passed after the compatibility correction. |
| `npm test` | Existing console suite39/39 passed, no failures. |
| Independent read-only review | No material findings. Reviewer freshly ran four-leg/compat/npm checks and the core2 build; inspected the core3 build log. |
| Diff/new-file checks and package | Whitespace checked; delivery ZIP CRC, source equivalence and SHA256 manifests checked. |

Actual original-ESP32 build results:

| Sketch | ESP32 Arduino core | Build tool | Flash bytes | Static/global RAM bytes |
|---|---|---|---:|---:|
| Four-leg | **2.0.17** | PlatformIO/espressif32 6.12.0 | **1,170,517** | **40,028** |
| Four-leg | **3.3.2** | Arduino CLI, `esp32:esp32:esp32` | **1,132,283** | **41,448** |
| Patched one-leg | **2.0.17** | PlatformIO/espressif32 6.12.0 | **1,166,869** | **39,796** |
| Patched one-leg | **3.3.2** | Arduino CLI, `esp32:esp32:esp32` | **1,128,563** | **41,216** |

All fit the default1,310,720-byte application partition. Adafruit PWM Servo Driver3.0.2 and BusIO1.17.4 were used. Arduino CLI performs the sketch preprocessing/build also used by Arduino IDE; the GUI itself was not run. Compile-time RAM figures do not measure live BLE heap or task-stack use.

The core3 compile command, after installing that core and the documented libraries, was:

```text
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/spyder_esp32_ble_four_leg_test
```

This workspace additionally supplied its local configuration, pinned library search directory and isolated build directory. Full logs are retained in `.tools/esp32-four-leg-build.log`, `.tools/esp32/compile-four-core3.log`, `.tools/esp32/compile-one-core3.log` and `.tools/esp32/one-leg-build-after-compat.log`.

## Checks that exercise the new boundaries

- Twelve-channel mapping, including channels8–11 and a16-bit active mask; independent detach and reserved-output isolation.
- Pure preflight rejects a fourth-leg limit violation before any target/output changes.
- Unequal-speed group stepping: faster legs do not lift into the next waypoint while the slow leg is still swinging.
- Complete finite cycles, global held STOP, cancelled staged activation, no late attachments and no catch-up activation after a delayed loop.
- Fourth-leg/global calibration record round trip and every individual bit corruption across the134-byte record.
- Actual-sketch host harness: valid saved boot remains all-off; BLE selection/notification gating; staged armall and pre-first-attachment save rejection; group poses/steps;13-line show-all response without falsely overflowing its32-job reply queue; global stop/release; saved/calibration validation; RX overflow/UART errors; disconnect/unsubscription during staging; I2C all-off fault latching.
- Earlier leg geometry/calibration/timing/codec regression checks run against the new controller copy; default single-leg behavior remains covered. The new synchronization hooks are confined to this new folder.

## Confirmed power limitation and physical limits

The owner confirms a **rectangular PP3 9V battery and a5V/3A buck converter**. **This is not a suitable source for the simultaneous twelve-servo tests.** The converter is limited to15W and twelve servo peaks can exceed3A; a PP3 cannot sustain even the input current needed for that full converter output. Staging the first activations does not solve the later simultaneous load. Replace this supply before powered multi-servo tests, and verify the replacement source, distribution, wiring and PCA9685 terminal/rail ratings against combined servo demand.

The user must perform physical tests with appropriate power, verified OE pull-up, each joint aligned/direction-checked and the body externally supported. No cloud result proves real BLE/iPhone operation, reset/OE voltage, oscillator accuracy, supply performance, mechanical clearance or balance. Native doubles do not run the whole FreeRTOS reply worker or the physical bus/radio; concurrent BLE checks use host threads. Actual pulse updates occur over a sequential I2C bus; the implementation coordinates commanded phases, not measured simultaneous physical arrival or contact.

The supplied String fix is verified in real core2/core3 builds. Phone connection, mechanics and safe powered operation remain for the owner to test. No publication before that feedback.

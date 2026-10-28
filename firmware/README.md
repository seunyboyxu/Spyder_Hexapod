# ESP32 sketch index

Every subfolder here is a separate sketch for an original ESP32 and PCA9685. Open the `.ino` with the same name as its folder in Arduino IDE. Keep all of that folder's `.h/.cpp` files beside it. Upload only one sketch to the board at a time.

| Folder | Role | Commands and instructions |
|---|---|---|
| `spyder_esp32_ble_walk` | **Latest walking prototype**, iPhone BLE; shares twelve-joint calibration and adds modeled crawl/turning | [README](spyder_esp32_ble_walk/README.md) |
| `spyder_esp32_ble_four_leg_test` | **Four-leg calibration controller, v1.1, iPhone BLE**;12 outputs, batch/individual calibration, staged arming, supported poses/finite steps | [README](spyder_esp32_ble_four_leg_test/README.md) |
| `spyder_esp32_ble_leg_test` | One-leg BLE isolation test;3 outputs; core2/core3 String fix included | [README](spyder_esp32_ble_leg_test/README.md) |
| `spyder_esp32_leg_test` | One-leg Bluetooth Classic alternative;3 outputs; Android/Windows SPP terminal | [README](spyder_esp32_leg_test/README.md) |
| `spyder_esp32_servo_test` | One unloaded motor;channel0; Classic SPP or USB; bounded pulse/angle checks and sweep | [README](spyder_esp32_servo_test/README.md) |

All use GPIO21 SDA/GPIO22 SCL/GPIO23 OE, PCA9685 address0x40 and USB115200/Newline. All outputs start off. BLE names are `Spyder-Walk-BLE`, `Spyder-4Leg-BLE` and `Spyder-Leg-BLE`; Classic names are `Spyder-Leg-Test` and `Spyder-Servo-Test`.

[Function and command guide](../docs/firmware-guide.md) · [Hardware](../docs/hardware.md) · [Upload](../docs/bring-up.md) · [Development](../docs/development.md).

`PlatformioEntry.cpp` is only the root PlatformIO build selector, not an uploadable sketch. Arduino IDE users open one of the five folders directly.

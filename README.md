# Spyder — ESP32 + PCA9685 robot tests

Spyder is a six-leg mechanical robot project currently developing with **four legs and twelve positional servos**, an **original ESP32** and a **PCA9685**. The current controller uses **BLE for iPhone commands**, with USB serial available for setup and diagnostics.

**For Ryan's latest walking controller, use [spyder_esp32_ble_walk](firmware/spyder_esp32_ble_walk/README.md).** It adds a modeled crawl gait; real balance and hardware behavior remain unverified. For initial calibration, **start with the [four-leg BLE sketch](firmware/spyder_esp32_ble_four_leg_test/README.md).** Version 1.1 supports group arming/calibration, individual corrections, supported standing, partial straightening and finite simultaneous step tests. These step tests lift all four feet together: support the body. Physical validation of the new crawl gait and six-leg control remain future work.

## Choose a sketch

Upload one sketch at a time. Download/clone the whole repository, open the matching `.ino`, and keep its companion files together. All five target ESP32; Arduino IDE is the upload tool.

| Sketch folder | Purpose | Wireless connection | PCA channels |
|---|---|---|---|
| [spyder_esp32_ble_walk](firmware/spyder_esp32_ble_walk/README.md) | **Latest walking prototype:** shares four-leg calibration; forward/reverse/turning, finite cycles and halt | **iPhone-compatible BLE**, `Spyder-Walk-BLE` | 0–11 |
| [spyder_esp32_ble_four_leg_test](firmware/spyder_esp32_ble_four_leg_test/README.md) | **Calibration/test controller:** twelve-joint calibration, all-leg arming/checks, standing, partial straightening and finite simultaneous steps | **iPhone-compatible BLE**, `Spyder-4Leg-BLE` | 0–11 |
| [spyder_esp32_ble_leg_test](firmware/spyder_esp32_ble_leg_test/README.md) | Isolate one leg for alignment, direction checks and the same supported leg motions | **iPhone-compatible BLE**, `Spyder-Leg-BLE` | 0–2 |
| [spyder_esp32_leg_test](firmware/spyder_esp32_leg_test/README.md) | Alternative one-leg controller for a Classic SPP terminal | Classic Bluetooth, `Spyder-Leg-Test`; Android/Windows | 0–2 |
| [spyder_esp32_servo_test](firmware/spyder_esp32_servo_test/README.md) | One unloaded motor: small moves, center, timed pulse tests and one bounded sweep | Classic Bluetooth, `Spyder-Servo-Test`; Android/Windows | 0 |

See the [firmware/function guide](docs/firmware-guide.md) for command tables, important source functions and differences between the sketches. Classic SPP versions do not connect through ordinary iPhone serial apps; use a BLE version for iPhone.

## Wiring and upload

- ESP32 GPIO21 → SDA, GPIO22 → SCL, GPIO23 → OE.
- ESP32 3V3 → PCA9685 VCC. Common GND; external regulated 5 V → servo V+.
- OE needs a verified pull-up to 3V3 to inhibit outputs during reset; see the [hardware guide](docs/hardware.md).
- Arduino IDE: **ESP32 Dev Module**, **esp32 by Espressif Systems**, **Adafruit PWM Servo Driver Library 3.0.2** and **Adafruit BusIO 1.17.4**. USB Serial Monitor: **115200, Newline**. See [bring-up](docs/bring-up.md) for exact versions and steps.

The confirmed **PP3 9 V battery with a 5 V/3 A buck is inadequate for powered multi-servo tests**. Replace the source and distribution with equipment sized for combined starting/stall current. The PCA9685 distributes external servo power; it does not regulate power for the ESP32. Keep servo power off during wiring/upload/reset.

## Four-leg workflow

Legs use consecutive hip/knee/foot triples: leg1=0/1/2, leg2=3/4/5, leg3=6/7/8, leg4=9/10/11. Channels12–15 stay off.

After wiring/power checks and supporting the body, send each command separately:

```text
leg all
speed 5
cal all
```

Wait for Ready. `cal all` explicitly arms at individual references, one joint every100ms, without requiring previous checks. `test J 5` tests the four matching joints; `test J 0` returns them. J=0 hip,1 knee,2 foot. Select `leg N` for individual corrections. After physically confirming all twelve at reference, `check all` records your confirmation. Use `off` then `save` to retain calibration. Later, checked `armall` prepares for `stand`, `straight` or `step 1`. Follow the [complete calibration procedure](firmware/spyder_esp32_ble_four_leg_test/README.md) before motion.

`stop` cancels motion and holds all legs; `off` or urgent `!` releases all signals. BLE disconnect/unsubscribe also releases all. Reconnection never resumes motion.

## Project and handover

| Need | Guide |
|---|---|
| Current state, verified limits and remaining work | [Project state](docs/project-state.md), [project plan](docs/project-plan.md) |
| Continue development | [Handover](docs/handover.md), [development](docs/development.md), [contributing](CONTRIBUTING.md) |
| Functions, commands, storage and source layout | [Firmware guide](docs/firmware-guide.md), [sketch index](firmware/README.md) |
| Wiring and first upload | [Hardware](docs/hardware.md), [bring-up](docs/bring-up.md) |
| Selected STLs and editable Fusion counterparts | [CAD guide](cad/README.md) |
| Original example project and attribution | [hpsaturn/quadpod reference](docs/reference/upstream.md) |
| Software evidence and physical-test log | [Verification index](docs/README.md), [calibration log](docs/templates/calibration-log.md) |

The active source tree contains ESP32 sketches only. Legacy Uno/Sensor Shield firmware, its browser console and AVR tooling have been removed; previous versions remain in Git history. Arduino IDE/framework references here apply to the ESP32 toolchain.

Software tests and compiled builds are documented separately from hardware results. Actual iPhone radio behavior, reset/OE bias, supply performance and loaded mechanical motion still need bench verification.

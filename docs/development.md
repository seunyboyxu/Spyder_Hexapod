# Building and testing the ESP32 sketches

Requirements for host checks: Bash, g++ with C++11, pthread and AddressSanitizer/UndefinedBehaviorSanitizer support. No JavaScript/browser/AVR toolchain is needed.

```sh
bash scripts/test-esp32.sh
```

This runs all six published suites: single motor, Classic one leg, BLE one leg, four legs, walking and BLE core2/core3 return-type compatibility. Individual scripts are named `scripts/test-esp32-servo.sh`, `test-esp32-leg.sh`, `test-esp32-ble-leg.sh`, `test-esp32-four-leg.sh`, `test-esp32-walk.sh` and `test-esp32-ble-compat.sh`.

Portable controller/parser regression fixtures live in `tests/leg_core/` and are reused against each relevant ESP32 copy. Adapter/sketch tests use GPIO/I2C/UART/Bluetooth/FreeRTOS doubles under `tests/esp32_*`. They exercise real controller and sketch code without a device; real radio/electrical/mechanical behavior remains a bench check.

## PlatformIO

The root [platformio.ini](../platformio.ini) provides five independently filtered ESP32 environments. [PlatformioEntry.cpp](../firmware/PlatformioEntry.cpp) selects the nested .ino for PlatformIO; Arduino IDE opens each sketch folder directly and does not use this entry file. Default `pio run` builds the walking BLE controller. To compile all five, without uploading:

```sh
python3 -m venv .tools/pio-venv
.tools/pio-venv/bin/pip install platformio==6.1.18
export PLATFORMIO_CORE_DIR="$PWD/.tools/platformio"
.tools/pio-venv/bin/pio run -e esp32_servo -e esp32_classic_leg -e esp32_ble_leg -e esp32_ble_four_leg -e esp32_ble_walk
```

Pinned dependencies: espressif32 platform6.12.0 (Arduino ESP32 core2.0.17), ESP32 Dev Module, Adafruit PWM Servo Driver Library3.0.2 and BusIO1.17.4. Existing per-sketch `scripts/build-esp32-*.sh` helpers build isolated projects under ignored `.tools/`; they also compile only. Those helpers use `.tools/platformio` as PlatformIO's cache.

For Arduino IDE, follow [bring-up](bring-up.md). The BLE sketches additionally compile against ESP32 core3.3.2 with the bundled BLE library. A reproducible Arduino CLI command, after installing that core and the documented Adafruit libraries, is:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/spyder_esp32_ble_four_leg_test
```

Compile each sketch folder separately; never concatenate the sketches or mix companion files. Do not add upload targets to verification scripts.

## Before a PR

Run affected host suites, the aggregate suite, relevant actual ESP32 builds, `git diff --check` and `sha256sum -c cad/SHA256SUMS`. Check relative documentation links, selected file scope and ignored build directories. Record toolchain versions and remaining physical limitations in a dated [verification report](README.md#verification-evidence). Do not commit local toolchain configuration/proxy settings, credentials, caches, generated firmware or delivery ZIPs.

Keep all five folders self-contained for Arduino IDE. When modifying a shared controller/transport behavior, inspect its copies and verify affected variants. Retain boot/reconnect off, explicit arming, per-joint bounds, complete group preflight, global interruption and no delayed command replay. Use branch/PR workflow; hardware testing and merge authorization remain with the owner.

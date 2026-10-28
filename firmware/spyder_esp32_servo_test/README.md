# ESP32/PCA9685 — one unloaded servo (Bluetooth Classic)

Arduino IDE sketch for an **original ESP32 with Bluetooth Classic**, one PCA9685 at address **0x40**, and **one positional or continuous-rotation servo on channel 0**. Use the ESP32 with the PCA9685 directly; no external Bluetooth module or ESP32 Servo library is required. Leave the other 15 servo connectors empty.

The reported marking `ESP-32D, Wifi+BT, N4XX` suggests an original ESP32 but does not identify the full development board. This sketch targets a standard original ESP32 development board exposing GPIO21/22/23. ESP32-C3/S3 do not support this Classic SPP sketch; S2 has no Bluetooth. A bare ESP32 module needs additional power/upload circuitry.

## Wire with all power disconnected

| From | To |
|---|---|
| ESP32 **3V3** | PCA9685 **VCC** (logic) |
| ESP32 **GND** | PCA9685 **GND** and external servo-supply negative |
| ESP32 **GPIO21** | PCA9685 **SDA** |
| ESP32 **GPIO22** | PCA9685 **SCL** |
| ESP32 **GPIO23** | PCA9685 **OE** |
| **10 kΩ resistor** | Between PCA9685 **OE** and ESP32 **3V3** |
| External regulated **5 V positive** | PCA9685 **V+** terminal (servo power) |
| Servo signal, usually orange/yellow/white | Channel **0 S / signal** |
| Servo positive, usually red | Channel **0 V+** |
| Servo ground, usually brown/black | Channel **0 GND** |

Follow the printed S/V+/GND labels: connector orientation and servo wire colours can vary. Keep address jumpers A0–A5 open for 0x40. Power PCA9685 logic from **3.3 V**, so the board's I2C pull-ups do not apply 5 V to ESP32 pins. OE is active LOW; its external pull-up inhibits outputs during ESP32 reset/bootloader. Do not leave OE permanently grounded. Some boards have an OE pull-down. **Verify OE stays near 3.3 V during ESP32 reset with servo power off.** A 10 kΩ pull-up against a 10 kΩ board pull-down gives only about 1.65 V and is inadequate. If a 10 kΩ pull-down is confirmed, use a **1 kΩ pull-up instead** (about 3.0 V idle, 3.3 mA when GPIO23 drives LOW). If the board circuit is unknown or OE does not remain HIGH, identify that circuit before energising the servo; do not rely on an unverified resistor arrangement.

```text
PC USB ------------------> ESP32 USB connector
ESP32 3V3 ---------------> PCA9685 VCC
ESP32 GND ----------------+---- PCA9685 GND
                          +---- servo supply negative
Servo supply +5 V ------------ PCA9685 V+ ---- channel 0 servo positive

ESP32 GPIO21 ------------> SDA
ESP32 GPIO22 ------------> SCL
ESP32 GPIO23 --------+---> OE
                    |
ESP32 3V3 -- 10 kΩ --+
```

Secure the motor with its horn/linkage disconnected from the leg. Keep fingers and cables clear. Keep servo power off while wiring, uploading or resetting, and keep a physical supply switch accessible. Output release removes pulses; it does not disconnect motor power and its mechanical response depends on the servo.

## Can one supply power the ESP32 and driver?

**The PCA9685 does not generate or regulate an ESP32 supply.** V+ distributes the external servo supply; VCC powers the PCA9685 logic. Start with ESP32 powered through USB and a separate regulated servo supply, with common ground as above. Do not power the motor through ESP32 3V3, its board regulator or assume its USB rail can carry servo current. Choose the external supply for the actual motor's starting/stall current and allowed voltage; a model label alone does not establish these ratings.

Later, a suitably rated regulated 5 V source can feed two **parallel branches**: one directly to PCA9685 V+, one to the ESP32 development board's **documented 5 V input**. Identify that board and its USB/external-power isolation first. Do not connect external 5 V alongside PC USB unless the board explicitly supports it. Never connect 5 V to 3V3. Keep motor-current wiring separate from thin ESP32 signal/ground leads; meet the grounds at the supply/driver ground. The PCA9685 rail and connector ratings still apply.

## Arduino IDE upload

1. Extract the complete sketch folder. Open **`spyder_esp32_servo_test.ino`**; keep all four `.h/.cpp` companion files beside it.
2. In Boards Manager install **esp32 by Espressif Systems**. The verified core version is **2.0.17**; select it for the same build configuration.
3. Select **ESP32 Dev Module** for a generic original ESP32 development board, or its exact manufacturer's target. Select the correct USB port. The verified target uses 4 MB flash and the default partition with about 1.3 MB for the application. This Bluetooth build needs more than 1 MB of application space.
4. In Library Manager install **Adafruit PWM Servo Driver Library**, verified **3.0.2**, accepting **Adafruit BusIO**, verified **1.17.4**. `BluetoothSerial`, `Wire` and `SPI` are bundled with the ESP32 core. Select the ESP32 target and use its bundled BluetoothSerial library.
5. Turn servo power **off** and upload over USB. If the board cannot enter upload mode, hold BOOT while the IDE starts connecting, then release once writing starts. Close Serial Monitor if it blocks the upload port.
6. Open Serial Monitor at **115200 baud**, **Newline**. Send `show`, then `help`. Startup must report outputs **OFF**. A fault requires fixing wiring/library/board selection with servo power off and resetting; it cannot be cleared by `arm`.
7. Verify wiring and an unloaded motor before turning on its external supply. No output starts automatically.

PCA9685 nominal PWM is approximately **50.03 Hz**, using the assumed 25 MHz oscillator and prescale 121. The firmware checks configuration and every PWM write, with a 25 ms I2C timeout and periodic configuration reads. I2C/setup errors inhibit OE and latch a fault requiring reset.

## Bluetooth

Pair/connect to **`Spyder-Servo-Test`** using an **Android or Windows Bluetooth Classic SPP serial terminal**. Normal iPhone BLE terminal apps cannot use Classic SPP. Use newline or CRLF for outgoing commands. Bluetooth SPP has no UART baud setting to match; 115200 applies to USB Serial Monitor.

The stack uses its default pairing behavior; no fixed HC-05 PIN is configured. Accept pairing through your device's normal prompt. This is a local bench controller without application-level authentication; operate it in a controlled setting.

USB Serial Monitor and Bluetooth can stay connected together. Each has an independent command line; replies go back to the source. Both control the same single motor, so use one operator. Bluetooth disconnect immediately inhibits OE in its callback and cancels motion/input when serviced by the loop, even during USB-controlled motion. Reconnecting never arms or resumes. USB can explicitly re-arm after the disconnect is handled. Radio-loss detection is not an instantaneous physical emergency stop.

## First test: send commands individually

Start with servo power off, the motor unloaded and `show` reporting OFF. Then energise its servo supply and send:

```text
arm
show
angle 95
show
center
off
```

Wait between commands and use `show` to observe the current request and target. `arm` starts a **1500 µs** signal immediately: the shaft can jump from its unknown starting position. `angle 95` requests about **1528 µs**, reached gradually at nominal **50 µs/s**. `center` gradually returns to 1500. `OK` accepts the request; it does not mean the shaft has reached it. The motor provides no position feedback.

| Command | Behavior |
|---|---|
| `help` / `show` | Commands or current state, requested pulse and target. |
| `arm` | Explicitly enable channel 0 at 1500 µs. Rejected if already armed. |
| `center` | While armed, slowly request 1500 µs. |
| `angle N` | While armed, slowly request a nominal angle, integer **0–180**. |
| `pulse N` | While armed, slowly request integer **1000–2000 µs**. |
| `1` | While armed, immediately request **1300 µs** for two seconds, then release. |
| `2` | While armed, immediately request **1500 µs** for two seconds, then release. |
| `3` | While armed, immediately request **1700 µs** for two seconds, then release. |
| `sweep` | While armed, make **one** gradual centre → 1000 → 2000 → centre pass, then hold. |
| `stop` | Cancel the test/sweep/target and hold the last commanded pulse. |
| `off` | Inhibit output; fresh `arm` required. |
| `!` | Inhibit output without Enter; discard the rest of its line. Send a fresh blank newline afterwards. |

For the two-second tests, send `arm` before each test because each releases at its end. Observe whether the shaft moves to a fixed position and holds, or keeps rotating. No movement is inconclusive; check wiring, power and the motor. A positional 360-degree servo also holds a position, so these observations **cannot prove an exact 180-degree versus 360-degree travel range**. Continuous-rotation servos interpret the pulse as speed/direction, not an angle; 1500 is only approximate neutral and may still rotate.

For an unloaded positional motor that passed small movements, send `arm` if released, then `sweep`. From centre the pass takes about **40 seconds**; from another position it can take up to about 50 seconds. Movement commands are rejected while a timed test/sweep runs; `show`, `help`, `stop`, `off` and `!` remain available. Completed sweeps hold centre, rather than starting another cycle. No command automatically seeks or detects a mechanical stop. Stop/release and remove power if the motor buzzes, stalls, slips or binds.

**Angle scale:** nominal 0/90/180 map to **1000/1500/2000 µs**. This differs from the leg sketches' 544/1472/2400 scale and is not measured physical degrees. Do not copy angle-based calibration between those scales. The limited pulse envelope may cover only part of the motor's travel. PCA9685 tick rounding is constrained inward at the endpoints (nominally about 1000.4–1995.92 µs); real oscillator error can change physical widths. Measure pulses/shaft position if precision is required.

## Input and failure behavior

Send one command at a time; do not paste movement batches or use repeating macros. Unknown/malformed commands and out-of-range values are rejected without changing the target. Overlong/non-ASCII input, USB UART receive errors, Bluetooth receive-queue overflow, or partial lines idle for over two seconds cancel motion and hold. The sketch uses its own bounded Bluetooth receive queue so overflow cannot be silently hidden by the default library buffer. Safety commands cancel both ports' partial input and flush a bounded snapshot of received transport data; bytes still in flight cannot be recalled. After `!`, an input error or an interrupted partial command, send a **blank newline** before the next command. Lost replies under excessive traffic cause a hold rather than blocking the motor loop.

Reply transmission runs in a separate task, so a congested Bluetooth connection cannot block command handling or gradual motion. Updates make at most one 20 ms step after a delayed loop; delays can make a test slower, not cause a catch-up jump. No calibration survives reset, and no leg calibration is imported.

## Developer verification

From the repository root, `bash scripts/test-esp32-servo.sh` runs sanitizer-backed controller and adapter checks. The controller, adapter and actual-sketch harnesses substitute physical I2C/GPIO, radio/UART and scheduler boundaries; it is not a real PCA9685, radio or mechanical simulation. `bash scripts/build-esp32-servo.sh` performs the verified original-ESP32 build using a locally installed PlatformIO (Arduino framework); it does not upload. See the accompanying verification report for exact results and hardware checks still required.

Published as an ESP32 bench-test controller at the owner's request. Physical validation remains pending; see the [current state](../../docs/project-state.md).

## Code overview

The [firmware and function guide](../../docs/firmware-guide.md) explains this sketch's commands, main classes/functions, storage, input handling and safety. The [sketch index](../README.md) compares all four ESP32 upload options.

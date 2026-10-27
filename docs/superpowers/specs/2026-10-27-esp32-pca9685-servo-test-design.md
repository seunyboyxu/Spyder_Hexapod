# ESP32 Bluetooth and PCA9685 single-servo test

Status: proposed for owner review; no ESP32 firmware written or uploaded yet.

## Purpose and scope

Replace the Uno/HC-05 connection for an unloaded, single-motor bench test with an original ESP32 and a PCA9685. Keep Arduino IDE upload and USB Serial Monitor support. The owner reports the marking `ESP-32D, Wifi+BT, N4XX`; this suggests the original ESP32 family, but is not a complete board model. Target a development board exposing GPIO21, GPIO22 and GPIO23, with Bluetooth Classic support. Do not apply this pinout to a bare module without its required support circuitry.

The owner's new ESP32/PCA9685 request supersedes CLAUDE.md's older Uno-only constraint for this new sketch. Existing Uno firmware, local one-leg files, tests and web console remain untouched. The four-leg Uno design is deferred by this hardware change. No walking or assembled-leg motions belong in this one-servo test.

## Approach

Use built-in Bluetooth Classic SPP, matching the kind of terminal used previously with HC-05. It preserves the existing text-command workflow without a separate module. BLE would require a different terminal/protocol; retaining HC-05 adds unnecessary wiring. Classic SPP is the proposed approach for the original ESP32. Ordinary iPhone BLE apps do not support this connection.

Create a separate Arduino sketch folder, proposed as `firmware/spyder_esp32_servo_test/`, using Espressif's Arduino ESP32 core, its bundled BluetoothSerial/Wire libraries and the Adafruit PWM Servo Driver library. USB Serial runs at 115200; Bluetooth has no matching UART baud requirement. Give USB and Bluetooth independent input buffers and send replies to the originating connection. Both can issue safety commands.

## Commands and motion

- Boot with all PCA9685 channels full-off and hardware OE HIGH. No automatic movement or automatic restart after reconnecting.
- Control channel 0 only. All other channels remain off.
- `help` and `show` report commands, controller state, channel and commanded pulse; there is no measured-position feedback.
- `arm` explicitly enables channel 0 at 1500 microseconds. First activation can move abruptly because the shaft position is unknown.
- `center` slowly requests 1500 microseconds while armed.
- `pulse N` requests an integer pulse within 1000–2000 microseconds while armed.
- `angle N` accepts nominal integer degrees 0–180 and maps linearly to 1000–2000 microseconds: 90 means 1500. This is a documented change from the Uno Servo library's default 544–2400 microsecond scale; neither mapping establishes physical degrees or travel.
- `1`, `2`, `3` perform the earlier 1300/1500/1700 microsecond tests, respectively, while armed, then release after two seconds. Each is one complete line.
- `sweep` performs one gradual centre → 1000 → 2000 → centre sequence, then holds. It does not search for mechanical stops or repeat indefinitely. Use only with the servo disconnected from the leg; the electrical envelope does not prove mechanical clearance.
- `stop` cancels the remaining motion and holds the last commanded pulse; `off` disables output. `!` disables immediately without Enter and discards the remainder of that input line. Release removes the signal, not servo power.
- Gradual moves and sweep use a nonblocking update loop, nominally 50 microseconds per second, without catch-up jumps. Manual motion commands during a running test/sweep are rejected until `stop` or completion. Releasing requires a fresh `arm` before any later motion.
- Reject malformed commands and out-of-range values without changing output. Overflow, stale partial lines or input integrity errors cancel motion and hold. Bound command buffers; partial lines expire after two seconds. Send one command at a time, with newline or CRLF.
- Bluetooth disconnect releases output and cancels its pending motion, even if USB is connected. USB can explicitly re-arm afterwards. A supply switch remains necessary: detecting radio loss is not an instantaneous physical emergency stop.
- I2C initialization/write failures disable OE and latch a fault requiring reset. Set a bounded I2C timeout; do not enable output after an unconfirmed write. Check write-status behavior of the chosen driver library during implementation.
- No persistent calibration, Wi-Fi interface, automatic servo-type identification, authentication design or phone app is included. The owner observes whether the shaft holds position or rotates. Positional behavior alone cannot distinguish a 180-degree positional servo from a 360-degree positional servo.

## Wiring for an original ESP32 development board

Disconnect power while wiring. Do not connect the old Sensor Shield or HC-05.

| Connection | Destination |
|---|---|
| ESP32 3V3 | PCA9685 VCC, logic power only |
| ESP32 GND | PCA9685 GND and external servo-supply negative |
| ESP32 GPIO21 | PCA9685 SDA |
| ESP32 GPIO22 | PCA9685 SCL |
| ESP32 GPIO23 | PCA9685 OE |
| 10 kilohm resistor | Between PCA9685 OE and ESP32 3V3 |
| Regulated external 5 V positive | PCA9685 V+ servo-power terminal |
| Servo signal | PCA9685 channel 0 signal/S pin |
| Servo positive | PCA9685 channel 0 V+ pin |
| Servo ground | PCA9685 channel 0 GND pin |

Check the actual board's printed signal/V+/GND labels; connector orientation varies. Use default I2C address 0x40 with address jumpers open. Nominal PWM frequency is 50 Hz. PCA9685's oscillator and 12-bit tick rounding mean physical pulse widths need measurement for precision. The implementation must round electrical endpoints inward so nominal requests remain within the configured pulse envelope at its assumed clock.

OE is active LOW. The external pull-up keeps it disabled while GPIO23 is high impedance on reset. Before enabling, explicitly initialize all channels full-off and preload channel 0. Firmware alone cannot guarantee output behavior through every partial-power or wiring failure. Keep servo power off for wiring/upload/reset and keep an accessible supply switch.

## Power

For the initial test, power the ESP32 by its USB connector and feed PCA9685 V+ from an external regulated 5 V supply rated for the actual servo's stall current. Share grounds. Do not power the servo from ESP32 3V3 or assume the USB/board regulator can supply it. Logic VCC must remain 3.3 V so I2C pull-ups do not pull ESP32 pins to 5 V.

The PCA9685 is not a voltage regulator or a power supply. Its V+ rail distributes external power; it does not generate a regulated ESP32 supply. One suitably rated regulated 5 V supply can later feed both devices in parallel: one branch to V+, one to the development board's documented 5 V input. The exact ESP32 board and its USB/external-power isolation must be identified before prescribing that shared-power connection. Do not connect external 5 V alongside USB unless that board documents support, and never feed 5 V into 3V3. Avoid routing servo current through the ESP32 board or thin signal jumpers.

## Arduino IDE and acceptance

Install `esp32 by Espressif Systems` through Boards Manager and `Adafruit PWM Servo Driver Library` plus its dependencies through Library Manager. For a generic original ESP32 development board, `ESP32 Dev Module` is the proposed board target; select the exact manufacturer's target if known. Upload over USB with servo power off. Monitor at 115200 baud/Newline. Pair to proposed device name `Spyder-Servo-Test` using a Bluetooth Classic SPP terminal on Android or Windows. No HC-05 library or hardware is required. The classic Bluetooth library must reject unsupported ESP32 variants clearly at compile time.

Before delivery, run focused host checks for pulse mapping, finite motion, input handling and stop/release; compile the actual sketch for the original ESP32 when the toolchain is available; review PCA9685 register/write handling and boot sequencing. Preserve existing checks and report build limitations explicitly. Package the complete sketch and wiring/usage instructions in a ZIP. Cloud checks do not establish real oscillator accuracy, supply performance, Bluetooth pairing, OE behavior or mechanical travel. The owner performs the hardware tests; do not upload, move motors or push new firmware before their feedback.

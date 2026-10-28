# ESP32/PCA9685 BLE — iPhone one-leg calibration and motion test

**Compatibility update:** BLE receive handling now accepts both ESP32 core2 `std::string` and core3 Arduino `String` return types. This fixes the `conversion from String to const std::string` error without changing calibration or wiring. See the [BLE compatibility report](../../docs/verification/2026-10-27-ble-string-compatibility.md) for build results.

Use this sketch **instead of the separate one-motor sketch**. It uses the positional one-leg controller with the original ESP32's built-in Bluetooth Low Energy (BLE) and the PCA9685. Upload through Arduino IDE over USB. It controls **one leg with three positional servos**, with the body mount supported and foot initially clear of the bench.

| Joint ID | PCA9685 channel | Servo |
|---|---|---|
| **0** | **0** | Hip / yaw |
| **1** | **1** | Knee / thigh pitch |
| **2** | **2** | Foot / lower-leg joint |

These are PCA9685 output channels, not ESP32 GPIO0/1/2. Leave channels3–15 empty. Motors must hold fixed positions; continuous-rotation motors cannot follow these joint angles. No whole-robot standing/balance claim is made.

## Wiring and power

Your **SDA → GPIO21** and **SCL → GPIO22** connections are correct.

| ESP32 / supply | PCA9685 |
|---|---|
| ESP32 **3V3** | **VCC** logic supply |
| ESP32 **GND** and external supply negative | **GND** |
| ESP32 **GPIO21 / D21** | **SDA** |
| ESP32 **GPIO22 / D22** | **SCL** |
| ESP32 **GPIO23 / D23** | **OE** |
| Regulated external **5 V positive** | **V+** servo-power terminal |

Also fit a pull-up from OE to ESP32 3V3. OE is active LOW and must remain HIGH during ESP32 reset. A **10 kΩ pull-up** works only where the board's OE bias permits it. If your board has a confirmed **10 kΩ OE pull-down**, use a **1 kΩ pull-up instead**: 10k up against10k down gives only1.65V and is insufficient; 1k up against10k down gives about3.0V idle. Verify OE near3.3V while resetting the ESP32, with servo power off. Identify an unknown OE circuit before energising the servos. Do not permanently ground OE.

Connect each servo's signal to its channel's **S/signal**, positive to **V+** and ground to **GND**. Read the board labels; connector orientation and wire colours vary. Keep address jumpers open for **0x40**.

Initially power the ESP32 through its USB connector, and feed the driver V+ from an external regulated5V supply able to handle the **combined starting/stall current of all three motors**. Share grounds, but route servo current through suitable power wiring rather than ESP32 pins or thin signal jumpers. PCA9685 VCC stays3.3V to keep I2C pull-ups at ESP32 logic levels. The PCA9685 is not a voltage regulator and does not generate power for the ESP32.

One suitable regulated5V source can later feed parallel branches to V+ and the ESP32 board's documented5V input. Identify the actual development board and its USB/external-power isolation first; do not connect external5V alongside PC USB unless supported. Never feed5V into3V3. Keep servo power off during wiring/upload/reset and keep a physical supply switch accessible.

## Arduino IDE and Bluetooth

1. Extract the **complete `spyder_esp32_ble_leg_test` folder**, and open `spyder_esp32_ble_leg_test.ino`. Keep every companion header and `.cpp` file beside it.
2. Install **esp32 by Espressif Systems**, verified versions **2.0.17 and3.3.2**, through Boards Manager. Select **ESP32 Dev Module** for a generic original ESP32 board, or its exact manufacturer target, and the correct USB port. The verified target is4MB flash with default application partition about1.3MB.
3. Install **Adafruit PWM Servo Driver Library 3.0.2**, accepting **Adafruit BusIO 1.17.4**, through Library Manager. BLEDevice/BLEServer, Wire, SPI and Preferences are bundled with the ESP32 core. Do not install a separate ArduinoBLE or BluetoothSerial library for this sketch. No external Bluetooth module or Servo library is used.
4. Turn servo power off and upload over USB. Open Serial Monitor at **115200 baud**, **Newline**. Send `help` and `show`: all outputs must report OFF. Invalid/absent stored calibration starts from defaults; no joint ever activates on boot.
5. For the iPhone, use the BLE setup below. USB Serial Monitor remains available at115200/Newline.

## Connect from an iPhone

The earlier sketch used **Bluetooth Classic SPP**, which ordinary iPhone serial apps cannot use. This version uses the **Nordic UART Service over BLE**, supported by iOS BLE UART apps. **Connect inside the app, not iPhone Settings → Bluetooth.** No HC-05 module or PIN is required.

1. Upload **this BLE sketch** with servo power off. Merely changing the phone app cannot make the earlier Classic firmware advertise BLE.
2. Install/open **Adafruit Bluefruit Connect** on the iPhone and allow its Bluetooth permission. Scan in the app for **`Spyder-Leg-BLE`**, connect, and open its **UART** module.
3. The app must enable **TX notifications** to receive replies; a Nordic-UART-aware app normally does this when opening UART. Send commands as text with **newline or CRLF**. Send `help`, then `show`, with motors initially unpowered. New connections never activate servos.
4. Confirm the reply identifies **ESP32 BLE leg test**, channel0 hip,1 knee,2 foot, and outputs OFF. Then follow the calibration procedure below before leg motions.

With a generic app such as **nRF Connect**, connect to the device and open:

| GATT item | UUID / action |
|---|---|
| Nordic UART service | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| **TX** — ESP32 → phone | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`; **enable notifications** using its subscription control |
| **RX** — phone → ESP32 | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`; write text plus newline; Write Request or Write Without Response supported |

If the app's text field cannot append a newline, use **Hex** writing to RX: `68 65 6C 70 0A` sends `help` plus newline; `73 68 6F 77 0A` sends `show` plus newline. Text backslash-n is not the newline byte. Reply notifications may arrive in20-byte pieces; concatenate them in order. The firmware paces these packets and supports the default23-byte ATT MTU without requiring a larger negotiated MTU.

If it does not appear, verify the uploaded sketch/USB banner says BLE, keep the ESP32 powered nearby, check the app's Bluetooth permission and scan again inside the app. An actively connected device stops advertising to other clients; disconnect the other app first. A device may be shown under its service/name after a scan response. An old Classic pairing entry in Settings is not this app connection.

USB and BLE can stay connected together, with separate command buffers/replies. Both control the same leg, so use one operator. **BLE disconnect or disabling TX notifications inhibits all leg outputs**, including USB-started motion, and cancels pending input/sequence. Reconnection or subscription never reattaches; use `cal J` explicitly. Do not rely on a phone remaining connected in the background; support the leg and keep a supply switch accessible. Radio-loss detection is not an instantaneous physical emergency stop.

BLE motion/configuration commands are rejected until notifications are enabled. `off`, `stop` and urgent `!` still work without a reply subscription. If a rapid notification toggle happens during a congested reply, the firmware releases safely and may reject that subscription; wait briefly and enable notifications again. Callback waits are bounded so radio congestion cannot deadlock the BLE event task. Queued replies from an old subscription are rejected; packets already accepted by the Bluetooth stack or in flight cannot be recalled. This local bench controller has no application-level authentication; use it in a controlled setting.

## Calibration — one joint at a time

The geometric reference is **hip pointing straight out from the future body, thigh horizontal, lower leg vertically down**. Secure the body mount and support loose links without resisting a motor. First attachment can jump from an unknown shaft position; do initial alignment with the relevant horn/linkage disconnected if clearance is uncertain. Refit a badly aligned horn with power off.

Initial reference commands are **90°**, raw limits **60–120°**, speed **10°/s**. Limits are starting bounds, not measured stops. For joint0, send each line separately:

```text
cal 0
show
move 0 88
zero 0
test 0 5
test 0 0
check 0
off
```

`move 0 88` is only an example adjustment. Use small changes until that joint is physically at its reference. Wait for **Ready** before recording `zero`, changing direction or marking it checked. A fresh `cal` attachment replies OK without Ready; allow the motor to settle. `zero J` stores the stationary raw command as its reference; it does not measure shaft position.

Positive `test` directions must be:

- **Hip0:** counter-clockwise when viewed from above the future body.
- **Knee/thigh1:** lift the thigh.
- **Foot2:** fold the foot toward the future body.

If a direction is reversed, return using `test J 0`, send `flip J`, then repeat the small `test J 5` and `test J 0` checks. Finish with `check J` at the reference. `check` records **your physical confirmation**, not a sensor reading. Repeat for joints1 and2, releasing with `off` between joints. `cal J` activates only that joint; already-active joints keep holding until released.

Changing a zero, direction or limit clears that joint's checked flag. With all outputs off, narrow limits to verified free travel, for example `limits 1 65 115`. Limits must contain the stored zero and remain within absolute **45–140°**. Re-check afterwards. Pose commands reject a pose outside any limit; they do not widen limits or silently clamp a joint. `speed 5` selects5°/s while stationary; valid speed is1–20°/s.

**Angle scale:** this port deliberately retains the earlier **leg controller's Servo-library scale**,544–2400µs over0–180°, independently bounded to1000–2000µs by permitted joint limits and the output adapter. Thus **90° requests1472µs**, unlike the separate one-motor tester's nominal90°=1500µs. Do not transfer angle-based calibration between those scales. Nominal degrees and pulse requests are not measured physical positions; real travel requires observation/measurement. PCA9685 tick quantization and oscillator tolerance also affect physical pulse widths.

## Test the supported leg

After all three joints have been aligned, direction-tested and marked checked, reattach their references individually:

```text
cal 0
cal 1
cal 2
show
stand
straight
stand
step 1
```

These are **individual commands**, not a batch to paste. Allow each attachment to settle; for a moving pose wait for Ready before the next command. All three must show `attached=1 checked=1`. Support the mount, keep the foot suspended for the first step, and check that no link binds. If a motor buzzes, stalls, slips or moves the wrong way, send `!` and remove power as needed. Support the links as torque is released.

| Command | Action |
|---|---|
| `show` / `help` | Current command/configuration or available commands. |
| `cal J` | Activate only J at its stored reference, or slowly return an active J to reference. |
| `move J DEG` | Set an active joint's raw nominal angle within its limits. |
| `zero J` | Record its current stationary command as reference. |
| `test J OFFSET` | Request an offset from reference in its checked positive direction. |
| `flip J` / `check J` | Reverse future offsets / record physical direction confirmation at reference. |
| `limits J MIN MAX` | Set bounds with all outputs off; invalidates the joint's check. |
| `speed N` | Nominal joint speed1–20°/s, stationary only. |
| `stand` | Move foot to85mm outward and60mm below hip axis; hold afterwards. |
| `straight` | Partial extension to125mm outward and30mm below; retain a knee bend. |
| `step 1` | One20mm end-to-end,12mm-lift step demonstration; return to standing and hold. |
| `step 2` / `step 3` | Two or three finite cycles; no endless walking mode. |
| `stop` | Immediately cancel remaining motion and hold current commanded positions. |
| `off` | Inhibit all three signals, then clear their channel outputs. |
| `!` | Inhibit without Enter; discard the rest of its input line. Send a fresh blank newline afterwards. |
| `save` / `load` | Save/load calibration with all outputs off; loading never activates motors. |

`stand`, `straight` and `step` require all joints attached, checked and stationary. Entire waypoint sequences are checked against all three limits before starting. During `step`, manual commands are rejected until it completes or `stop` cancels it. Manual joint commands can replace a gradual non-step pose target. All gradual movement is nonblocking and makes no catch-up jump after a delayed loop. Ready means the **commanded** target/sequence completed, not measured physical arrival.

Geometry is the earlier estimated **27.5mm hip offset,55mm thigh,74mm shin** in `LegTest.h`; measure your assembly's shaft-to-shaft/shaft-to-foot distances before relying on the Cartesian poses. A finite cycle is a supported leg-motion demonstration, not proof of balanced walking, foot clearance or load-bearing standing. There is deliberately no full-range sweep for an assembled leg; the separate unloaded one-motor sketch remains available for that task.

## Stored settings and safety behavior

Send `off`, then `save` after completing physical calibration. References, limits, directions and checked flags are serialized with the earlier version/CRC/semantic checks into ESP32 Preferences namespace **`spyder-leg`**, key **`cal`**. A valid record loads on boot while every output remains off. Invalid data is ignored; explicit failed load leaves current settings unchanged. No single-motor configuration or external calibration is imported. Re-check saved calibration after changing horns, wiring or mechanics.

STOP holds the current pulse commands and discards the remaining sequence. OFF/`!` removes signal, not electrical servo power. I2C/setup failures inhibit OE and latch a fault requiring reset. RX buffer overflow, UART receive errors, malformed input bytes, overlong lines and partial lines idle for over2seconds hold motion and clear pending input. Unknown complete commands/out-of-range values are rejected without target changes.

Send one command at a time. After urgent release/input errors/interrupted partial lines, send a blank newline before the next command. Both inputs' partial/queued data are cancelled on safety events; bytes still in flight cannot be recalled. Repeated command macros are inappropriate. Replies run in a separate task; reply congestion cannot block motion/input servicing, and excessive reply load holds motion.

## Software verification and your test report

From the repo, `bash scripts/test-esp32-ble-leg.sh` runs controller, parser, PCA adapter and actual-sketch host checks under sanitizers. `bash scripts/build-esp32-ble-leg.sh` compiles for the actual original ESP32 Arduino target without uploading. The BLE contract and actual-sketch checks use host doubles, not an iPhone or radio. Physical BLE app interoperability, electrical reset/OE behavior, oscillator accuracy, supply quality and mechanics are not reproduced by the host doubles.

After testing, send the complete `show` output and note whether each direction check, `stand`, `straight`, one `step 1`, STOP/OFF and BLE disconnect behaved correctly. Published at the owner's request; physical validation remains pending.

**Power update:** the confirmed rectangular PP3 9V battery with a5V/3A buck is not a suitable multi-servo supply. Use an appropriately rated regulated5V source and distribution before powered leg tests; initial help/show connection checks can be done with servo power off.

## Code overview

The [firmware and function guide](../../docs/firmware-guide.md) explains this sketch's commands, main classes/functions, storage, input handling and safety. The [sketch index](../README.md) compares all four ESP32 upload options.

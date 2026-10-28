# ESP32 and PCA9685 hardware

Current board marking: **ESP-32D, Wifi+BT, N4XX**. This indicates the original ESP32 family but does not identify the complete development board. Verified compilation target is **ESP32 Dev Module**,4MB flash, default approximately1.3MB application partition. Classic sketches require original ESP32 Bluetooth Classic; use the verified original target for BLE too. Do not assume ESP32-C3/S3/S2 or a bare module share these capabilities/pins.

| Connection | Destination |
|---|---|
| ESP32 3V3 | PCA9685 VCC logic power |
| ESP32 GND and external servo-supply negative | PCA9685 GND |
| ESP32 GPIO21 / D21 | PCA9685 SDA |
| ESP32 GPIO22 / D22 | PCA9685 SCL |
| ESP32 GPIO23 / D23 | PCA9685 OE |
| External regulated5V positive | PCA9685 V+ servo rail, subject to connector/trace ratings |

Keep PCA address jumpers open for0x40. Read signal/V+/GND labels rather than guessing connector orientation. VCC stays3.3V so I2C pull-ups do not apply5V to ESP32 pins. PCA nominal frequency is50.03Hz using assumed25MHz oscillator and prescale121; oscillator accuracy and real pulse widths need measurement.

OE is active LOW. Fit a pull-up to3V3 and verify it holds near3.3V during reset with servo power off. A10k pull-up is suitable only if actual board bias permits. Against a confirmed10k board pull-down, use1k pull-up:10k/10k yields only1.65V, whereas1k/10k yields about3.0V. Identify unknown circuitry before energizing servos; do not ground OE permanently.

Four-leg PCA map:1=0/1/2,2=3/4/5,3=6/7/8,4=9/10/11, in hip/knee/foot order. Channels12–15 stay off. One-leg sketches use0–2; unloaded motor sketch uses0 only. These are PCA output channels, not ESP32 GPIOs.

## Power

The confirmed small rectangular PP3 9V battery →5V/3A buck is inadequate for powered multi-servo tests. A3A output at5V is15W and requires roughly2A at9V after typical losses, beyond a PP3's sustained capability; twelve servo peaks can also exceed3A. Staged activation cannot fix inadequate supply capacity.

Use an external regulated5V source and distribution sized for combined starting/stall demand with margin. Check actual servo ratings and PCA terminal/trace ratings; do not route excessive current through thin jumpers or an underrated board rail. Start with ESP32 on USB and separate servo power, common ground. Motor current must not pass through ESP32 3V3, its regulator or the PC USB supply.

The PCA9685 distributes external servo power and does not regulate an ESP32 supply. A suitable regulated5V source may later supply two parallel branches, one to V+ and one to the development board's documented5V input, after checking that board's USB/external-power isolation. Never5V into3V3; do not parallel external5V and USB unless the board supports it.

Keep motor power off for wiring/upload/reset, support the body, and keep a physical servo-supply switch accessible. OFF removes signal, not electrical power or guaranteed holding torque. First attachment may jump from an unknown shaft position. Verify horn retention and free travel before loaded tests.

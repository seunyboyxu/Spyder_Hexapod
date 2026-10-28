# ESP32/PCA9685 — four-leg BLE controller v1.1 (current)

**Version1.1 adds group calibration and unchecked calibration arming.** Arduino IDE sketch for the original ESP32, one PCA9685 and **four three-joint positional-servo legs**. It extends the earlier iPhone BLE leg test and includes the ESP32 core2/core3 `getValue()` String compatibility fix.

| Leg selection | Hip J0 | Knee/thigh J1 | Foot J2 |
|---|---|---|---|
| **leg 1** | **channel0** | **channel1** | **channel2** |
| **leg 2** | **channel3** | **channel4** | **channel5** |
| **leg 3** | **channel6** | **channel7** | **channel8** |
| **leg 4** | **channel9** | **channel10** | **channel11** |

Channels12–15 stay off. These are PCA9685 channels, not ESP32 GPIO numbers. The three servos on each leg must hold fixed positions; continuous-rotation motors cannot follow these commands.

**Support or suspend the body for these tests. All four legs perform the same local step at once, including lifting together. This is a motion test, not a balanced walking gait.** The chosen physical front/corner assignment does not automatically mirror servo directions: physically calibrate every joint.

## Wiring and power

Your **SDA → GPIO21** and **SCL → GPIO22** wiring is retained.

| Connection | Destination |
|---|---|
| ESP32 **3V3** | PCA9685 **VCC**, logic power |
| ESP32 **GND**, servo-supply negative | PCA9685 **GND** |
| ESP32 **GPIO21 / D21** | PCA9685 **SDA** |
| ESP32 **GPIO22 / D22** | PCA9685 **SCL** |
| ESP32 **GPIO23 / D23** | PCA9685 **OE** |
| Regulated external **5 V positive** | Servo power **V+**, subject to rail/connector current ratings |

Connect each servo's S/signal, V+ and GND to its channel's printed labels. Keep I2C address jumpers open for0x40. ESP32 USB supplies the controller; servo current must come from the external supply, with common ground. Logic VCC stays3.3V so I2C pull-ups do not apply5V to ESP32 pins.

OE is active LOW and needs a pull-up to3V3 during ESP32 reset. A10k pull-up is suitable only if the actual board bias permits it. With a confirmed10k board pull-down, use a1k pull-up instead:10k up/10k down gives only1.65V;1k up/10k down gives about3.0V. Verify idle OE near3.3V during reset with servo power off. Identify an unknown OE circuit before applying motor power. Do not ground OE permanently.

### Twelve-servo supply requirement

The owner confirms a **small rectangular PP3 9V battery feeding a5V/3A buck converter**. **Do not use this setup for these simultaneous twelve-servo tests.** The converter is limited to15W output, requiring roughly2A from a nominal9V source after typical conversion losses at full rating; a PP3 cannot sustain that input demand. Twelve servos can also exceed the converter's3A output. Voltage sag can cause resets, twitching and erratic motion. A different power source and suitable converter/supply are required before testing all twelve.

Use an external regulated5V source and power distribution sized for the **combined starting/stall demand of all twelve SG90s**, with margin. Converter output-current ratings and the source battery's real current capability both matter; model labels alone do not establish stall current. The existing PP3/3A combination is insufficient; the replacement supply and distribution ratings must be checked against actual peak demand. A current-limited bench supply or suitably rated regulated servo supply is preferable for initial testing.

Check the PCA9685 board's V+ traces, screw-terminal and connector ratings. Do not route a twelve-servo peak through an underrated board rail or thin jumper leads: use appropriately rated distribution, wiring and protection where needed, with common signal ground. Do not power motors through ESP32 3V3, its regulator or PC USB. Staged `armall` spreads initial activation; simultaneous motion can still produce a large current peak.

The PCA9685 does not regulate power for the ESP32. One suitable regulated5V supply can later feed separate parallel branches to servo power and the development board's documented5V input, but identify that board's USB/external-power isolation first. Never feed5V into3V3 or connect external5V alongside USB unless supported. Keep servo power off for wiring/upload/reset and keep a physical motor-supply switch accessible.

## Arduino IDE and iPhone

Extract the complete **`spyder_esp32_ble_four_leg_test`** folder and open its matching `.ino`. Keep all companion `.h/.cpp` files beside it.

- Board: **ESP32 Dev Module** for a generic original ESP32 development board, or its exact manufacturer target. Use4MB flash and the default application partition of about1.3MB for the verified configuration.
- Install **esp32 by Espressif Systems**. The receive code handles both the core2 `std::string` and core3 Arduino `String` BLE APIs automatically. See the [current verification report](../../docs/verification/2026-10-28-four-leg-group-calibration.md) for actual build versions/results. Verified real ESP32 cores are2.0.17 and3.3.2.
- Install **Adafruit PWM Servo Driver Library3.0.2** and **Adafruit BusIO1.17.4**. BLE, Wire, SPI and Preferences are bundled with the ESP32 core. Do not install a separate ArduinoBLE/BluetoothSerial library.
- USB Serial Monitor: **115200 baud, Newline**. Upload with servo power off, then send `help` and `show`; startup must be OFF. No outputs activate on reset or calibration load.

For iPhone, open **Adafruit Bluefruit Connect**, allow Bluetooth permission, scan/connect to **`Spyder-4Leg-BLE`**, and open **UART**. Connect inside the app, rather than iOS Bluetooth Settings. Enable TX notifications and send each text command with newline/CRLF. USB and BLE can stay connected, but both control the same motors: use one operator.

Generic **nRF Connect** settings:

| Item | UUID / action |
|---|---|
| Nordic UART service | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| TX, ESP32 → phone | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`; **enable notifications** |
| RX, phone → ESP32 | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`; write text plus newline |

If an app cannot append newline, Hex-write `68 65 6C 70 0A` for `help` or `73 68 6F 77 0A` for `show`. Text backslash-n is not the newline byte. Replies are paced20-byte notifications; allow a complete status response to arrive before another query. `show all` is longer than the old three-joint status. If the app caches the previous name, rescan/reconnect and verify the four-leg banner with `show`.

BLE disconnect or disabling TX notifications inhibits **all twelve** outputs and cancels motion, including USB-started motion. Reconnecting/subscribing never reattaches. Wireless motion/configuration requires notifications; stop/off/! remain effective without them. Rapid subscription toggles during congestion fail closed; wait briefly and re-enable notifications. Packets already in flight cannot be recalled. This local bench controller has no application-level authentication; use it in a controlled setting.

## Calibrate all four legs together

After replacing the inadequate PP3/3A power setup, support the body with all legs clear. Connect in the iPhone app as above, then send these commands **one line at a time**:

```text
leg all
speed 5
cal all
```

Wait for **Ready**. `cal all` explicitly arms all12 at their own saved reference angles, one joint every100ms (about1.2seconds), even if not yet checked. It requires all outputs off initially. Nothing is marked checked automatically. This is for initial calibration; use checked `armall` for later test sessions. First attachment can jump from an unknown physical position; align loose horns before mounting if needed.

You can also arm just the four matching joints with `cal 0` (hips/channels0,3,6,9), `cal 1` (knees/1,4,7,10), or `cal 2` (feet/2,5,8,11). Previously attached joints keep holding. Matching-joint attachment is immediate; allow the motors to settle. If already attached away from reference, `cal J` returns gradually and prints Ready on completion.

For each J=0,1,2, use small movements and physically observe **all four**:

```text
test 0 5
test 0 0
```

Wait for Ready after each movement. Each joint uses its **own zero and direction**. Positive motion is hip counter-clockwise viewed from above, knee lifting the thigh, foot folding toward the body. If only one leg needs adjustment, select `leg N`, correct its alignment/direction with the individual commands below, return to reference, then select `leg all` again. `flip J` in all mode reverses **all four** matching joints; use it only if all four need reversal. `move J DEG` requests the same raw angle on all four, whereas `test J OFFSET` preserves their individual references/directions. `zero J` records each matching joint's own stationary angle. Neither command copies one leg's settings onto another.

After verifying alignment, directions and free travel for every joint, return to reference and explicitly confirm:

```text
cal 0
cal 1
cal 2
check all
show all
off
save
```

Wait for Ready after each return move, or allow settling if already at reference. `check all` is your confirmation of all12, not an automatic measurement. It rejects without changing any checks if even one joint is detached, moving or away from its own reference. Alternatively `check J` confirms just the four matching joints. Changes to zero/direction/limits invalidate the affected checks.

Group calibration preflights every recipient: if even one leg rejects a move, limit or confirmation, **none** is changed. During a group move, wait for all legs to finish before selection/calibration changes; stop/off remain global. Group `limits J MIN MAX` requires all legs released and valid limits containing each leg's own zero. Use single-leg limits when travel differs. STOP cancels remaining staged activations and holds attached joints; OFF or BLE disconnection releases all12. An interrupted stage never restarts by itself; send off before restarting cal all/armall.

## Calibrate twelve joints individually

Startup selects **leg1**. Use `leg 1`, `leg 2`, `leg 3` or `leg 4` to select one leg while stationary. Joint numbers remain local **0 hip,1 knee/thigh,2 foot**. For example `leg 4`, then `cal 2`, addresses PCA9685 channel11.

The reference pose for each leg is **hip straight outward from the body, thigh horizontal, lower link vertically down**. Secure/support the assembly. First activation can jump from an unknown shaft position, so align disconnected horns/linkages first if clearance is uncertain. Refit badly aligned horns with motor power off.

Example for **leg1, joint0**; send lines separately and wait for movement to finish:

```text
leg 1
cal 0
show
move 0 88
zero 0
test 0 5
test 0 0
check 0
off
```

`move 0 88` is only an example adjustment. Use small changes until that joint physically matches its reference, then `zero J` records its stationary raw command. A fresh `cal` activation replies OK without Ready; allow the motor to settle. Gradual moves print Ready when their commanded targets finish.

Positive directions, checked separately for every leg:

- Hip J0: counter-clockwise viewed from above the body.
- Knee/thigh J1: lift the thigh.
- Foot J2: fold toward the body.

If a direction is reversed, return with `test J 0`, send `flip J`, and repeat `test J 5` and `test J 0`. Send `check J` at reference only after physically confirming alignment/free motion/direction. Checked flags are human confirmations, not measurements.

Repeat for J1 and J2, then for legs2–4. `off` always releases **all** legs; it preserves calibration/check flags in memory. `cal J` only activates its selected joint; previously attached joints keep holding. Do not copy leg1's direction signs or references blindly to mirrored legs.

Initial limits are60–120°, reference90°, nominal speed10°/s. With that selected leg released, use `limits J MIN MAX` to narrow to verified free travel, then re-check. Absolute bounds are45–140° and must contain its zero; changes to limits/zero/direction clear that joint's check. `speed N` accepts1–20°/s while stationary; `leg all` followed by `speed 5` applies one speed to all four legs.

The angle scale is the earlier leg controller's544–2400µs scale, independently constrained to1000–2000µs: nominal90° requests1472µs. It differs from the standalone motor tester's1500µs centre. Nominal angles/pulses are not measured physical degrees. PCA9685 oscillator error and tick quantization affect actual pulses. No automatic end-stop seeking or assembled-leg full-range sweep is provided.

## Run all four together

After physically calibrating/checking all12 joints, release and save:

```text
off
save
show all
```

Then send these **individually**:

```text
leg all
speed 5
armall
```

Wait for **Ready** after `armall` (nominally about1.2seconds). It explicitly activates one checked joint at its stored reference every100ms, with the body supported. It requires every joint checked and all outputs initially off. It does not auto-restart if interrupted. STOP cancels remaining activations but leaves those already active holding; send off before another armall attempt.

Now send each motion and wait for Ready before the next:

```text
stand
straight
stand
step 1
```

- `stand`: each foot85mm outward and60mm below its local hip axis.
- `straight`: partial extension to125mm outward and30mm below; retain a knee bend.
- `step 1`: one local20mm end-to-end,12mm-lift cycle, returning to standing. `step 2`/`step 3` are finite repeats.

All four tests preflight **all twelve** joints and the entire waypoint path. If even one is unchecked, detached, moving or outside a required limit, nothing starts. All four share a tick origin. At each step waypoint, faster legs wait for the last leg before the next phase. Different starting positions/speeds can produce different arrival times; this coordinates commanded waypoints, not measured foot contact or physical shaft arrival. There is no endless walking loop.

The geometry remains estimated:27.5mm hip offset,55mm thigh and74mm lower link, in LegTest.h. Measure the fitted assembly. Tests use the same local motion on every leg; they do not automatically produce global forward travel or balance the body. All four feet can lift together: **keep the body externally supported**. Stop/release and remove power if a motor buzzes, binds, stalls or slips.

To test one leg independently, select `leg N` when stationary and use its same `stand`, `straight` or `step` commands. `leg all` applies calibration commands to all four matching joints, as described above. During any coordinated move, test or staged activation, selections/configuration changes are rejected until it finishes or STOP/OFF cancels it. Read-only status and safety commands remain available.

## Commands and storage

| Command | Scope |
|---|---|
| `leg 1..4` / `leg all` | Select one leg or the coordinated test group; stationary only. |
| `help`, `show`, `show all` | Commands/status; all-status does not change selection. |
| `cal J`, `move J DEG`, `zero J` | Selected leg, or all four matching joints in all mode: activate/reference, raw angle, record each reference. |
| `cal all` | In leg all mode, all outputs off: staged calibration activation at individual references, no prior checks required. |
| `test J OFFSET`, `flip J`, `check J` | Selected leg, or all four matching joints: relative move using own zero/direction, reverse, confirm. |
| `check all` | In leg all mode: explicitly confirm all12; all must be attached, stationary and at their own references. |
| `limits J MIN MAX` | Selected leg, or all four matching joints; recipients must be released; invalidates their checks. |
| `speed N` | Selected leg, or every leg in all mode; stationary only. |
| `armall` | All12 checked, all off: staged reference activation, independent of selection. |
| `stand`, `straight`, `step [1..3]` | Selected leg, or coordinated four-leg test in all mode. |
| `stop` | **All legs**: cancel targets/sequences/staged activation, hold current commands. |
| `off`, urgent `!` | **All12 outputs**: inhibit OE before cleanup. Fresh cal/armall needed afterwards. |
| `save`, `load` | All outputs off and staging stopped; all four calibrations saved/loaded together. |

Settings use a distinct **`spyder-four`** Preferences namespace, key `cal`: one134-byte versioned record with per-leg and whole-record CRC/semantic validation. All four are validated before applying. Boot load restores settings only; it never activates a motor. Previous one-leg settings are not automatically imported or replicated. Keep all12 physically checked after wiring/horn/mechanical changes.

## Failure behavior and verification

Send one command at a time. `!` works without Enter and discards its line; send a fresh blank newline afterwards. Overlong/non-ASCII input, stale partial lines after2seconds, UART errors, RX overflow or failed reply delivery hold **all** motion and clear pending input. Safety clears both sources' partial/queued data, but cannot recall bytes in flight. Repeated macros and pasted motion batches are inappropriate.

I2C/setup errors inhibit OE, release all controller state and latch a fault requiring reset. Only channels0–11 may activate;12–15 stay off. Updates are nonblocking and make no catch-up jumps after a delayed loop. Replies run in a separate task; excessive reply traffic holds motion instead of stalling the control loop.

`bash scripts/test-esp32-four-leg.sh` runs portable controller/phase/storage and actual-sketch/PCA boundary checks with sanitizers. `bash scripts/test-esp32-ble-compat.sh` verifies both BLE string APIs. `bash scripts/build-esp32-four-leg.sh` compiles the original ESP32 Arduino target without uploading. See the [group-calibration verification report](../../docs/verification/2026-10-28-four-leg-group-calibration.md) for checks, actual core versions and remaining limits. Host doubles do not prove real radio behavior, reset/OE bias, pulse accuracy, supply adequacy, mechanics or balance.

Published at the owner's request as a bench-test candidate; physical validation remains pending. Report the full show-all output, each joint's alignment/direction, supported stand/straight/step observations, STOP/OFF/disconnect behavior, and actual power-source/converter ratings.

## Code overview

The [firmware and function guide](../../docs/firmware-guide.md) explains this sketch's commands, main classes/functions, storage, input handling and safety. The [sketch index](../README.md) compares all five ESP32 upload options.

# ESP32 firmware and function guide

All four sketches use an original ESP32, a PCA9685 at0x40, SDA21/SCL22/OE23 and USB115200/Newline. Each sketch is independently uploadable through Arduino IDE. Choose a complete folder from the [sketch index](../firmware/README.md); files from different folders are not interchangeable.

## Single unloaded servo

Folder: [spyder_esp32_servo_test](../firmware/spyder_esp32_servo_test/README.md). Wireless name: **Spyder-Servo-Test**, Classic SPP for Android/Windows; USB works independently. Controls channel0 only. Use to observe positional versus continuous rotation and test an unloaded motor before fitting a leg. Holding a position does not prove an exact180° versus360° travel range.

| Command | Function |
|---|---|
| `help`, `show` | Available commands, armed/fault state, requested pulse and target |
| `arm` | Enable channel0 at1500µs; first attachment can jump |
| `angle N`, `pulse N` | Gradual nominal0–180° or1000–2000µs request while armed |
| `center` | Gradual return to1500µs |
| `1`, `2`, `3` | Immediate1300/1500/1700µs request for2seconds, then release; arm before each |
| `sweep` | One gradual center→1000→2000→center pass, then hold; unloaded motor only |
| `stop` | Cancel test/sweep/target and hold current command |
| `off`, urgent `!` | Disable output; ! needs no Enter and discards its line |

`ServoBench.cpp` contains `Bench::command()` for validation/state changes, `Bench::tick()` for nonblocking motion/timed tests, `hold()` for cancellation, `release()` for signal removal and `fault()` for latched output failure. `Inputs::feed()/expire()/disconnect()` assemble separate USB/radio lines and cancel unsafe input. `pulseTicks()` converts pulse requests to PCA ticks with inward endpoint rounding. `PcaOutput.cpp` implements checked enable/write/disable, configuration checks and the disconnect-versus-enable guard. Nothing is saved across reset.

## One leg — Classic Bluetooth

Folder: [spyder_esp32_leg_test](../firmware/spyder_esp32_leg_test/README.md). Wireless name: **Spyder-Leg-Test**, Classic SPP for Android/Windows. Controls hip0, knee1 and foot2 on channels0/1/2. The leg's body mount must be supported.

| Command | Function |
|---|---|
| `help`, `show` | Command list and per-joint commanded position, reference, limits, direction and checks |
| `cal J` | Attach J at its stored reference; an already attached J returns gradually |
| `move J DEG` | Gradually request a raw nominal angle inside J's limits |
| `zero J` | Record the stationary requested angle as J's reference; clear its check |
| `test J OFFSET` | Move relative to J's own reference and direction; offset0 returns to reference |
| `flip J` | Reverse J's direction; clear its check |
| `check J` | Record human alignment/direction confirmation; attached, still, at reference required |
| `limits J MIN MAX` | Change J's allowed raw angles with all outputs off; clear its check |
| `speed N` | Set1–20°/s while stationary |
| `stand`, `straight` | Supported standing pose or partial extension; all three checked/attached and in bounds required |
| `step [1..3]` | Finite local step demonstration; full path checked before starting |
| `stop`, `off`, `!` | Hold/cancel, release all three, or urgent release without Enter |
| `save`, `load` | Persist/restore all three calibrations with outputs off; load never attaches |

J=0 hip,1 knee/thigh,2 foot. Reference: hip outward, thigh horizontal, lower link vertical. Positive motion: hip counter-clockwise from above, knee lifts thigh, foot folds toward body. `check` is an operator assertion, not position feedback.

`LegTest.cpp` implements `command()` validation/calibration, `tick()` bounded-rate updates, `solve()` inverse kinematics, `targetsFor()` zero/direction/limit conversion, `startPose()/startSteps()` full-path preflight and finite sequences. `stop()/release()` cancel motion or detach. `validSettings()/encodeSettings()/decodeSettings()` check semantic bounds, version and CRC. `CommandInputs.cpp` assembles separate USB/Bluetooth lines, recognizes urgent safety input and handles expiry/overflow. `PcaLegOutput.cpp` handles three checked hardware outputs; `LegSafety.h` inhibits OE before release cleanup. `NvSettings.cpp` saves/loads a32-byte record in Preferences namespace **spyder-leg**, key **cal**.

## One leg — BLE for iPhone

Folder: [spyder_esp32_ble_leg_test](../firmware/spyder_esp32_ble_leg_test/README.md). Wireless name: **Spyder-Leg-BLE**. Same channels, joint commands, geometry, storage and safety as the Classic one-leg sketch. The transport is Nordic UART Service BLE, with a bundled API compatibility fix for ESP32 cores2/3.

Connect **inside Bluefruit Connect UART or another NUS app**, enable TX notifications, and send newline-terminated text. Do not pair through iOS Bluetooth Settings. RX UUID ends0002; TX UUID ends0003 under service `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`. Exact UUIDs and hex-write examples are in its README.

`BleUart::begin()` creates the service/characteristics. `onWrite()` receives command bytes or subscription changes. `ready()` requires a connected, subscribed client; `write()` sends paced20-byte notifications for the expected session epoch; `poll()` restarts advertising. Disconnect/unsubscribe callbacks inhibit outputs before controller cleanup. Epoch checks prevent queued old replies crossing subscriptions. Safety commands remain effective without subscription; movement/configuration requires it. The remaining controller/PCA/storage modules have the roles described above.

## Four legs — current BLE controller

Folder: [spyder_esp32_ble_four_leg_test](../firmware/spyder_esp32_ble_four_leg_test/README.md), **v1.1**. Wireless name: **Spyder-4Leg-BLE**. Leg1 uses0/1/2, leg2 uses3/4/5, leg3 uses6/7/8 and leg4 uses9/10/11. Each triple is hip/knee/foot;12–15 stay off. Physical corners previously chosen: front-left1, front-right2, rear-left3, rear-right4. Directions still require physical calibration; no automatic mirroring is inferred.

It includes all one-leg commands plus:

| Command | Function |
|---|---|
| `leg 1..4`, `leg all` | Select a single leg or the four-leg group while stationary |
| `show all` | Report all twelve without changing selection |
| `cal all` | In all mode, with every output off: arm at each saved reference, one joint every100ms; no previous checks required |
| `cal J`, `move J DEG`, `test J OFFSET`, `zero J`, `flip J`, `check J`, `limits J MIN MAX` | In all mode, act on four matching joints after validating every recipient; keep each leg's own settings |
| `check all` | In all mode, record operator confirmation for all twelve only when attached, still and at their own references |
| `armall` | Activate all twelve, one every100ms, only if all are checked and initially off; independent of selection |
| `stand`, `straight`, `step [1..3]` | Selected leg, or coordinated four-leg motion in all mode; all recipients/path limits preflighted |
| `speed N` | Selected leg, or all four in all mode; stationary only |
| `stop`, `off`, `!` | Always global: cancel/hold or release all twelve, including pending staged attachments |
| `save`, `load` | All-four calibration record, only while all outputs off and staging stopped |

Group moves reject entirely if any recipient fails validation. Step phases wait for the slowest leg. `cal all` never confirms checks automatically. To correct one mirrored joint, select its leg; group `flip J` reverses all four. Interrupted stages never replay on reconnect.

`FourLegTest::command()` routes selections, checked/unchecked staged arming, atomic calibration broadcasts and group poses. `MappedOutputs` maps local joints to global channels. `FourLegTest::tick()` services stages and synchronized phase barriers; `stop()/release()` cancel every leg. `LegTest::validateCommand()` runs the real command handler against a value copy with silent outputs so preflight cannot touch hardware or live state. `canStart()/atTarget()/advanceStep()/resetClock()` support group validation and shared timing. `PcaOutputs` tracks all12 outputs with a16-bit mask; `FourLegSafety` inhibits OE before cleanup. Four-leg `encodeSettings()/decodeSettings()` validate a134-byte aggregate with per-leg and outer CRCs; `NvSettings.cpp` uses separate namespace **spyder-four**, key **cal**. One-leg records are not automatically imported.

## Sketch entry points and shared boundaries

Each `.ino` owns `setup()` (queues, hardware checks, radio start, settings load where supported; outputs remain off) and `loop()` (safety servicing, bounded USB/radio reads, parser expiry, periodic I2C health checks and motion ticks). `processCommand()` in leg sketches handles help/status/storage before delegating movement. `transmitReplies()` runs separately so a blocked transport cannot stall motion/safety handling; reply epochs invalidate stale responses. `serviceSafetyEvents()` handles radio releases, corrupt input and latched hardware faults before more motion.

The sketches deliberately keep companion modules in their own folder for Arduino IDE portability. When changing shared behavior, inspect the relevant copies and run every affected suite.

## Angles, geometry and test limits

The motor tester maps nominal0/90/180° to1000/1500/2000µs. Leg sketches retain the544–2400µs angle scale, so90° requests1472µs, with output independently restricted to1000–2000µs. Do not transfer nominal angle calibrations between those scales. Leg defaults: zero90°, raw limits60–120°, absolute permitted45–140°. Nothing measures actual shaft position or discovers end stops.

Estimated link lengths are27.5/55/74mm. Stand target is(85,0,-60)mm and partial straight target(125,0,-30)mm; local step is20mm end-to-end with12mm lift. Measure the fitted geometry before relying on these targets. Four-leg simultaneous steps lift all feet and require external body support; balanced gait and six-leg motion are unimplemented.

# ESP32/PCA9685: four-leg BLE walking controller v1.1

This sketch is the [four-leg BLE controller v1.1](../spyder_esp32_ble_four_leg_test/README.md) with a modeled crawl gait added. Physical balance has not been verified. Every v1.1 command, check and safety behaviour is unchanged. On top of them it adds:

| Command | What it does |
|---|---|
| `walk [N]` | Walk forwards. N cycles (1–100) or, with no number, until `halt`. |
| `back [N]` | Walk backwards. |
| `left [N]` / `right [N]` | Turn on the spot. |
| `halt` | Ramp down, then settle to standing; may take over6seconds and several steps. An unsafe stopping path is rejected; use stop/off. |
| `stop`, `off`, `!` | Same as v1.1: hold immediately, or release all twelve servos. |

A walking command sent while the robot is walking requests a validated direction transition, or rejects it without changing the current walk. Wiring, power, Arduino IDE setup and the iPhone app are exactly as in the [v1.1 README](../spyder_esp32_ble_four_leg_test/README.md). The BLE name is **`Spyder-Walk-BLE`**.

## How it walks

The gait is a crawl, with only one foot in the air at a time:

- Between steps, all four feet are down and the body leans away from the next leg to lift. The model places the body centre inside the other three feet's support triangle; the real centre of mass, contact and slip must be checked.
- The lifted foot swings forward, then all feet push back together while the next leg takes its turn.
- Legs step in the order rear-right, front-right, rear-left, front-left. A full cycle (all four legs) takes 6 s.
- Walking starts by moving to the v1.1 `stand` pose (85, 0, −60 mm). The stride grows over the first cycle and shrinks after `halt`, and an accepted completion requests standing; actual foot contact and balance are unverified.

Before accepting a walking command, the controller samples a copy of the requested startup/direction transition and cycle against every joint's saved limits, including the planned stop for finite walks. Halt validates its current stopping path before changing the live gait. An unsafe command is rejected with the current motion unchanged; use stop/off for immediate cancellation. These sampled checks do not prove every possible timing trajectory: each live frame validates all four legs before any write, and a runtime rejection explicitly reports an error and holds all legs.

## Before walking

1. Upload with servo power off. Use the same libraries and board settings as v1.1.
2. **Calibration is shared with the four-leg test sketch** (Preferences `spyder-four`/`cal`), so you only calibrate once. Calibrate and `save` with either sketch, following the v1.1 README's calibration steps:
   - `leg all`, `cal all`
   - `test`/`flip`/`zero` for each joint
   - `check all`, then `off`, then `save`
3. Support the body. Send `leg all`, then `armall`, wait for `Ready`, then `stand` and wait for Ready again.
4. Send `walk 1` and watch one full cycle before trying longer walks. Keep `stop` or `!` ready.

**The leg corners must be correct.** Legs 1–4 are front-left, front-right, rear-left and rear-right, as in `docs/project-state.md`. If a leg is mounted somewhere else, change `LegMountDeg` and `StepOrder` in `WalkConfig.h`. Each joint's direction must also be calibrated so that a positive `test` means:

- hip: counter-clockwise from above
- knee: thigh up
- foot: folds in

Otherwise the robot walks crooked or falls.

## Tuning: `WalkConfig.h`

| Setting | Default | Effect |
|---|---|---|
| `StrideMm` | 20 | Step length. Larger is faster but needs wider joint limits. |
| `LiftMm` | 15 | Foot clearance. |
| `CycleMs` | 6000 | Time for all four legs to step once. |
| `BodyShiftMm` | 18 | Lean towards the three supporting feet. Raise it if the robot tips towards the lifted leg. |
| `TurnDeg` | 10 | Body rotation per cycle for `left`/`right`. |
| `LegMountDeg`, `HipRadiusMm` | ±30°/±150°, 80 mm | Where the hips are on the body. |
| `WalkMaxStepTenths` | 24 | Joint speed cap while walking (120°/s). |

Link lengths (27.5/55/74 mm) are estimates in `LegTest.h`. Measure the fitted legs and update them if they differ.

## Verification and limits

`bash scripts/test-esp32-walk.sh` runs host checks with fake hardware. They cover:

- **One foot at a time:** never more than one foot is lifted.
- **Balance:** the body centre stays at least 11 mm inside the support triangle.
- **Joint limits:** every joint stays inside the default ±30° (worst case 28°, when walking backwards).
- **Joint speed:** no joint needs more than 72°/s, under the 120°/s cap.
- **Commands:** walk, back, left and right with preflight rejection, halt, stop, release, and BLE loss.
- **Regressions:** the copied v1.1 controller still passes the four-leg suites.

Actual ESP32 build results and integration checks are recorded in the [merge verification report](../../docs/verification/2026-10-28-esp32-walk-merge.md). Nothing has been tested on the real robot yet. Geometry, leg corners, directions and the power supply all need bench checks. The PP3 + 3 A buck in the v1.1 README is not enough to walk.

## Complete folder and first run

Open **spyder_esp32_ble_walk.ino** from this complete folder in Arduino IDE, keeping every .h/.cpp beside it. Board: original ESP32 Dev Module; USB Serial Monitor:115200/Newline. Libraries: Adafruit PWM Servo Driver3.0.2 and BusIO1.17.4, plus the ESP32 core's bundled BLE/Wire/SPI/Preferences. Connect inside Bluefruit Connect UART to **Spyder-Walk-BLE**, with notifications enabled. The PCA map remains leg1=0/1/2,leg2=3/4/5,leg3=6/7/8,leg4=9/10/11; SDA21/SCL22/OE23. See the [hardware guide](../../docs/hardware.md) for the external5V rail,3V3 logic and verified OE pull-up.

With valid saved calibration, body supported and an adequate supply, send these **individually**, waiting for Ready after armall, stand and walking:

```text
leg all
show all
armall
stand
walk 1
```

If armall rejects unchecked joints, follow the linked calibration procedure; do not mark checks without inspecting every joint. `speed 5` affects calibration/test poses, not walking's separate gait timing/speed cap. Start with finite `walk 1`; unnumbered walk/back/left/right continues until halted or stopped. `halt` ramps down before returning to stand; it may perform several more steps and is not immediate. If it replies ERR unsafe halt path, the current walk continues: use stop/off. Use `stop` for immediate hold/cancellation or `off`/`!` to release all signals. Keep physical servo power accessible and use a support that prevents a fall.

Calibration is shared across the walking and four-leg test sketches, but not with one-leg sketches. Changing geometry, wiring or horn alignment requires physical rechecking. Full calibration command/function descriptions remain in the [four-leg README](../spyder_esp32_ble_four_leg_test/README.md) and [firmware guide](../../docs/firmware-guide.md).

Version1.1 integration adds stop/direction lifecycle preflight, atomic frame validation and explicit runtime limit-error reporting. The gait geometry/configuration from Ryan's original commit0447bac is retained.

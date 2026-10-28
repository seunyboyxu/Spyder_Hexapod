# ESP32/PCA9685: four-leg BLE walking controller v1.0

This sketch is the [four-leg BLE controller v1.1](../spyder_esp32_ble_four_leg_test/README.md) with a balanced walking gait added. Every v1.1 command, check and safety behaviour is unchanged. On top of them it adds:

| Command | What it does |
|---|---|
| `walk [N]` | Walk forwards. N cycles (1–100) or, with no number, until `halt`. |
| `back [N]` | Walk backwards. |
| `left [N]` / `right [N]` | Turn on the spot. |
| `halt` | Finish the current step and settle back to standing. |
| `stop`, `off`, `!` | Same as v1.1: hold immediately, or release all twelve servos. |

A walking command sent while the robot is walking changes direction smoothly. Wiring, power, Arduino IDE setup and the iPhone app are exactly as in the [v1.1 README](../spyder_esp32_ble_four_leg_test/README.md). The BLE name is **`Spyder-Walk-BLE`**.

## How it walks

The gait is a crawl, with only one foot in the air at a time:

- Between steps, all four feet are down and the body leans away from the next leg to lift. This keeps its weight over the other three feet.
- The lifted foot swings forward, then all feet push back together while the next leg takes its turn.
- Legs step in the order rear-right, front-right, rear-left, front-left. A full cycle (all four legs) takes 6 s.
- Walking starts by moving to the v1.1 `stand` pose (85, 0, −60 mm). The stride grows over the first cycle and shrinks after `halt`, so the robot always ends standing.

Before any servo moves, the sketch checks a whole cycle against every joint's saved limits. If any position falls outside them, the command is rejected and nothing moves.

## Before walking

1. Upload with servo power off. Use the same libraries and board settings as v1.1.
2. **Calibration is shared with the four-leg test sketch** (Preferences `spyder-four`/`cal`), so you only calibrate once. Calibrate and `save` with either sketch, following the v1.1 README's calibration steps:
   - `leg all`, `cal all`
   - `test`/`flip`/`zero` for each joint
   - `check all`, then `save`
3. Support the body. Send `armall`, wait for `Ready`, then `stand`.
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

No ESP32 build has been run: the cloud environment cannot download the ESP32 toolchain. Nothing has been tested on the real robot yet. Geometry, leg corners, directions and the power supply all need bench checks. The PP3 + 3 A buck in the v1.1 README is not enough to walk.

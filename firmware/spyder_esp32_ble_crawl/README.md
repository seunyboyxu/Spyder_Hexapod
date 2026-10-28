# ESP32/PCA9685: four-leg BLE crawl controller v1.0 (quadpod gait)

This sketch is the [walking controller](../spyder_esp32_ble_walk/README.md) with its gait generator swapped for the step sequences of [hpsaturn/quadpod](https://github.com/hpsaturn/quadpod/blob/master/src/servos.cpp). That code's header credits SunFounder's 2015 crawling-robot gait, which is also the basis of most RegisHsu-style 12-servo spider builds. Everything else (BLE, calibration, arming, preflight, `stop`/`off`/`!`) is byte-for-byte the walking sketch's, and the test script checks that. **Physical walking has not been verified.**

The commands are the same as the walking sketch:

| Command | What it does |
|---|---|
| `walk [N]` / `back [N]` | Crawl forwards or backwards. N cycles (1–100), or until `halt` |
| `left [N]` / `right [N]` | Turn on the spot |
| `halt` | Finish the current move, then return to standing, lifting one foot at a time |
| `stop`, `off`, `!` | Hold immediately, or release all twelve servos |

The BLE name is **`Spyder-Crawl-BLE`**. Calibration is shared with the four-leg test and walking sketches (Preferences `spyder-four`/`cal`), so calibrate once with any of them. First run is the same as the walking sketch: `leg all`, `armall`, `stand`, then `walk 1`, with the body supported and `stop` ready.

## What is taken from quadpod, and what is not

**Taken: the sequences.** Each foot sits −1, 0 or +1 "steps" from its stance point. A cycle runs as follows:

1. Lift one leg from −1 to +1.
2. Move the body (every foot −1).
3. Lift two more legs.
4. Move the body again.
5. Lift the last leg.

The lift order is quadpod's, mapped to Spyder's legs:

| Direction | Lift order |
|---|---|
| Forward | FL, RR, FR, RL |
| Back | RL, FR, RR, FL |
| Left | RL, RR, FR, FL |
| Right | FL, FR, RR, RL |

A host test checks the steady order against these. Lifts are straight up, across and down at a constant speed, as in quadpod.

**Not taken:**
- **quadpod's servo layer.** `polar_to_servo` holds per-leg magic offsets tuned to its author's build, with integer-division scaling, so it doesn't transfer. Spyder's own IK and calibration (`LegTest`) turn foot positions into pulses.
- **quadpod's code.** None is copied. Only the sequences are re-expressed in Spyder's body frame. Attribution is in `Walk.cpp` and [upstream](../../docs/reference/upstream.md).

**Added for Spyder's body:**
- **Lean before each lift.** quadpod's stance is square and wide. Spyder's front and rear hexagon stations give a long, narrow stance: unmodified, the body centre sits only about 5 mm inside the support triangle at this stride. So before each lift the body leans, with all feet down, until the centre is at least `MinMarginMm` (12 mm) inside the other three feet. The lean is capped at `MaxLeanMm`.
- **Start, stop and direction changes.** These move one foot at a time with the same lean rule. A direction change returns to standing first, so no foot is ever placed for a mix of two directions.

## Tuning: `WalkConfig.h`

| Setting | Default | quadpod | Notes |
|---|---|---|---|
| `StrideMm` | 20 | 40 (`y_step`) | Body moves 2× per cycle. Larger needs wider joint limits |
| `LiftMm` | 10 | 20 | The default ±30° knee/foot limits allow about 10 at the 85/−60 mm stand |
| `TurnDeg` | 6 | — | Rotation per step about the body centre |
| `LegSpeedMmS` / `BodySpeedMmS` | 80 / 40 | — | Gives a cycle of about 5 s walking, 4 s turning |
| `MinMarginMm` / `MaxLeanMm` | 12 / 25 | — | Raise the margin if it tips towards the lifted leg |
| `StanceYawDeg` | 0 | — | Spreads feet sideways like quadpod's stance; needs hip limits wider than ±30° |
| `LegMountDeg`, `HipRadiusMm` | ±30°/±150°, 80 mm | — | Where the hips are on the body |
| `*Order` | quadpod's | — | Change only if the legs are mounted elsewhere |

`CycleMs` is the controller's preflight window unit, not the cycle time. Link lengths (27.5/55/74 mm) are estimates in `LegTest.h`; measure the fitted legs.

## Verification

`bash scripts/test-esp32-crawl.sh` runs host checks with fake hardware (ASan/UBSan):

- Every 20 ms frame of 3 cycles in each direction, and of five direction changes:
  - only one foot up at a time
  - body centre at least 12 mm inside the support triangle
  - all joints within ±27°, inside the default ±30° limits
  - no joint faster than 93°/s, under the 120°/s cap
  - always ends exactly at the stand pose
- The steady lift order matches quadpod for all four directions.
- Controller: preflight rejection, finite walks returning to the same standing pulses, direction change with a count, halt, stop, release, and over-tight limits rejected over BLE.
- The copied four-leg suites still pass.
- All shared files are byte-identical to `spyder_esp32_ble_walk`.

Not yet done: an ESP32 compile of this folder (run `bash scripts/build-esp32-crawl.sh` or open it in Arduino IDE), and anything physical. Centre of mass, foot slip, servo torque and the power supply are all unverified. A battery mounted off-centre moves the real centre of mass, and the 12 mm margin assumes it is at the body centre.

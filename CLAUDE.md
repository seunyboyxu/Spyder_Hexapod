# Spyder: notes for the next Claude agent

Claude Code reads this file automatically. It records where the standing/walking work stands as of 23 October 2026, so a new session (from any account) can carry on without the original chat. For the older background, read `docs/project-state.md` and `docs/handover.md`. Those describe the calibration firmware and predate the walking work below.

## Owner's rules

- **Don't edit existing code.** Put new standing/walking code in new files or new sketch folders. Leave `firmware/spyder_uno/`, `web/`, the existing tests and Seun's `servo_and_standing_calibration.ino` untouched.
- **Arduino only.** The target is an Arduino Uno with a Sensor Shield V5 and plain `Servo.h`, with no PCA9685.
- Work on a branch and open a PR. Don't push to `main`.
- Never upload firmware or move motors yourself. The owner tests on the robot and reports back.

## Hardware as built

- Four legs, 12 servos (SG90 plus some MG90D), each leg with hip, knee and foot joints.
- **Pin map**, from Seun's sketch, which matches the physical wiring:

  | Leg | Hip | Knee | Foot |
  |---|---|---|---|
  | 1 | D2 | D3 | D4 |
  | 2 | D5 | **D1** | D7 |
  | 3 | D8 | D9 | D10 |
  | 4 | D11 | D12 | D13 |

  Leg 2's knee was moved to **D1** (USB serial TX), apparently because the D6 header didn't work. Any sketch using D1 for a servo must not transmit on Serial: receive only, and clear `TXEN0` after `Serial.begin`. That servo also twitches during upload.
- Which physical corner each leg 1–4 sits on hasn't been confirmed yet. The owner was asked.
- **Power:** the recommended setup is a 3S LiPo → 5 V 3 A buck → the shield's servo terminal, with the shield's power jumper removed. The Uno is powered separately. During testing the owner was still on a 4.5 V pack or 6×AA, which is weak.
- Mechanical: horns have slipped off splines when joints were driven past their stops. Use small test moves of 15–20°.

## Standing pose that works

Seun's `servo_and_standing_calibration.ino` stands the robot with these `Servo.write()` angles:

| Leg | Hip | Knee | Foot |
|---|---|---|---|
| 1 | 90 | 90 | 50 |
| 2 | 70 | 140* | 50 |
| 3 | 110 | 90 | 0 (was -5) |
| 4 | 90 | 75 | 25 |

\*Because Seun's sketch calls `Serial.begin`, the leg 2 knee on D1 never actually received a pulse, so 140 was never tested. Driving it to 140 forced that knee past its stop. The simple walk sketch uses 90 for it until the owner tunes a real value.

## Walking code: PR #1, branch `walking-gait`

1. **`firmware/spyder_simple_walk/spyder_simple_walk.ino`.** This is the one in use. It's a single file in Seun's style and starts from the standing pose above, with no calibration step.
   - It runs a joint-space creep gait: each hip swings ±`HIP_SWING`/2 around standing, a knee lifts by `KNEE_LIFT` during its leg's step, and one leg is up at a time in `STEP_ORDER`. Stride ramps up over the first cycle and down over the last, so walks start and end standing.
   - Serial Monitor at 9600, receive only: `stand`/`s`, `forward`/`w [n]`, `back`/`b`, `left`/`a`, `right`/`d`, `stop`/`x`, `off`/`o`, `check`/`c`, and `hip|knee|foot <leg> <deg>` for live tuning of standing angles. Live values aren't saved.
   - `CHECK_DIRECTIONS 1` runs a check at boot: each knee should lift the foot and each hip should swing the foot forward. Wrong legs get their entry flipped in `KNEE_UP` or `HIP_FORWARD`. **These signs and `LEFT_SIDE` are still guesses** waiting on the owner's check.
   - An optional button between A0 and GND walks forward `WALK_CYCLES`.
2. **`firmware/spyder_walk/`.** This is the earlier, fuller version: IK with leg lengths 27.5/55/74 mm estimated from the CAD, a body-lean creep gait, trims saved in EEPROM at 512+, and commands at 115200. It assumes consecutive pins D2–D13 and **does not match the current D1 wiring**, so it's on hold. PC tests are in `tests/walk/`, run with `bash scripts/test-walk.sh`.

## Verifying changes without hardware

There's no CI. To check a sketch:

- **Compile for the Uno** with `avr-g++` (apt: `gcc-avr avr-libc arduino-core-avr`, plus Servo from `github.com/arduino-libraries/Servo` tag 1.2.2).
  - The Arduino IDE adds function prototypes above the first function, so any struct used in a function signature must be declared in a header. That rule came from the "'Saved' does not name a type" bug.
- **Run it in simavr** (apt: `simavr libsimavr-dev libelf-dev`) and measure the pulse widths on each pin after sending commands over UART. `tests/avr/waveform.c` shows the harness pattern.
  - 544 µs is 0°, 1472 µs is 90° and 2400 µs is 180°.

## Next steps

1. The owner refits the leg 2 knee horn, tunes its standing angle with `knee 2 <deg>`, and copies the good values into `STAND_*`.
2. The owner runs the direction check and reports which legs are wrong, plus which leg is at which corner. Then fix `KNEE_UP`, `HIP_FORWARD`, `LEFT_SIDE` and `STEP_ORDER` (a crawl lifts rear, then front on the same side, then the other side).
3. Tune `HIP_SWING`, `KNEE_LIFT` and `CYCLE_MS` on the real robot. If it tips while a leg is up, it needs a body lean, which means shifting the stance hips away from the lifted leg.

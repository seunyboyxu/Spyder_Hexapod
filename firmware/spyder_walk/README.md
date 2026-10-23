# Spyder standing and walking sketch

`spyder_walk` is a standalone Arduino sketch for the four-leg Uno build. It uses inverse kinematics to stand, sit, walk forwards and backwards, and turn on the spot with a slow creep gait, where one foot is in the air at a time and the body leans away from it. It runs without a PC.

It is separate from the calibration firmware in `firmware/spyder_uno` and from `servo_and_standing_calibration.ino`, and changes neither. Upload one sketch at a time. This sketch keeps its trims at EEPROM address 512 and up, so it never overwrites the calibration firmware's saved settings (addresses 0–125).

## Before you start

- Power the servos from their own 5–6 V supply into the Sensor Shield's servo terminal, with the shield's power jumper removed. Power the Uno separately. Twelve servos cannot run from the Uno.
- Switch servo power off while uploading. The bootloader blinks D13, which is the L3 foot servo.
- Hold or prop up the body the first few times. `off` makes all servos go limp, so the robot drops.
- Optional: wire a push button between **A0** and **GND** to run it without a PC.

## Uploading

Open `firmware/spyder_walk/spyder_walk.ino` in the Arduino IDE. All files in this folder open as tabs. Select **Arduino Uno** and upload. Open the Serial Monitor at **115200 baud** with line ending set to **Newline**, and type `help`.

## Settings: `Config.h`

All lengths, leg directions, pins, poses and gait timing are in [`Config.h`](Config.h).

The leg lengths are estimates from the CAD: hip 27.5 mm, thigh 55 mm and lower leg 74 mm. Measure each one from servo shaft centre to servo shaft centre, and to the foot tip for the last segment. Update the numbers if they differ.

## One-time calibration

Calibrate with the robot held off the ground.

1. Send `cal`. Every servo moves to the **reference pose**: hips pointing straight out from the body, thighs level, and lower legs pointing straight down.
2. **Find each leg.** Send `leg 0` to `leg 3`. Each command raises one leg's thigh. Check that `LEG_MOUNT_DEG` in `Config.h` matches where that leg really is. The defaults are L0 front right, L1 rear right, L2 rear left and L3 front left.
3. **Trim each joint.** Joints are numbered `leg*3 + 0` (hip), `+1` (thigh) and `+2` (foot), so 0–11 in the same wiring order as the calibration firmware. Use `nudge <j> <deg>`, for example `nudge 4 -3`, until the joint sits exactly in the reference pose.
4. **Check directions.** Send `test <j> 20` for each joint:
   - A hip should swing counter-clockwise when viewed from above.
   - A thigh should lift.
   - A foot should swing in towards the body.

   If a joint moves the wrong way, send `flip <j>` and test again. Send `test <j> 0` to return the joint.
5. Send `save` to store the trims in EEPROM. Send `show` to print the values; you can paste them into `Config.h` so they also become the defaults.

## Running

| Command | What it does |
|---|---|
| `stand` | Attach the servos low, then rise to standing. |
| `walk [mm]` / `back` | Walk forwards or backwards. The optional value sets the stride. |
| `left` / `right` | Turn on the spot. |
| `stop` | Finish the current step, then stand still. |
| `sit` / `off` | Lower the body / make all servos limp. |
| `height <mm>` / `stride <mm>` | Adjust stance height and stride length. |

Button on A0: a short press steps through off → stand → walk → stop. Holding it for 1.5 s sits the robot down and switches the servos off.

## Tuning tips

- Start with the robot on a grippy surface and a slow `CYCLE_MS`. If it tips towards the lifted leg, raise `BODY_SHIFT_MM`.
- If a leg scrapes during a step, raise `LIFT_MM`. If the feet slip, lower `STRIDE_MM`.
- If the Uno resets or the servos twitch when several move at once, the servo supply is too weak.

## Tests

`bash scripts/test-walk.sh` compiles the kinematics and gait on a PC and checks them. The checks are:

- The inverse kinematics matches the forward kinematics.
- Only one foot is ever in the air.
- The body centre stays inside the triangle formed by the three feet on the ground, with at least 11 mm of margin at the default settings.
- Feet move smoothly.
- The gait stops with all feet down.

These checks use the estimated geometry. They don't prove the real robot balances.

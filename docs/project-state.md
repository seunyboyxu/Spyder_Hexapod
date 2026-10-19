# Current project state

Updated **19 October 2026**. Read this before the historical handover: later user choices below supersede its ESP32/six-leg starting instructions.

## What the project is

Spyder adapts the mechanical ideas of [hpsaturn/quadpod](reference/upstream.md) toward a six-leg, 18-SG90 robot. The current working milestone is deliberately smaller: **four installed legs, twelve servo outputs, Arduino Uno and Arduino Sensor Shield V5**. Each installed leg has three consecutively wired servos, from hip/proximal to foot/distal.

The repository contains a revision 3 calibration sketch, a desktop Web Serial console, a hardware-free demo and automated regression tests. It can deliberately arm individual joints, approach pulse targets slowly, store calibration and replay a captured standing record. This is commanded-pulse control; there is no measured joint position, contact sensing or automatic balance. Stable standing and walking have not been demonstrated here.

## Confirmed decisions

| Decision | Current choice |
|---|---|
| Mechanical mount | Fixed four-screw flange; the detachable dovetail/socket concepts were abandoned. |
| Servos | Owner reports 9 g SG90s and some MG90Ds (19 October). Exact quantities/brands and per-pin model assignments remain unrecorded; do not replace MG90D with an assumed MG90S. |
| Controller for this milestone | Arduino Uno + Sensor Shield V5; no PCA9685 is used by this sketch. |
| Signals | D2–D13 in twelve-joint wire order; D0/D1 reserved for USB serial. The original D0–D11 idea was replaced after discussion. |
| D13 | Retained by explicit owner choice. Switch servo power off during connection, reset and upload because the bootloader can toggle D13 before the sketch runs. |
| Power as last reported | Servos were drawing from Uno USB. This must be corrected before powered calibration; no external supply installation or load measurements have been confirmed. |
| Startup/recovery | Detached and hold latched; deliberate Resume and individual Arm required. No automatic standing pose on boot. |
| Compatibility | `SPYDER 3` firmware/console together; EEPROM and exported JSON schema remain version 1. |
| Six-leg electronics | Earlier ESP32 + two PCA9685 boards is a proposal for later work, not the current implementation. |

Leg labels L0–L3 preserve wiring groups. The four occupied body stations, physical front, and actual axis/sign convention are not yet recorded. Do not copy the old quadruped's FR/FL/RR/RL identities or its pitch/knee/yaw channel ordering into this map.

| Wire group | Joint IDs | Proximal / middle / distal signal |
|---|---|---|
| L0 | 0, 1, 2 | D2, D3, D4 |
| L1 | 3, 4, 5 | D5, D6, D7 |
| L2 | 6, 7, 8 | D8, D9, D10 |
| L3 | 9, 10, 11 | D11, D12, D13 |

## Evidence and limitations

The 16 October revision 2 run passed native sanitizer checks, 27 JavaScript tests, 10 Chromium workflows and 24 actual compiled-AVR simulation scenarios. The real Uno image used 11,492 bytes flash and 1,015 bytes static RAM. See the [verification record](verification.md) and [18 October handover checks](verification/2026-10-18-handover.md) for exact scope and fresh results.

The unwanted-movement audit reproduced stuck-HIGH release pulses, ignored Release while busy, delayed STOP, fractional movement loss, heartbeat starvation and a stale initial arming field. Those software defects were fixed and retained as regressions. The [historical audit](archive/2026-10-16-calibration-audit.md) describes the old version; it is not the current release verdict.

The selected seven print meshes and two native Fusion files are now available under [cad/](../cad/README.md). Six print parts have named counterparts in the complete Fusion assembly; the backing-pivot editable source remains missing. CAD bytes and archive integrity were checked, but Fusion itself, a complete fitted joint sweep, actual fastener retention and physical print fit were not verified here.

## Next owner actions

| Priority | Work still needed | Completion evidence |
|---|---|---|
| First | External regulated servo supply, common ground and verified shield rail/jumper isolation | Supply model/rating, wiring record and measured voltage/current during staged loads. |
| First | Check the installed horn, matching centre screw, passive pivot and free motion | Photos, fitted revision/parts and one retained unloaded joint passing its sweep. |
| Next | Map physical stations/front and calibrate every fitted joint | Completed [calibration log](templates/calibration-log.md), exported JSON and EEPROM save. |
| Next | Demonstrate four-leg standing under staged support/load | Recorded stance, surface/foot grip, supply readings, no observed slip or binding over a stated trial. |
| Next | Recover missing editable backing pivot and later fit packages | Inspected native/STEP files with source provenance and print correspondence. |
| Later | Define six-leg hardware and control design | Explicit board/output allocation, power budget, measured geometry and separate implementation plan. |

No hardware has been flashed or moved from this cloud workspace. Uploaded CAD and passing simulation do not establish that the robot already stands without slipping.

## Unloaded calibration update

Revision 3 adds individual nominal-90° centering and a single-pass unloaded sweep with a matching console/firmware. After reporting SG90/MG90D motors with no datasheets, the owner explicitly chose the existing 1000–2000 µs defaults with 1500 µs nominal center. Only the selected output may attach; STOP/lease loss cancels the sweep, and Release exits test mode. Saved leg limits stay unchanged; the selected travel-check flag and standing record are cleared in RAM. See the [unloaded guide](unloaded-servo-calibration.md) and [revision 3 verification](verification/2026-10-19-unloaded-servo.md). Full angular travel and physical response remain unverified.

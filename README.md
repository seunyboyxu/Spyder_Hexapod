# Spyder robot

Spyder is a six-leg mechanical robot project currently testing **four installed legs, twelve servo outputs for SG90/MG90D motors, an Arduino Uno and Sensor Shield V5**. This repository contains the motor-calibration firmware, a desktop browser console, selected print STLs, native Fusion 360 assemblies and the continuation handover.

**Current milestone:** revision 3 calibration software is tested in software and MCU simulation. Real-board calibration, load-tested standing without slipping and six-leg walking remain to be demonstrated. Read [current state](docs/project-state.md) before the historical six-leg/ESP32 proposal.

## Start here

| Goal | Read |
|---|---|
| Take over the project | [Contributor handover](docs/handover.md) |
| Understand what works and what is missing | [Current state](docs/project-state.md) |
| Continue the work | [Project plan](docs/project-plan.md) |
| Center or sweep a motor removed from the leg | [Unloaded servo instructions](docs/unloaded-servo-calibration.md) |
| Upload and calibrate assembled legs | [Bring-up and calibration instructions](docs/bring-up.md) |
| Find print files and editable Fusion counterparts | [CAD guide and part map](cad/README.md) |
| Build/test or change the software | [Development](docs/development.md), [protocol](docs/protocol.md) |
| Find the original GitHub | [hpsaturn/quadpod and provenance](docs/reference/upstream.md) |

## Try the console

Download/clone this repository. On the computer running desktop Chrome or Edge:

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory web
```

On Windows, use `python` if that is your Python command. Open `http://localhost:8000` and choose **Try demo** to explore without hardware. No npm installation is needed to serve the console. For the real Uno, first follow [bring-up](docs/bring-up.md), upload [`spyder_uno.ino`](firmware/spyder_uno/spyder_uno.ino) with **Servo 1.2.2**, close Serial Monitor and select **Connect Uno**. Upload the revision 3 sketch and use the matching console together; old firmware is rejected.

**Before powered calibration:** twelve servos must not draw power through Uno USB. Use a suitable external regulated servo supply, shared ground and verified shield rail/jumper isolation. Keep servo power off during USB connection, reset and upload. The owner chose to retain D13, which the Uno bootloader can toggle before the sketch starts. Support the body; first ARM can jump to its explicit pulse and Release can let the robot drop.

## Center and sweep an unloaded servo

In **Unloaded servo setup**, select one signal, confirm the motor is removed from its leg, use **Resume controls**, then **Center selected at 90°**. This commands a nominal **1500 µs**, without measuring the shaft angle. Once centered, **Sweep selected range** makes one pass from center to minimum, maximum and back to center. The owner chose the existing **1000–2000 µs defaults** for the SG90/MG90D motors; full mechanical travel is not guaranteed.

Only the selected output is attached. STOP or heartbeat loss cancels the sweep; Resume never restarts it. Release before choosing another motor. Saved leg limits stay unchanged, but the selected travel-check flag and standing record are cleared in RAM. After refitting horns/motors, repeat supported assembled-leg calibration. See the [full procedure](docs/unloaded-servo-calibration.md).

## Signal map and controls

| Wiring group | Proximal / middle / distal | Joint IDs |
|---|---|---|
| L0 | D2 / D3 / D4 | 0 / 1 / 2 |
| L1 | D5 / D6 / D7 | 3 / 4 / 5 |
| L2 | D8 / D9 / D10 | 6 / 7 / 8 |
| L3 | D11 / D12 / D13 | 9 / 10 / 11 |

D0/D1 stay free for USB serial. Labels preserve wire order; physical front/stations and joint axis names still need recording. The shield supplies connectors, not high-current power or position feedback.

The console deliberately arms one joint at a time, moves within per-joint limits at 10–100 µs/s, and stores calibration in EEPROM/JSON. STOP latches hold; Resume does not restart old targets. Release detaches signals and drives them LOW. Standing capture stores commanded pulses after all twelve joints are checked, attached and stationary; playback does not balance the robot. Use the full [one-servo → one-leg → twelve-joint → standing procedure](docs/bring-up.md) and [calibration log](docs/templates/calibration-log.md).

## Repository layout

```text
cad/                  Selected print STLs, native Fusion files, part map and hashes
firmware/spyder_uno/   Uno sketch, control, serial protocol and EEPROM storage
web/                  Web Serial console and hardware-free demo
tests/                Native C++, JS/integration, browser and AVR waveform checks
scripts/              Test runners and signed AVR toolchain/build helpers
docs/                 State, plan, calibration, development, references and archive
```

[Revision 3 verification](docs/verification/2026-10-19-unloaded-servo.md) records native checks, 36 JavaScript tests, 14 browser tests and 25 compiled-AVR scenarios, including an isolated one-cycle output sweep. [Revision 2 verification](docs/verification.md) is retained as history. [Handover verification](docs/verification/2026-10-18-handover.md) records the later organization and fresh checks. The later [connection diagnostic patch](docs/verification/2026-10-19-connection-diagnostics.md) passes 39 JavaScript tests and 17 browser tests and preserves startup failure reasons; its firmware is unchanged. These are software results, not a physical standing demonstration.

The two Fusion archives contain named editable counterparts for six of the seven selected print parts. The backing-pivot editable source and later fit packages still need recovery; the [CAD guide](cad/README.md) marks the gap and explains editing limitations. The [owner's Windows recovery folder](docs/handover.md#recovery-folder-supplied-by-the-owner) is documented but not readable from this cloud environment.

The additional shared-chat link could not be read here; its contents have not been claimed as incorporated. The original supplied Markdown is preserved in [the archive](docs/archive/2026-10-16-SPYDER-CODEX-HANDOVER.md), with later decisions summarized in the current state. See [contribution guidance](CONTRIBUTING.md) and [source/license attribution](docs/reference/upstream.md).

# Spyder hexapod — project handover and code-testing brief

Prepared: **16 October 2026**. This document summarises the design work and local project evidence so another Codex session can continue without the original conversation.

## 1. Start here

The project adapts **hpsaturn/quadpod**, a four-legged, 12-servo ESP32 robot, into **Spyder**, a six-legged robot with **18 SG90 servos**, three per leg.

- GitHub: **https://github.com/hpsaturn/quadpod**
- Clone URL: **https://github.com/hpsaturn/quadpod.git**
- Reviewed local commit: **`e4a7f0efd0447abaa603074676545080f0f3187b`**.
- Local checkout: `C:/Users/seun_/Documents/Codex/2026-09-22/ca/outputs/quadpod`.
- Workspace: `C:/Users/seun_/Documents/Codex/2026-09-22/ca`.

The checkout was rechecked for this handover: its remote is the above repository, HEAD matches that commit, and `git status --short` returned no changes. This is a verified local snapshot, not a claim that it is the latest upstream revision.

**Current position:** a complete six-leg structural CAD layout exists, but the firmware is still the original four-leg code. There is no documented successful six-leg firmware build, flash, powered integration test or walking demonstration in this work. The hip-to-mount joint also remains mechanically unresolved. Software analysis, compilation and tests without connected motors can start now; powered assembled-leg testing needs a retained, freely moving joint.

## 2. User requirements and decisions

- Six legs instead of four, keeping the original leg geometry where possible.
- SG90 servos; do not silently substitute MG90S. The upstream README recommends stronger servos because of the original robot's performance, but the user's chosen hardware remains SG90.
- Early exploration covered three detachable concepts: dovetail with release latch, keyed socket with pin, and quarter-turn collar.
- **The user subsequently rejected the dovetail/socket design. The current mounting choice is a fixed four-screw flange.** Do not restore rails, sockets, springs or release catches in the active design.
- Keep the main body and lid editable in STEP/Fusion for cosmetic changes.
- Intended branding: **University of Manchester Robotics Society** logo. Exact artwork has not been supplied/applied; do not invent it.
- Print prototypes in PLA on an **Elegoo Neptune 3 Pro**, with a **0.4 mm nozzle**.
- Hardware BOM must **exclude 3D-printed parts and filament**. CAD/print inventories remain separate.

The user has printed and assembled a leg and reported actual looseness. A plausible-looking CAD assembly is not evidence that the physical joint fits.

## 3. Work completed and revision history

| Stage | Output and present status |
|---|---|
| Repository review | Downloaded source, inspected firmware and source STL models, documented four-to-six-leg changes. |
| Three attachment concepts | CAD examples and illustrations created. Historical exploration; superseded by the screw-mount decision. |
| Dovetail leg prototype | Original leg meshes, nominal servo references, latch/pivot/spring-space work and print kit created. Historical, not the current attachment. |
| Temporary pivot/spacer kit | Printable latch test pin, spring-replacement spacer, washer and friction caps supplied. These are not metal parts and are not the later missing SG90 backing pivot. |
| Fixed body mount | Dovetail mechanism removed from the active leg design; continuous bridge and four-hole body flange retained. Fusion/STEP/STL exports exist. |
| Six-leg structural assembly | Six fixed screw-mounted legs attached to a hexagonal chassis, with separate editable lid and reference body-fixing hardware. STEP and Fusion exports exist. |
| SG90 fit v2 | Local passive bearing correction to a 4.85 mm bore for the source approximately 4.65 mm pin; optional fit spacers/packing supplied. Did not fit every servo interface. |
| SG90 fit v3 | Narrowed cradle, added removable front retainer, tie channels, rear ear relief and raised passive bearing seat. Some nominal CAD clearances and slicing passed, but the complete joint stack was incomplete. |
| Missing-pivot investigation, 15 October | Confirmed the source backing insert was omitted from the recent package and full assembly. Recovered it as a separate STL/STEP fit-trial package. Found a related 0.30 mm nominal ear/keeper interference. **Complete retained joint still pending.** |

No firmware conversion was implemented during these CAD revisions. The complete six-leg CAD export predates v3 and still contains the earlier fixed mount and v2 hip, not all the later fit-trial parts.

## 4. Current CAD and dimensional facts

The structural assembly has six stations at **0°, 60°, 120°, 180°, 240° and 300°**. Those are CAD station angles, not yet an established firmware leg-numbering/direction convention.

| Feature | Nominal dimension / status |
|---|---|
| Chassis across flats | 136 mm |
| Chassis across corners | Approximately 157.04 mm |
| Chassis height / separate lid thickness | 31 mm / 2.5 mm |
| Each body flange | Four 3.3 mm holes, 30 × 20 mm centre pattern; flange approximately 2.2 mm thick |
| Body-fixing hardware model | 24 M3 × 16 screws, 24 M3 nyloc nuts, 48 washers |
| Lower body fixing access | Twelve floor reliefs incorporated in the complete six-leg assembly; 1.5 mm residual floor thickness |
| Source leg CAD quality | Original STL-derived surfaces remain faceted. Editable solid exports are not a reconstructed fully parametric original leg. |

The complete structural STEP records 122 solids: 26 structural solids and 96 reference fixing-hardware solids. Recorded checks include valid STEP re-import, 289 static interference checks and 192 hardware-versus-structure checks. The Fusion audit records the same solid count and matching volume. These checks cover the shown arrangement; they do not establish full joint sweep, moving cable clearance, fatigue, strength, servo fit or walking stability.

Actual servos, actual horns, passive backing inserts, joint fasteners, battery and electronics are **not installed as fitted components in the complete structural assembly**. Earlier motor boxes were approximate references, not manufacturer CAD.

### Latest supplied SG90 drawing

Use these later dimensions rather than the earlier generic 23 × 12 / 26 / 29 mm drawing:

| Drawing feature | Dimension |
|---|---:|
| Main casing length | 22.4 mm |
| Casing width | 12.0 mm |
| Case bottom to top of mounting ears | 22.8 mm |
| Overall height to fitted horn top | 32.0 mm |
| Outer span across mounting ears | 31.8 mm |
| Ear mounting-hole pitch | 27.7 mm |
| Side housing step height | 16.0 mm |

The user's earlier 31.8 × 32 mm measurement included the motor's protruding features; it is not a rectangular casing size. The later drawing does **not** establish the output shaft offset, actual horn boss diameter, fitted joint span or all moulding tolerances. V3 assumed 2.4 mm ear thickness and symmetric ear projections; these were not measured.

## 5. Unresolved physical hip connection — read before powered testing

The user identifies the troublesome pieces as **`hip_rear_pivot` and the fixed mount**, with approximately 2 mm reported play earlier. The latest clarification was:

> “i am not using cable ties. The cable ties stop the motor slip. The last thing is the two parts connecting together. I didnt originally see a back pivot”

Interpret this carefully: cable ties address casing slip, but their present installation is not confirmed. The remaining reported problem is joining the hip and mount. The user had not seen/installed the backing pivot. Do not assume the actual horn/centre screw/horn-to-hip attachment is present or correct.

Three functions must be checked separately:

1. **Casing retention:** retain the motor casing in the fixed cradle; v3 uses a front cap and two ties. This does not attach the driven hip to its horn.
2. **Driven connection:** compatible SG90 horn, its matching centre screw, and the horn-to-hip attachment must transmit motion and retain the driven side.
3. **Passive support/retention:** a backing pivot supports the opposite side. A bearing pin is not automatically an axially retained or snap-locking joint.

### Missing original insert

The relevant source is **`models/Insert_body.stl`**. Do not confuse it with **`models/Body_insert.stl`**, the source chassis.

- Grounded print copy: `outputs/Quadpod-PLA-test-kit/STL/07_Insert_body.stl`.
- New clearly named copy: `outputs/SG90-missing-backing-pivot/Backing-pivot-original-PRINT.stl`.
- Plate: approximately **12.1 × 22.5 × 0.5 mm**.
- Pin diameter: approximately **4.65 mm**; projection from plate **3.675 mm**.
- Blind central opening: approximately **2.0 mm diameter × 2.0875 mm deep**. This is **not a confirmed M2 thread or fastener specification**.
- V3 passive hip bore: **4.85 mm**, giving nominal **0.20 mm diametral clearance**.
- In the checked nominal plate-on-cradle-floor arrangement, the insert clears the mount/hip and engages the hip by approximately **2.10 mm**. Actual horn alignment remains unverified.

**Known incompatibility:** v3 omitted the backing plate from the motor seating-height calculation. Adding its 0.50 mm plate beneath the servo raises the ears by 0.50 mm, while the old nominal ear clearance was only 0.20 mm. This leaves **0.30 mm nominal interference at the rear ear slot/front keeper**. The recovered insert is an identification/fit-trial part, not a validated drop-in repair of the whole v3 joint. Do not force the cap, stack spacers automatically or print six sets assuming it is fixed.

Still needed for a fitted revision: actual assembly photographs from the horn side and underside, installed fastener identification, horn dimensions/attachment and shaft position. Then check the full retained stack, bearing freedom and intended motion before duplicating it.

For one original insert trial: PLA, 0.4 mm nozzle, flat plate down/pin up, 100% scale, no supports, four walls, 100% infill, 0.10 mm layers and 0.20 mm first layer. Its closed mesh, valid solid and slicing were checked; physical retention was not.

## 6. Hardware BOM summary — excludes printed parts

This is the current purchasing/design basis, not a list of components confirmed owned or installed. Detailed quantities, conditional choices and pictures are in the BOM files listed below.

| Component | Quantity / status |
|---|---|
| SG90 servos | 18 installed; two optional spares |
| Matching servo horns | 18; commonly supplied with servos |
| Matching horn centre screws | 18; use the actual servo's supplied type, not generic M3 |
| Servo ear / horn-to-part / passive-pivot retaining hardware | Final types and counts pending actual joint assembly; do not treat all joint screws as specified |
| ESP32 board | 1; original ESP32/Lolin32-compatible baseline for Bluetooth Classic Serial |
| PCA9685 boards | 2, proposed addresses 0x40 and 0x41 |
| Regulated servo supply / buck or UBEC | 1 appropriately rated supply; voltage and current capability require actual servo/load confirmation |
| Separate controller regulator | 1 proposed; verify actual board input requirements |
| Battery and suitable charger | 1 each if battery powered; pack and connector selection pending |
| Power distribution, fuse/holder, servo-power switch | 1 set each; ratings pending measured current and routing |
| Bulk capacitors | Two 1000 µF, ≥10 V starting candidates from the earlier review; assess voltage dips rather than treating this as a final sizing |
| I2C/control harness and power wiring | 1 set; common ground, suitable wire and connector ratings |
| Servo extension leads | 0–18, according to routing |
| Electrical leg disconnects | Six matched connector pairs optional; individual servo plugs are an alternative |
| Body flange screws | 24 M3 × 16 mm |
| Body flange nyloc nuts | 24 M3 |
| Body flange washers | 48; CAD assumes 6 mm OD × 0.5 mm thick |
| V3 case-retainer cable ties | Two per leg if this retention arrangement is retained: twelve total, approximately 2.5–3.6 mm wide, ≤1.2 mm thick, ≥120 mm long |
| Lid fixings | Six positions; screw/insert arrangement still to be designed |
| Electronics mounting hardware | Actual footprints and hardware counts pending |
| USB data cable, insulation, strain relief | As needed |
| Battery strap / foot grip pads | One strap recommended / six pads optional |

The body screw length assumes a 2.2 mm flange, 7 mm body wall, two 0.5 mm washers and a 4 mm nyloc nut: approximately 14.2 mm stack. Check purchased hardware and real access; CAD hardware uses simplified envelopes.

The original repository contains useful board photographs and an electrical diagram, but not isolated pictures of every component. The illustrated BOM labels context pictures and unmatched items. Its overview includes MG90S imagery; that does not change the selected SG90. The old XL6009 boost-module picture is not a purchase match for the proposed step-down servo supply.

The original diagram's 7.4 V battery → boost-only module → 6 V relationship is not an appropriate step-down power design. A proposed 2S pack reaches 8.4 V fully charged; supply the servos through a correctly selected regulator. Do not power 18 servos from the ESP32 USB/header. Logic and servo power have distinct requirements, with a common ground. Exact voltage, supply current, battery, wire, fuse and connector ratings remain unresolved; there is no validated whole-robot power budget.

## 7. Firmware baseline: what is actually in the repository

Primary source links at the reviewed commit:

- [Repository README](https://github.com/hpsaturn/quadpod/blob/e4a7f0efd0447abaa603074676545080f0f3187b/README.md)
- [Build configuration](https://github.com/hpsaturn/quadpod/blob/e4a7f0efd0447abaa603074676545080f0f3187b/platformio.ini)
- [Entry point](https://github.com/hpsaturn/quadpod/blob/e4a7f0efd0447abaa603074676545080f0f3187b/src/main.cpp)
- [Motion, commands, inverse kinematics and PWM](https://github.com/hpsaturn/quadpod/blob/e4a7f0efd0447abaa603074676545080f0f3187b/src/servos.cpp)
- [Pins and command definitions](https://github.com/hpsaturn/quadpod/blob/e4a7f0efd0447abaa603074676545080f0f3187b/include/servos.hpp)

| Configuration | Existing setting |
|---|---|
| PlatformIO environment name | `esp32dev` |
| Actual board setting | `lolin32` |
| Platform / framework | `espressif32 @ 4.4.0` / Arduino |
| PWM dependency | `Adafruit PWM Servo Driver Library @ 3.0.2` |
| Command dependency | `https://github.com/shyd/Arduino-SerialCommand.git#25599a0` |
| Serial monitor | 115200 baud |
| Bluetooth | `ENABLE_BLUETOOTH=1`, Classic `BluetoothSerial`, advertised name `QuadPod` |
| I2C pins | SDA GPIO16; SCL GPIO4 |
| PWM boards | One `Adafruit_PWMServoDriver` instance |
| PWM frequency | Existing code requests 60 Hz; validate actual servo operation before retaining/changing it |
| Servo-service timing | FreeRTOS task requests a 10 ms delay between updates |

Do not confuse the environment name `esp32dev` with the configured board `lolin32`. Confirm the user's actual board and pins before flashing. A board without Bluetooth Classic support is not a transparent substitute for this control interface.

### Original channel order and geometry

The existing mapping is:

```cpp
const int servo_pin[4][3] = {
    {0, 1, 2},
    {4, 5, 6},
    {8, 9, 10},
    {12, 13, 14}
};
```

These are **PCA9685 output channels, not ESP32 GPIO pins**. The current column order is **alpha = hip pitch, beta = knee, gamma = yaw**. Original software leg identities are 0 front right, 1 rear right, 2 front left, 3 rear left; those identities cannot simply be extended to six radial stations without defining a new convention.

Original kinematic constants are `length_a=55`, `length_b=77.5`, `length_c=27.5`, `length_side=71` mm. They are existing code values, not newly verified measurements of the fitted hexapod. Measure axis-to-axis lengths and hip-axis origins after the actual horn/pivot stack is settled. Mesh bounding boxes and chassis face radius are not substitutes for these measurements.

### Code locations and findings to address

Line numbers below refer to the pinned clean source and will move after edits.

| Location | Finding / work needed |
|---|---|
| `servos.cpp:8–14` | One driver and four-leg channel map; implement board + channel routing for 18 joints. |
| `servos.cpp:16–24` | Quadruped geometry/stance constants; verify fitted link lengths and define six local hip coordinate frames. |
| `servos.cpp:28–42` | `[4][3]` motion arrays and twelve named FR/FL/RR/RL calibration fields. Generalise to six legs and 18 joint calibration records. |
| `servos.cpp:78–100` | `wait_reach()` busy-waits with no timeout; `wait_all_reach()` visits four legs. Stop/communications must remain responsive during motion. |
| `servos.cpp:120–135` | `set_site()` divides by move length: handle zero-distance/no-op moves and reject non-finite input. |
| `servos.cpp:152–607` | Stances, turns, walking and gestures contain explicit four-leg sequences. Increasing array sizes alone will not produce a hexapod gait. |
| `servos.cpp:612–619` | `action_cmd()` calls `atoi()` on unchecked command tokens. Validate missing, non-numeric, negative and excessive arguments. |
| `servos.cpp:623–874` | Command handling/calibration tied to four legs; forward/back/turn set `lastComm`, allowing repeated movement in the main loop. |
| `servos.cpp:883–897` | Inverse kinematics uses unguarded `acos()`/denominators. Handle unreachable targets, singularities and numerical boundary errors explicitly. |
| `servos.cpp:911–948` | Four leg-specific direction/offset cases and shared pulse clamps. Integer expressions `850/180` and `580/180` truncate; replacing them with floating point alone is not a calibrated pulse model. |
| `servos.cpp:953–977` | Servo task locks a mutex, while other motion-state readers/writers do not consistently share that protection. Establish one state owner/queue or consistent synchronisation. |
| `servos.cpp:995–1035` | Initialisation sets four targets and starts the servo task, then calls startup poses. Existing firmware can move on boot. |
| `servos.cpp:1045–1081` | Bluetooth client presence mainly changes LED timing; motion can repeat through `lastComm`. Define disconnect/watchdog/STOP behaviour. |
| `include/servos.hpp` and `bt/serial_bluetooth_terminal.cfg.txt` | Reconcile command labels and directions; create controls/calibration for six legs. The phone profile contains the author's saved device settings. |

`site_now` is an interpolated commanded position, **not measured joint/foot feedback**. These SG90s provide no encoder feedback to the controller. A software “target reached” test cannot detect a detached horn, stall or slipping joint. Existing comments about EEPROM are not evidence of implemented persistent calibration; explicitly implement and test persistence if required.

## 8. Proposed six-leg control architecture — not implemented

One PCA9685 provides 16 outputs, fewer than the required 18. The reviewed proposal uses **two boards**, with separate addresses **0x40 and 0x41**. Confirm address jumpers and actual I2C detection before enabling motor power.

Proposed numbering: label L0–L5 consecutively around the body when viewed from above. Establish a physical front, clockwise/counterclockwise convention, station angles, joint-positive directions and wiring labels before using the table.

| Leg | Proposed board address | Yaw channel | Hip-pitch channel | Knee channel |
|---|---|---:|---:|---:|
| L0 | 0x40 | 0 | 1 | 2 |
| L1 | 0x40 | 3 | 4 | 5 |
| L2 | 0x40 | 6 | 7 | 8 |
| L3 | 0x41 | 0 | 1 | 2 |
| L4 | 0x41 | 3 | 4 | 5 |
| L5 | 0x41 | 6 | 7 | 8 |

**This table is yaw-first; the original code is pitch/knee/yaw.** Implement the mapping explicitly rather than reconnecting motors and running the old code. Board/channel allocation does not establish the power distribution rating.

The conversion should include:

1. Central `LEG_COUNT=6`, `JOINT_COUNT=3`, and complete per-leg/per-joint state/configuration; search explicit leg IDs as well as array declarations.
2. A driver abstraction with board address/channel mapping and an output-disabled/mock mode for tests without hardware.
3. Eighteen calibration records: direction, neutral, pulse endpoints and mechanical limits, plus versioned persistent storage if used.
4. Body-to-leg coordinate transforms using actual hip origins/yaws, then checked inverse kinematics and output limiting.
5. A nonblocking motion scheduler that can process STOP, timeouts, command loss and disconnect while moving.
6. An explicit disabled startup state, intentional enable, and defined stop/hold/power-off behaviour. Stopping target updates does not itself remove servo holding torque.
7. Start with a slow six-leg wave gait, one foot lifted at a time. An alternating tripod gait is a later option after support geometry and load tests; do not assume even/odd groups are stable before checking the numbering and stance.
8. Keep toolchain upgrades separate from the mechanical/leg-count conversion so failures can be isolated.

## 9. Code-testing sequence for the next Codex session

These are next steps, **not tests already performed**. This handover did not install dependencies, compile, upload firmware or operate hardware.

### A. Preserve and build the original baseline

Use the existing checkout if available. In a new workspace:

```powershell
git clone https://github.com/hpsaturn/quadpod.git
Set-Location quadpod
git switch --detach e4a7f0efd0447abaa603074676545080f0f3187b
git switch -c spyder-hexapod
git status --short
pio run -e esp32dev
```

PlatformIO must be installed/available and may need network access to download the pinned dependencies. The original baseline is useful for compiler/dependency checks, not direct six-leg motion. Do not run upload as part of a compile-only check. Record tool versions, build output, warnings and exact commit. Preserve any work already present before changing branches or source.

### B. Tests without connected motors

- Exercise leg/channel mapping: exactly 18 unique intended board/channel pairs, valid board/channel ranges, and no accidental duplicate assignments.
- Verify all six legs initialise and invalid leg/joint identifiers are rejected.
- Test no-op moves, non-finite coordinates, singular/unreachable targets and limits; no NaN/invalid pulse may reach the output abstraction.
- Check each joint's direction/neutral/endpoints and calibration save/load/default/schema handling if persistence is implemented.
- Check scheduler progress and STOP during motion, lost commands/disconnect, timeouts and disabled startup using a fake clock/output backend.
- Verify malformed, missing, negative and excessively large command arguments cannot trigger unintended or unbounded movement.
- Check stance/gait trajectories in body and leg frames, including reach limits and supporting-foot geometry. Simulation is not physical validation.

### C. Controller and driver bring-up

Initially keep servo power disconnected. Confirm the actual ESP32 variant, GPIO16/GPIO4 wiring, logic voltage, common ground and both driver addresses. Run a deliberately limited test mode; the original program's startup routine already commands poses.

Record the detected board addresses, output enable state, command response and reset/disconnect behaviour. Confirm no output appears on the wrong board/channel.

### D. One servo, then one supported leg

Use a verified servo supply and an accessible power disconnect. Begin with one servo in a small tested range, without forcing a fitted mechanism against its stops. Establish neutral/direction and conservative pulse limits for the actual servo. Then attach/test one mechanically retained leg while the body is supported, checking alignment, free bearing rotation, wire clearance, current, voltage dips and temperature.

Resolve the missing backing pivot, horn retention and v3 plate/ear interference before assembled-leg motion. Do not treat the original blind hole as a specified threaded fastening. Inspect actual assembly photos/measurements instead of issuing another guessed tight-fit revision.

### E. Six legs and gait tests

After every individual joint/leg passes, test all 18 mappings and a supported neutral pose. Check full motion envelopes and adjacent-leg/wire clearance. Establish power/torque margins under staged loads, then try slow one-leg-at-a-time walking and turning. Tripod gait and cosmetic completion follow stable basic operation.

Passing compilation, host tests, mesh validation or slicing is not evidence that this robot can walk safely or reliably.

## 10. Existing files to carry across

All paths below are under this absolute output root:

`C:/Users/seun_/Documents/Codex/2026-09-22/ca/outputs`

| Path relative to output root | Purpose / revision warning |
|---|---|
| `quadpod/` | Clean original firmware and models, pinned above. |
| `quadpod-source.zip` | Earlier source archive; inspect its contents/commit before treating it as the verified checkout. |
| `quadpod-review.md` | Initial code/model analysis; later decisions and fit findings in this handover take precedence. |
| `Spyder-complete-six-leg-CAD.zip` | Current complete structural CAD package, fixed screw mounts; predates v3 fit work. |
| `Spyder-six-leg-assembly/Spyder-complete-six-leg-assembly.f3d` | Fusion component assembly. |
| `Spyder-six-leg-assembly/Spyder-complete-six-leg-assembly.step` | Separate solid bodies for CAD interchange. |
| `Spyder-six-leg-assembly/README.md` | Dimensions, assembly limitations and structural check scope. |
| `Spyder-six-leg-assembly/assembly-validation.json` | Static CAD/hardware checks; not physical/full-motion validation. |
| `Spyder-six-leg-assembly/fusion-verification.json` | Saved Fusion component/body audit. |
| `Spyder-six-leg-assembly/Parts/Chassis-fixed-mount-v2.step` | Revised fixed-mount chassis with lower fixing recesses. |
| `Spyder-six-leg-assembly/Parts/Plain-cosmetic-lid.step` | Editable plain lid; no society artwork applied. |
| `Spyder-fixed-screw-mount.zip` | Earlier individual fixed-mount leg package. |
| `SG90-fit-revision-v2.zip` | Earlier local bearing/spacer fit trial; superseded in part by v3. |
| `SG90-secure-fit-v3-STLs.zip` | Fit-trial mount/hip/retainer parts; includes known-issue notice, not a complete repair. |
| `SG90-secure-fit-v3-CAD.zip` | Corresponding editable solid exports with the same known issue. |
| `SG90-secure-fit-v3/KNOWN-JOINT-ISSUE-2026-10-15.md` | Important warning about incomplete retention and missing plate allowance. |
| `SG90-missing-backing-pivot.zip` | Recovered original pivot STL/STEP, pictures, dimensional check and fitting caveats. |
| `SG90-missing-backing-pivot/READ-FIRST.md` | Installation hypothesis, interference and physical information still needed. |
| `SG90-joint-audit/FINDINGS-2026-10-15.md` | Root-cause investigation and user clarification. |
| `Hexapod-hardware-BOM-and-conversion-review.md` | Detailed hardware BOM and firmware conversion review. Some older mechanical “to do” statements predate the completed structural assembly; use this handover/current CAD README for their status. |
| `Hexapod-hardware-BOM.csv` | Nonprinted hardware purchasing list. |
| `Hexapod-conversion-checklist.csv` | Conversion actions from the review; reconcile older CAD items with current exports. |
| `Hexapod-illustrated-BOM.zip` | Illustrated hardware BOM package. |
| `Hexapod-illustrated-BOM/Hexapod-hardware-BOM-with-pictures.xlsx` | Spreadsheet with available source pictures embedded. |
| `Hexapod-illustrated-BOM/Illustrated-BOM.html` | Illustrated browser-readable version. |
| `Hexapod-body-v1.zip`, `modular-leg-cad.zip` | Historical dovetail/body concepts; do not use as the active fixed-mount design. |
| `Pivot-and-spacer-test-kit.zip` | Historical printed latch pivot/spacer; distinct from SG90 backing insert. |
| `Quadpod-PLA-test-kit.zip` | Earlier leg print kit; contains the backing insert but is not a final combined six-leg fit revision. |

CAD generation and verification scripts are in `C:/Users/seun_/Documents/Codex/2026-09-22/ca/work`. The local CAD Python environment is `work/cad-env/Scripts/python.exe`. Some authoring scripts execute file writes and rebuilds at import/top level; read them before running/importing. Rebuilding an old package can overwrite later issue notes or recreate superseded geometry.

For a code-only transfer, this Markdown file and a clone of the pinned repository are enough to start inspection/build work. To continue mechanical work on another machine, also transfer the current six-leg CAD, v3 packages and missing-pivot package; GitHub alone does not contain these custom changes. Absolute local paths in this document will need rebasing on another computer.

## 11. Suggested first instruction to the next Codex session

```text
Read this Spyder handover first. Work from hpsaturn/quadpod at commit
e4a7f0efd0447abaa603074676545080f0f3187b and preserve the original baseline.
My target is six legs with three SG90 servos per leg, two PCA9685 boards,
an ESP32, and fixed four-screw body mounts. The dovetail concept is abandoned.

Start with a compile-only baseline and a focused firmware review. Then add
testable six-leg configuration and board/channel routing, numerical/command
validation, responsive stop/watchdog handling, and tests without motors.
Keep the toolchain upgrade separate from the six-leg conversion. Distinguish
verified results from proposed work and record commands/results.

Do not flash or initiate robot movement as part of the initial software work.
The physical SG90 hip connection is unresolved: a backing insert was missing,
and including its 0.5 mm plate creates 0.3 mm nominal interference with the v3
ear keeper. The complete CAD still uses earlier fit parts and does not contain
fitted motors/horns/backing pivots. Review those issues before powered testing.
Do not assume the proposed yaw-first channel table matches the original
pitch/knee/yaw output order. Confirm real wiring and hardware before enabling.
```

## 12. Completion criteria still outstanding

- One physically secure SG90 leg with retained horn and passive pivot, free joint motion and measured link geometry.
- Updated complete CAD incorporating the final fitted leg parts and actual electronics/fastener envelopes.
- Selected and tested whole-robot power distribution, battery/regulators and torque margin.
- Six-leg firmware, calibration for all 18 joints, tested stop/enable behaviour and recorded successful build.
- Staged hardware tests followed by a demonstrated slow stable gait.
- Actual society logo artwork and final cosmetic lid edits.

This handover supersedes optimistic “complete” wording in earlier prototype filenames: structural CAD completion and slicer success do not mean completed mechanical retention or functioning six-leg firmware.

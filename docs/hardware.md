# Hardware basis

Current bench build: four installed legs, three servo outputs per leg, Arduino Uno and Arduino Sensor Shield V5. On 19 October the owner identified the motors as 9 g SG90s and some MG90Ds; exact quantities, manufacturer and per-joint assignments are not recorded. See [current state](project-state.md) for the pin table and [bring-up](bring-up.md) for the actual procedure.

This list excludes printed parts and filament; the [CAD inventory](../cad/parts.csv) is separate. It is not a claim that every item has been purchased or fitted.

| Item | Current milestone / status |
|---|---|
| Arduino Uno | 1, owner-selected test controller. |
| Sensor Shield V5 | 1, owner-confirmed connector shield; inspect actual revision and S/V/G markings. |
| SG90 / MG90D | Owner reports 9 g SG90s plus some MG90Ds. The current software addresses twelve outputs; the eventual six-leg design needs eighteen. Record which model is connected at each pin. MG90D is the stated model, not an assumed MG90S substitution. |
| Compatible horns and centre screws | One per servo as appropriate to the actual joint; attachment/retention still needs inspection. Use the servo's matching screw type. |
| Passive pivots / retaining fasteners | Confirm the installed stack. Printed backing insert is in CAD; a blind hole is not a verified M2 thread specification. |
| External regulated servo supply | Required before powered calibration; actual model, supported voltage, current capability and load measurements are still unconfirmed. |
| Distribution / wiring / protection / switch | Size for the real combined startup/load current. Verify shield isolation to prevent backfeeding USB/Uno and retain common ground. Accessible servo-power disconnect required. |
| Body flange hardware | Six-leg CAD reference: 24 M3 × 16 screws, 24 M3 nylocs, 48 washers (four screw positions per station). Verify installed quantities and real stack/access. |
| USB data cable / strain relief | Needed for local firmware upload and desktop Web Serial. |
| Foot grip and body support | Required for staged standing trials; material and surface still to be checked physically. |

The earlier ESP32, two PCA9685 boards, regulators/battery and full six-leg BOM remain design proposals in the [archived handover](archive/2026-10-16-SPYDER-CODEX-HANDOVER.md). They are not installed requirements for this Uno sketch. No whole-robot power budget or final battery/fuse/wire sizing has been validated.

## Unloaded test defaults

The [unloaded calibration controls](unloaded-servo-calibration.md) center one motor and sweep one explicit pulse window. After identifying SG90 and MG90D motors without datasheets, the owner chose the existing **1000–2000 µs defaults** and **1500 µs nominal center**. These are the selected software envelope, not verified full-travel specifications for either motor. No wider manufacturer claim is made; manufacturer pages were inaccessible through this environment's proxy. Use smaller endpoints if physical behavior requires it, and recalibrate assembled travel after refitting.

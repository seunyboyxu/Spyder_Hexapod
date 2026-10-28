# Documentation map

| Need | Document |
|---|---|
| Choose the current sketch | [Root README](../README.md), [sketch index](../firmware/README.md) |
| Understand functions, commands, storage and safety | [Firmware guide](firmware-guide.md) and each sketch README |
| Current status / next steps / takeover | [State](project-state.md), [plan](project-plan.md), [handover](handover.md) |
| Wiring / upload / physical test record | [Hardware](hardware.md), [bring-up](bring-up.md), [log](templates/calibration-log.md) |
| Build and test | [Development](development.md) |
| Print and edit CAD | [CAD guide](../cad/README.md), [CAD recovery history](reference/cad-history.md) |
| Original project and licensing | [Upstream](reference/upstream.md) |

## Verification evidence

- [Ryan walking integration and merge](verification/2026-10-28-esp32-walk-merge.md) — latest walking checks and package provenance.

- [ESP32 publication and cleanup checks](verification/2026-10-28-esp32-publication.md) — current repository-wide checks.
- [Four-leg group calibration v1.1](verification/2026-10-28-four-leg-group-calibration.md).
- [Four-leg controller baseline](verification/2026-10-27-esp32-four-leg-test.md).
- [BLE String compatibility](verification/2026-10-27-ble-string-compatibility.md).
- [BLE one-leg baseline](verification/2026-10-27-esp32-ble-leg-test.md).
- [Classic one-leg baseline](verification/2026-10-27-esp32-leg-test.md).
- [Unloaded motor baseline](verification/2026-10-27-esp32-servo-test.md).

Dated reports describe the tested snapshot at the time. Their historical references to preserved legacy files or an earlier publication pause are superseded by the current state. The active repository has ESP32 source/tests only. Software evidence is not a physical motion or power-supply validation.

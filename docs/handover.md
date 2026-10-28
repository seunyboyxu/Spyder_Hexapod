# Contributor handover

Start at [README](../README.md) and [current state](project-state.md). The active controller is **firmware/spyder_esp32_ble_four_leg_test**, v1.1, using original ESP32 BLE and PCA9685 channels0–11. Other ESP32 folders isolate one leg or one unloaded motor and remain separate upload choices. Read the [function guide](firmware-guide.md) before changing copies of shared modules.

The owner uses Arduino IDE, an iPhone, SDA GPIO21/SCL GPIO22, and consecutive channel triples hip/knee/foot. BLE app connection is inside Bluefruit Connect UART or nRF Connect with TX notifications. Classic SPP sketches are Android/Windows alternatives. ESP32 board marking is ESP-32D, Wifi+BT, N4XX; the complete board model and power-isolation circuit remain unidentified.

The latest group workflow is leg all → cal all → small joint tests/individual corrections → return to reference → explicit check all → off/save. Subsequent checked armall prepares supported poses/finite simultaneous steps. Preserve all-off boot, separate per-leg settings, atomic group preflight, finite sequences and global STOP/OFF/disconnect cancellation. Do not invent measured positions or automatic direction detection.

Current PP3 battery/5V3A buck is inadequate. Supply, OE/reset behavior, actual iPhone interoperability, fitted geometry, joint clearance and loaded motion require physical verification. See [hardware](hardware.md), [bring-up](bring-up.md), [plan](project-plan.md) and [log template](templates/calibration-log.md). No firmware upload or motor actuation should be performed by a cloud agent.

On28 October2026 the owner explicitly requested all ESP32 work be published and legacy board files removed. Old code and browser tooling remain recoverable through Git history, rather than in the active tree. Development uses a branch and pull request; do not merge or push directly to main without authorization. Follow [development checks](development.md) and [contribution guidance](../CONTRIBUTING.md).

## CAD and upstream

[Selected STLs and Fusion sources](../cad/README.md) retain their exact bytes/provenance. Six of seven printed parts have named editable counterparts in the native archives; the backing-pivot source is still missing. [Original GitHub](reference/upstream.md): hpsaturn/quadpod at pinned commit e4a7f0efd0447abaa603074676545080f0f3187b. It is a reference, not a drop-in controller/calibration for this wiring.

## Recovery folder supplied by the owner

The owner supplied this Windows folder for additional project files:

```text
C:\Users\seun_\OneDrive - The University of Manchester\Robotics Scoiety\FInalnd Hackathon
```

It is not mounted in this cloud workspace. The extra shared conversation was also inaccessible. Do not claim those files were inspected or incorporated. Recover additional fit packages, the missing-pivot STEP or new decisions explicitly and record source/version/physical checks before adding them.

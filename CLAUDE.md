# Spyder — instructions for continuing agents

Current target: original ESP32 (owner marking ESP-32D, Wifi+BT, N4XX), PCA9685 0x40, four positional-servo legs on channels0–11. GPIO21 SDA, GPIO22 SCL, GPIO23 OE. Arduino IDE and Arduino ESP32 framework are still used; this is not an Arduino Uno project.

Read README.md, docs/project-state.md, docs/firmware-guide.md and docs/handover.md before changing behavior.

- Calibration controller: firmware/spyder_esp32_ble_four_leg_test, version1.1, BLE/iPhone. Latest walking prototype: firmware/spyder_esp32_ble_walk, BLE name Spyder-Walk-BLE; shares calibration. Preserve checked arming, explicit calibration, per-leg settings, atomic group preflight and global safety cancellation.
- Keep the five ESP32 sketches separately uploadable; retain their matching folder/.ino names and companion files. Do not mix Classic SPP and BLE transports or their phone instructions.
- Owner explicitly requested publication of all new ESP32 sketches and removal of legacy Uno/Sensor Shield files on28 October2026. This supersedes earlier Uno-only and preserve-old-firmware instructions. Legacy implementations remain recoverable in Git history; do not reintroduce them.
- Work on a branch, push the authorized changes and open a PR. Do not push to main or merge without further authorization.
- Never upload firmware or move motors yourself. The owner performs physical tests; distinguish host/compiled evidence from measured hardware behavior.
- Keep CAD originals and provenance intact. Seven STLs/two Fusion archives are recorded in cad/SHA256SUMS.
- Current PP3 9V battery +5V/3A buck is inadequate for multi-servo testing. Do not claim staged arming solves supply limitations. GPIO/OE reset behavior must be electrically checked.
- Run affected checks and bash scripts/test-esp32.sh before publishing changes; reproduce builds as described in docs/development.md. Commands must not upload automatically.
- Walking: firmware/spyder_esp32_ble_walk (v1.0) is the four-leg BLE v1.1 controller copied unchanged plus a balanced crawl gait (`walk/back/left/right [N]`, `halt`). It shares the `spyder-four` calibration. Gait settings live in its `WalkConfig.h`; host checks are `bash scripts/test-esp32-walk.sh` (not yet in `test-esp32.sh`). Owner rule: add new walking/standing code as new files or sketch folders rather than editing existing sketches.

The owner explicitly authorized merging the ESP32 cleanup and Ryan walking branches on28 October2026. That authorization applies to PR#3 into the cleanup branch and PR#2 into main after verification. Do not merge unrelated legacy branches that reintroduce old code.

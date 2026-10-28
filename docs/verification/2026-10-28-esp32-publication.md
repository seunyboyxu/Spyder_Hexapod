# ESP32 publication and cleanup — 28 October 2026

Scope authorized by the owner: publish all four new ESP32/PCA9685 sketches, remove legacy board software from the active repository, and clearly label each sketch and explain its functions. Target branch: feat/esp32-project-cleanup; PR to main. No direct main push/merge, hardware upload or motor actuation.

## Included and removed

Included all four complete, self-contained ESP32 sketches, their host test fixtures/runners, per-sketch compile helpers, root PlatformIO environments and a current documentation/handover set. Four-leg BLE v1.1 includes group calibration/arming/checks. Both BLE folders include core2/core3 String compatibility. Classic SPP alternatives are explicitly labeled Android/Windows; BLE versions are the iPhone choices. The function guide covers commands, controller classes, input/radio/output boundaries, storage, geometry and physical limitations.

Removed legacy Uno/Sensor Shield sketches, standalone root calibration, browser console/launcher, Node/Playwright manifests, AVR build/test tools, old board-specific tests and superseded board documentation. Portable leg controller/parser tests used by ESP32 were retained under tests/leg_core, and all remaining runners updated. Older implementations remain in Git history; no history rewriting. A local legacy backup outside the repository was also preserved.

CAD files, part manifest, source attribution and license text remain. CAD recovery notes retain relevant missing-source/fit context without carrying forward superseded electronics instructions. CLAUDE.md now records the current ESP32 target and owner's explicit publication/cleanup request.

## Verification

- `bash scripts/test-esp32.sh`: all five ESP32 suites passed under AddressSanitizer/UndefinedBehaviorSanitizer and compiler warnings as errors. Covers single-motor, Classic one-leg, BLE one-leg, four-leg and both BLE return-type models.
- Root PlatformIO build `pio run -e esp32_servo -e esp32_classic_leg -e esp32_ble_leg -e esp32_ble_four_leg`: all four compiled/linked successfully, no upload. PlatformIO6.1.18/espressif32 platform6.12.0, Arduino ESP32 core2.0.17, Adafruit PWM3.0.2 and BusIO1.17.4. Used ignored local PLATFORMIO_CORE_DIR because the sandbox home is read-only.

| Environment | Flash bytes / 1,310,720 | Static/global RAM bytes / 327,680 |
|---|---:|---:|
| esp32_servo | 1,122,933 | 40,356 |
| esp32_classic_leg | 1,133,345 | 40,484 |
| esp32_ble_leg | 1,166,325 | 39,796 |
| esp32_ble_four_leg | 1,170,689 | 40,028 |

Static/global RAM totals exclude dynamic BLE heap/task stacks. The root entry-file build can optimize differently from per-sketch Arduino conversion; both build paths use the same sketch/controller code.
- The first root matrix build exposed PlatformIO's lack of nested .ino conversion: companion .cpp files compiled but setup/loop were missing. The build selector now explicitly includes the selected nested sketch; the same four-environment build verifies the correction. Sketch folder contents remain directly uploadable through Arduino IDE.
- Both BLE sketches were compiled against actual Arduino ESP32 core3.3.2 during the preceding delivery; the current cleanup preserves their functional code. Classic variants use the verified2.0.17 configuration.
- CAD checksums: all10 recorded files passed; print/Fusion bytes unchanged.
- Relative Markdown links:141 file targets checked, zero broken. Source inventory:126 indexed files, exactly four ESP32 .ino sketches; no legacy sketches, browser/Node manifests, caches or generated firmware staged. Whitespace checks passed.
- Exported the Git index into a fresh temporary checkout and ran `bash scripts/test-esp32.sh` there: all five suites passed, demonstrating retained tests do not depend on removed/untracked legacy files.
- A fresh independent read-only reviewer reran all five suites, all four actual root builds, CAD checksums, staged whitespace and141 link checks: no material findings. It independently confirmed source filtering, portable-test dependencies, documentation/function claims and publication scope.

## Physical limits

No hardware/iPhone is attached in this environment. Real BLE interoperability, OE/reset bias, pulse accuracy, runtime heap, supply performance and mechanical behavior remain unverified. The confirmed PP3 battery plus5V/3A buck is inadequate for powered multi-servo tests; replace and size the source/distribution before use. All-four simultaneous step tests require external body support and are not a balanced gait. Arduino IDE/framework support is retained because all current sketches target the ESP32 Arduino core.

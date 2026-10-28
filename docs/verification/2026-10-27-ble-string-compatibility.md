# BLE one-leg v1.1 — ESP32 String compatibility fix

Historical verification snapshot. For current publication status, retained tests and source layout, see [project state](../project-state.md) and [publication checks](2026-10-28-esp32-publication.md).
27 October 2026. The user's newer ESP32 Arduino core returns Arduino `String` from `BLECharacteristic::getValue()`, whereas core2 returns `std::string`. The explicit `const std::string` assignment caused the reported compile error.

`BleUart.cpp` now infers the return type with `const auto`, then accesses the received bytes through `value.c_str()` and `value.length()`, which both types support. This preserves embedded bytes for the existing input validation. Leg commands, calibration scale, PCA9685 channels0/1/2, GPIO21/22/23 wiring and BLE service are unchanged.

Fresh checks:

- Reproduced the identical conversion error against an Arduino String return-type model; both BLE variants pass after correction.
- One-leg sanitizer suite:11 reported checks/groups passed, including concurrent subscription handling and the actual-sketch host harness.
- Actual one-leg ESP32 Arduino2.0.17 build:1,166,869 flash bytes /39,796 static RAM bytes.
- Actual one-leg ESP32 Arduino3.3.2 build through Arduino CLI:1,128,563 flash bytes /41,216 global RAM bytes. Both fit the default application partition. Adafruit PWM3.0.2 and BusIO1.17.4 used.
- Existing npm suite39/39 passed. The four-leg independent review also checked the shared compatibility correction and newer-core build evidence.

The updated ZIP is **v1.1**; keep all companion files beside `spyder_esp32_ble_leg_test.ino`. The new four-leg package includes the same correction. No hardware upload, physical motor movement or GitHub push performed.

No real iPhone/radio or electrical/mechanical behavior was tested in this cloud workspace. The confirmed **PP3 9V battery plus5V/3A buck is not a suitable multi-servo test supply**; replace it with an appropriately rated regulated5V source/distribution before powered leg tests. Compile compatibility does not resolve inadequate power. Initial USB/BLE help/show checks can be done with servo power off.

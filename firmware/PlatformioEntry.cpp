// PlatformIO converts .ino files only at src_dir's root. Select one nested
// Arduino IDE sketch explicitly; its companion .cpp files are source-filtered
// by platformio.ini. Arduino IDE opens each sketch folder directly and never
// uses this file. Keep all functional code in the original sketch folders.
#if defined(SPYDER_SERVO)
#include "spyder_esp32_servo_test/spyder_esp32_servo_test.ino"
#elif defined(SPYDER_CLASSIC_LEG)
#include "spyder_esp32_leg_test/spyder_esp32_leg_test.ino"
#elif defined(SPYDER_BLE_LEG)
#include "spyder_esp32_ble_leg_test/spyder_esp32_ble_leg_test.ino"
#elif defined(SPYDER_BLE_FOUR_LEG)
#include "spyder_esp32_ble_four_leg_test/spyder_esp32_ble_four_leg_test.ino"
#elif defined(SPYDER_BLE_WALK)
#include "spyder_esp32_ble_walk/spyder_esp32_ble_walk.ino"
#else
#error "Select a documented ESP32 environment from platformio.ini."
#endif

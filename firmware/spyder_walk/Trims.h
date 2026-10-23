#pragma once
#include "Walk.h"
// Layout of the trims saved in EEPROM. Kept in a header so the Arduino IDE's
// automatic function prototypes can see the type.
struct Saved {
  uint8_t magic, version;
  float trim[walk::LegCount][3];
  int8_t dir[walk::LegCount][3];
  uint8_t sum;
};
const uint8_t SavedMagic = 0x5c;

#pragma once
#include "Wire.h"
class Adafruit_PWMServoDriver {
 TwoWire &wire;
public:
 Adafruit_PWMServoDriver(uint8_t,TwoWire &w):wire(w) {}
 bool begin() { return wire.present; }
 uint8_t setPWM(uint8_t channel,uint16_t on,uint16_t off) {
  wire.beginTransmission(0x40); wire.write(6+channel*4);
  wire.write(on&255); wire.write(on>>8); wire.write(off&255); wire.write(off>>8);
  auto status=wire.endTransmission();
  if(wire.afterWrite) { auto fn=wire.afterWrite; wire.afterWrite=nullptr; fn(); }
  return status==0 ? 0 : 1;
 }
};

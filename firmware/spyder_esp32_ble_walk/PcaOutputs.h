#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "FourLegTest.h"

class PcaOutputs : public fourleg::Channels {
public:
  bool begin();
  void attach(uint8_t joint,uint16_t pulseUs) override;
  void write(uint8_t joint,uint16_t pulseUs) override;
  void detach(uint8_t joint) override;
  void inhibit();
  void disableAll();
  bool healthy();
  bool failed() const { return bad_; }
  uint16_t activeMask() const { return activeMask_; }
  void radioClosed();
  bool takeRadioRelease();
private:
  static constexpr uint8_t Address=0x40, OePin=23;
  Adafruit_PWMServoDriver driver_{Address,Wire};
  portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
  volatile bool radioRelease_=false;
  bool bad_=false, initialized_=false;
  uint16_t activeMask_=0;
  bool fail();
  bool setPulse(uint8_t joint,uint16_t pulseUs);
  bool writeRegister(uint8_t reg,uint8_t value);
  bool readRegister(uint8_t reg,uint8_t &value);
};

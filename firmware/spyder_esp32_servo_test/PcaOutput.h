#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "ServoBench.h"

class PcaOutput : public servobench::Output {
public:
  bool begin();
  bool enable(uint16_t pulseUs) override;
  bool write(uint16_t pulseUs) override;
  void disable() override;
  bool interrupted() const override { return cancelled_; }
  bool healthy();
  // Called from Bluetooth's task, never an ISR. Serialize inhibition with enable.
  void radioClosed();
  bool takeRadioRelease();
private:
  static constexpr uint8_t Address=0x40, OePin=23;
  Adafruit_PWMServoDriver driver_{Address,Wire};
  portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
  volatile bool radioRelease_=false;
  bool bad_=false, cancelled_=false;
  bool fail();
  bool writeRegister(uint8_t reg,uint8_t value);
  bool readRegister(uint8_t reg,uint8_t &value);
};

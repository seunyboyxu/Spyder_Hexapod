#pragma once
#include "FourLegTest.h"
#include "CommandInputs.h"
#include "PcaOutputs.h"

class FourLegSafety : public legtest::InputSafety {
  fourleg::FourLegTest &leg_;
  PcaOutputs &outputs_;
public:
  FourLegSafety(fourleg::FourLegTest &leg,PcaOutputs &outputs):leg_(leg),outputs_(outputs) {}
  void hold() override { leg_.stop(); }
  void release() override {
    // OE goes HIGH before any I2C cleanup: inhibit all twelve outputs immediately.
    outputs_.disableAll(); leg_.release();
  }
};

#pragma once
#include "LegTest.h"
#include "CommandInputs.h"
#include "PcaLegOutput.h"

class LegSafety : public legtest::InputSafety {
  legtest::LegTest &leg_;
  PcaLegOutput &outputs_;
public:
  LegSafety(legtest::LegTest &leg,PcaLegOutput &outputs):leg_(leg),outputs_(outputs) {}
  void hold() override { leg_.stop(); }
  void release() override {
    // OE goes HIGH before any I2C cleanup: inhibit the entire leg immediately.
    outputs_.disableAll(); leg_.release();
  }
};

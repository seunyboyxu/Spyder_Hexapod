#pragma once
#include "Control.h"
namespace spyder {
class TextSink {
public:
  virtual void write(const char *) = 0;
};
class SettingsService {
public:
  virtual Result save() = 0;
  virtual Result load() = 0;
};
class Protocol {
public:
  Protocol(Controller &c, TextSink &s, SettingsService &settings)
      : c_(c), sink_(s), settings_(settings), length_(0), bad_(false),
        overflow_(false), sawCR_(false) {}
  void feed(char byte, uint32_t now);

private:
  Controller &c_;
  TextSink &sink_;
  SettingsService &settings_;
  char buffer_[192];
  uint8_t length_;
  bool bad_, overflow_, sawCR_;
  void execute(uint32_t now);
  void reply(uint16_t id, Result result);
  void status(uint16_t id);
};
} // namespace spyder

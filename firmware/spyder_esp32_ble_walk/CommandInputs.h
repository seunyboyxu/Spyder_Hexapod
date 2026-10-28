#pragma once
#include <stdint.h>
namespace legtest {
class InputSafety {
public:
  virtual void hold() = 0;
  virtual void release() = 0;
};
// Independent USB/Bluetooth line assemblers. No Arduino dependencies.
class CommandInputs {
public:
  static const uint8_t LineSize = 64;
  enum Notice : uint8_t { Held = 1, Released = 2, BadLine = 4, Expired = 8 };
  explicit CommandInputs(InputSafety &safety);
  void feed(uint8_t source, char c, uint32_t now);
  void expire(uint32_t now);
  void cancelAll(bool release = false);
  void resetSource(uint8_t source);
  bool take(uint8_t source, char (&line)[LineSize]);
  uint8_t notice(uint8_t source);
  uint16_t generation() const { return generation_; }
private:
  struct Input {
    char text[LineSize];
    uint32_t lastByte;
    uint8_t used, notices;
    bool complete, discard;
  } input_[2];
  InputSafety &safety_;
  uint16_t generation_;
  void cancel(uint8_t source, bool release, bool discardCurrent);
};
} // namespace legtest

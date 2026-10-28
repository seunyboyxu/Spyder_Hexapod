#pragma once
#include <stdint.h>

namespace servobench {
enum class Result : uint8_t { None, Ok, Invalid, Busy, Unarmed, Fault, Help, Show, Held, Released, InputError };
class Output {
public:
  virtual ~Output() {}
  virtual bool enable(uint16_t pulseUs) = 0;
  virtual bool write(uint16_t pulseUs) = 0;
  virtual void disable() = 0;
  virtual bool interrupted() const { return false; }
};
class Bench {
public:
  explicit Bench(Output &output);
  Result command(const char *line, uint32_t now);
  void tick(uint32_t now);
  void hold();
  void release();
  void fault();
  bool armed() const { return armed_; }
  bool faulted() const { return fault_; }
  bool busy() const { return mode_ != Mode::Idle; }
  uint16_t pulse() const { return pulse_; }
  uint16_t target() const { return target_; }
private:
  enum class Mode : uint8_t { Idle, Test, Sweep };
  Output &output_;
  bool armed_, fault_;
  Mode mode_;
  uint8_t phase_;
  uint16_t pulse_, target_;
  uint32_t lastTick_, testStarted_;
  bool apply(uint16_t pulse);
};
class Inputs {
public:
  explicit Inputs(Bench &bench);
  Result feed(uint8_t source, char byte, uint32_t now);
  Result expire(uint32_t now);
  void disconnect();
  void resetSource(uint8_t source);
  void clear();
private:
  struct Line { char text[64]; uint8_t used; bool discard; uint32_t last; } lines_[2];
  Bench &bench_;
  void cancel(uint8_t source, bool release, bool discardCurrent);
};
// PCA9685 internal-clock assumption: 25 MHz, prescale 121 (~50.03 Hz).
// Endpoint conversion rounds inward; oscillator accuracy still needs measurement.
uint16_t pulseTicks(uint16_t pulseUs);
} // namespace servobench

#pragma once
#include <stdint.h>

namespace legtest {
const uint8_t JointCount = 3;
// Estimated shaft-to-shaft geometry, in mm; measure your fitted leg.
const float HipLength = 27.5f, ThighLength = 55.0f, ShinLength = 74.0f;
struct Calibration {
  int16_t zero, minimum, maximum; // Tenths of Servo-library degrees.
  int8_t direction;
  uint8_t checked;
};
struct Settings { Calibration joint[JointCount]; };
struct Point { float x, y, z; }; // Outward, sideways/fore-aft, upward; mm.
bool validSettings(const Settings &settings);
const uint8_t SavedSize = 32;
bool encodeSettings(const Settings &settings, uint8_t (&bytes)[SavedSize]);
bool decodeSettings(const uint8_t (&bytes)[SavedSize], Settings &settings);
bool solve(const Point &foot, float (&angle)[JointCount]);
uint16_t pulseFor(int16_t angle);
class Outputs {
public:
  virtual void attach(uint8_t joint, uint16_t pulse) = 0;
  virtual void write(uint8_t joint, uint16_t pulse) = 0;
  virtual void detach(uint8_t joint) = 0;
};
enum class Motion : uint8_t { Stand, Straight, Step };
class LegTest {
public:
  explicit LegTest(Outputs &outputs);
  void begin(uint32_t now);
  // Mutable input is tokenized. Success is "OK"; rejection starts "ERR".
  const char *command(char *line);
  // Validate against a value copy with silent outputs; never changes this leg.
  const char *validateCommand(const char *line) const;
  void tick(uint32_t now,bool advance = true);
  bool atTarget() const;
  bool canStart(Motion motion) const;
  void advanceStep();
  void resetClock(uint32_t now) { lastTick_=now; remainder_=0; }
  void stop();
  void release();
  bool applySettings(const Settings &settings);
  const Settings &settings() const { return settings_; }
  bool attached(uint8_t i) const { return attached_[i]; }
  bool anyAttached() const;
  bool busy() const;
  bool stepping() const { return cyclesLeft_ != 0; }
  // Walking: true if every joint is checked, attached and still, and foot
  // position p is inside this leg's limits.
  bool canTrack(const Point &p) const;
  // Walking: move each joint at most maxStep tenths of a degree towards the
  // pose for p, immediately. arrived is set when all joints are there.
  bool track(const Point &p, int16_t maxStep, bool &arrived);
  int16_t position(uint8_t i) const { return position_[i]; }
  uint8_t speed() const { return speed_; }

private:
  Outputs *out_;
  Settings settings_;
  bool attached_[JointCount];
  int16_t position_[JointCount], target_[JointCount];
  uint32_t lastTick_;
  uint16_t remainder_;
  uint8_t speed_, cyclesLeft_, phase_;
  bool ready() const;
  bool targetsFor(const Point &foot, int16_t (&target)[JointCount]) const;
  void setTargets(const int16_t (&target)[JointCount]);
  bool startPose(const Point &foot);
  bool startSteps(uint8_t cycles);
};
} // namespace legtest

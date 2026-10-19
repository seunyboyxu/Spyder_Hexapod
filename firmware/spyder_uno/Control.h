#pragma once
#include <stdint.h>
namespace spyder {
const uint8_t JointCount = 12;
// User-selected default unloaded envelope; not a measured angular range.
const uint16_t BenchMinimumPulse = 1000;
const uint16_t BenchMaximumPulse = 2000;
const uint16_t BenchCenterPulse = 1500;
enum class Result : uint8_t {
  Ok,
  Invalid,
  Range,
  State,
  Calibration,
  Pose,
  Overflow,
  Storage
};
struct JointConfig {
  uint16_t minimum, maximum, initial;
  int8_t direction;
  bool calibrated;
};
struct JointState {
  uint16_t commanded, target;
  bool attached;
};
struct Config {
  JointConfig joints[JointCount];
  uint16_t standing[JointCount];
  bool standingValid;
};
class Outputs {
public:
  virtual void attach(uint8_t joint, uint8_t pin, uint16_t pulse) = 0;
  virtual void write(uint8_t joint, uint16_t pulse) = 0;
  virtual void detach(uint8_t joint) = 0;
};
bool validConfig(const Config &config);
class Controller {
public:
  explicit Controller(Outputs &outputs);
  void begin(uint32_t now);
  static uint8_t pin(uint8_t joint) { return joint + 2; }
  Result arm(uint8_t joint, uint16_t pulse, uint32_t now);
  Result move(uint8_t joint, uint16_t pulse, uint16_t speed);
  Result center(uint8_t joint, uint32_t now);
  Result sweep(uint8_t joint, uint16_t minimum, uint16_t maximum,
               uint16_t speed);
  int8_t benchJoint() const { return benchJoint_; }
  uint8_t sweepPhase() const { return sweepPhase_; }
  Result configure(uint8_t joint, const JointConfig &config);
  void stop();
  void release();
  Result resume(uint32_t now);
  void heartbeat(uint32_t now) { heartbeat_ = now; }
  void tick(uint32_t now);
  Result captureStanding();
  Result stand(uint16_t speed);
  Result setStanding(const uint16_t (&pulses)[JointCount]);
  Result applyConfig(const Config &config);
  const Config &config() const { return config_; }
  const JointState &joint(uint8_t i) const { return state_[i]; }
  bool stopped() const { return stopped_; }
  bool allDetached() const;

private:
  Outputs &out_;
  Config config_;
  JointState state_[JointCount];
  uint16_t speed_[JointCount], remainder_[JointCount];
  uint32_t heartbeat_, lastTick_;
  bool stopped_;
  int8_t benchJoint_;
  uint8_t sweepPhase_;
  uint16_t sweepMinimum_, sweepMaximum_;
};
} // namespace spyder

#include "Control.h"
namespace spyder {
static bool validJoint(const JointConfig &j) {
  return j.minimum >= 1000 && j.maximum <= 2000 && j.minimum < j.maximum &&
         j.initial >= j.minimum && j.initial <= j.maximum &&
         (j.direction == 1 || j.direction == -1);
}
bool validConfig(const Config &c) {
  for (uint8_t i = 0; i < JointCount; ++i) {
    if (!validJoint(c.joints[i]))
      return false;
    if (c.standingValid &&
        (!c.joints[i].calibrated || c.standing[i] < c.joints[i].minimum ||
         c.standing[i] > c.joints[i].maximum))
      return false;
  }
  return true;
}
Controller::Controller(Outputs &o)
    : out_(o), heartbeat_(0), lastTick_(0), stopped_(false) {
  config_.standingValid = false;
  for (uint8_t i = 0; i < JointCount; ++i) {
    config_.joints[i] = JointConfig{1400, 1600, 1500, 1, false};
    config_.standing[i] = 1500;
    state_[i] = JointState{1500, 1500, false};
    speed_[i] = 50;
    remainder_[i] = 0;
  }
}
void Controller::begin(uint32_t now) {
  release();
  stopped_ = true;
  heartbeat_ = lastTick_ = now;
}
bool Controller::allDetached() const {
  for (uint8_t i = 0; i < JointCount; ++i)
    if (state_[i].attached)
      return false;
  return true;
}
Result Controller::arm(uint8_t i, uint16_t p, uint32_t now) {
  if (uint32_t(now - heartbeat_) >= 1500)
    stop();
  if (i >= JointCount)
    return Result::Invalid;
  if (stopped_ || state_[i].attached)
    return Result::State;
  if (p < config_.joints[i].minimum || p > config_.joints[i].maximum)
    return Result::Range;
  state_[i] = JointState{p, p, true};
  remainder_[i] = 0;
  lastTick_ = now;
  out_.attach(i, pin(i), p);
  return Result::Ok;
}
Result Controller::move(uint8_t i, uint16_t p, uint16_t speed) {
  if (i >= JointCount)
    return Result::Invalid;
  if (stopped_ || !state_[i].attached)
    return Result::State;
  if (p < config_.joints[i].minimum || p > config_.joints[i].maximum ||
      speed < 10 || speed > 100)
    return Result::Range;
  const int8_t oldDirection = (state_[i].target > state_[i].commanded) -
                              (state_[i].target < state_[i].commanded);
  const int8_t newDirection =
      (p > state_[i].commanded) - (p < state_[i].commanded);
  if (!oldDirection || oldDirection != newDirection)
    remainder_[i] = 0;
  state_[i].target = p;
  speed_[i] = speed;
  return Result::Ok;
}
Result Controller::configure(uint8_t i, const JointConfig &j) {
  if (i >= JointCount)
    return Result::Invalid;
  if (!allDetached())
    return Result::State;
  if (!validJoint(j))
    return Result::Range;
  config_.joints[i] = j;
  config_.standingValid = false;
  return Result::Ok;
}
void Controller::stop() {
  stopped_ = true;
  for (uint8_t i = 0; i < JointCount; ++i) {
    state_[i].target = state_[i].commanded;
    remainder_[i] = 0;
  }
}
void Controller::release() {
  for (uint8_t i = 0; i < JointCount; ++i) {
    if (state_[i].attached)
      out_.detach(i);
    state_[i].attached = false;
    state_[i].target = state_[i].commanded;
    remainder_[i] = 0;
  }
}
Result Controller::resume(uint32_t now) {
  stopped_ = false;
  heartbeat_ = lastTick_ = now;
  return Result::Ok;
}
void Controller::tick(uint32_t now) {
  if (uint32_t(now - heartbeat_) >= 1500)
    stop();
  if (stopped_ || uint32_t(now - lastTick_) < 20)
    return;
  lastTick_ = now;
  for (uint8_t i = 0; i < JointCount; ++i) {
    JointState &s = state_[i];
    if (!s.attached || s.commanded == s.target)
      continue;
    remainder_[i] += speed_[i] * 20;
    uint16_t step = remainder_[i] / 1000;
    remainder_[i] %= 1000;
    const uint16_t distance = s.commanded > s.target ? s.commanded - s.target
                                                     : s.target - s.commanded;
    if (step > distance)
      step = distance;
    if (step) {
      s.commanded =
          s.commanded < s.target ? s.commanded + step : s.commanded - step;
      out_.write(i, s.commanded);
    }
  }
}
Result Controller::captureStanding() {
  if (stopped_)
    return Result::State;
  for (uint8_t i = 0; i < JointCount; ++i) {
    if (!config_.joints[i].calibrated)
      return Result::Calibration;
    if (!state_[i].attached || state_[i].target != state_[i].commanded)
      return Result::State;
  }
  for (uint8_t i = 0; i < JointCount; ++i)
    config_.standing[i] = state_[i].commanded;
  config_.standingValid = true;
  return Result::Ok;
}
Result Controller::setStanding(const uint16_t (&p)[JointCount]) {
  if (!allDetached())
    return Result::State;
  for (uint8_t i = 0; i < JointCount; ++i) {
    if (!config_.joints[i].calibrated)
      return Result::Calibration;
    if (p[i] < config_.joints[i].minimum || p[i] > config_.joints[i].maximum)
      return Result::Range;
  }
  for (uint8_t i = 0; i < JointCount; ++i)
    config_.standing[i] = p[i];
  config_.standingValid = true;
  return Result::Ok;
}
Result Controller::stand(uint16_t speed) {
  if (stopped_)
    return Result::State;
  if (!config_.standingValid)
    return Result::Pose;
  if (speed < 10 || speed > 100 || !validConfig(config_))
    return Result::Range;
  for (uint8_t i = 0; i < JointCount; ++i)
    if (!state_[i].attached)
      return Result::State;
  for (uint8_t i = 0; i < JointCount; ++i) {
    state_[i].target = config_.standing[i];
    speed_[i] = speed;
    remainder_[i] = 0;
  }
  return Result::Ok;
}
Result Controller::applyConfig(const Config &c) {
  if (!allDetached())
    return Result::State;
  if (!validConfig(c))
    return Result::Range;
  config_ = c;
  return Result::Ok;
}
} // namespace spyder

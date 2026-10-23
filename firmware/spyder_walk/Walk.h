#pragma once
#include <stdint.h>
// Leg kinematics and the creep gait. No Arduino calls in here, so the
// maths can be tested on a PC (see tests/walk/).
namespace walk {
const uint8_t LegCount = 4;
struct Vec3 {
  float x, y, z;
};
// Joint angles in degrees, zero at the reference pose described in Config.h.
struct Angles {
  float hip, thigh, foot;
};

// Foot position in the body frame (x forward, y left, z up, origin at the
// body centre at thigh-shaft height) for the given pose.
Vec3 homeFoot(uint8_t leg, float reach, float height);
// Body frame -> that leg's frame (x straight out from the hip shaft).
Vec3 bodyToLeg(uint8_t leg, const Vec3 &p);
// Inverse kinematics in the leg frame. Returns false (and the closest
// reachable answer) if the point is out of reach.
bool solveLeg(const Vec3 &foot, Angles &out);

class Gait {
public:
  Gait();
  // stride: mm per cycle (negative walks backwards).
  // turn: degrees per cycle (positive turns left).
  void walk(float stride, float turn);
  // Finish the current step and stop with all feet down.
  void stop();
  bool active() const { return active_; }
  // Advance by dt milliseconds and write each foot's body-frame position.
  void update(uint16_t dtMs, float height, Vec3 (&feet)[LegCount]);
  float phase() const { return phase_; }

private:
  float phase_, amplitude_;
  float stride_, turn_;             // currently being walked
  float wantStride_, wantTurn_;     // requested
  bool active_, stopping_;
};
} // namespace walk

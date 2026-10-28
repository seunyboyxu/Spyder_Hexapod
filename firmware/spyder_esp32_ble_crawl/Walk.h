#pragma once
#include <stdint.h>
#include "LegTest.h"
// SunFounder/quadpod crawl gait in Spyder's body frame. No Arduino calls, so
// it is tested on a PC. Same interface as spyder_esp32_ble_walk's Walk.h, so
// the copied FourLegTest controller drives it unchanged.
namespace walk {
const uint8_t LegCount = 4;
struct Vec3 {
  float x, y, z;
};
// Standing foot position in the body frame (x forward, y left, z up, origin
// at the body centre at thigh-shaft height): the four-leg test "stand".
Vec3 homeFoot(uint8_t leg);
// Body frame -> that leg's LegTest frame (x outward, y counter-clockwise).
legtest::Point toLeg(uint8_t leg, const Vec3 &p);
// How far (mm) the body centre is inside triangle abc, seen from above.
// Negative when outside.
float supportMargin(const Vec3 &a, const Vec3 &b, const Vec3 &c);

class Gait {
public:
  Gait();
  // stride: mm per step (negative walks backwards).
  // turn: degrees per step (positive turns left). Turning wins if both set.
  void walk(float stride, float turn);
  // Finish the current move, then return to standing with all feet down.
  void stop();
  bool active() const { return active_; }
  bool stopping() const { return stopping_; }
  // Advance dt ms and write each foot's body-frame position. Does nothing
  // once inactive. Returns how many full cycles completed during this call.
  uint8_t update(uint16_t dtMs, Vec3 (&feet)[LegCount]);

private:
  enum Kind : uint8_t { Shift, Swing };
  // One straight move: all feet together (Shift), or one foot lifted,
  // carried and lowered (Swing).
  struct Move {
    Kind kind;
    uint8_t leg;
    bool endsCycle;  // the last lift of a steady cycle
    float from[LegCount][2], to[LegCount][2];
    float ms;
  };
  float at_[LegCount][2];  // foot x/y when nothing is moving
  float lean_[2];          // body offset added to the nominal pose
  Move move_;
  float elapsed_;
  bool moving_;
  int8_t mode_, want_;     // 0 none, 1 forward, 2 back, 3 left, 4 right
  float stride_, turn_, wantStride_, wantTurn_;
  int8_t q_[LegCount];     // steps from stance point, steady gait only
  uint8_t stage_;          // 0..5 within a steady cycle
  bool steady_;
  int8_t pending_;         // leg to lift after the current lean, or -1
  float pendingTo_[2];
  bool pendingEndsCycle_;
  bool homing_;            // returning to stand before a direction change
  bool active_, stopping_;

  void nominal(uint8_t leg, int8_t q, int8_t mode, float stride, float turn,
               float (&p)[2]) const;
  void target(uint8_t leg, float (&p)[2]) const;
  bool bestLean(uint8_t lifted, const float (&support)[LegCount][2],
                float (&lean)[2], float &margin) const;
  void startShift(const float (&to)[LegCount][2]);
  void startSwing(uint8_t leg, const float (&to)[2], bool endsCycle);
  void lift(uint8_t leg, const float (&nominalTo)[2], const float (&lean)[2],
            bool endsCycle);
  bool transition();
  void next();
};
} // namespace walk

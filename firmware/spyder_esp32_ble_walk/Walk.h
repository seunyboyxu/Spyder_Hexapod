#pragma once
#include <stdint.h>
#include "LegTest.h"
// Body-frame crawl gait. No Arduino calls, so it is tested on a PC.
namespace walk {
const uint8_t LegCount = 4;
struct Vec3 {
  float x, y, z;
};
// Standing foot position in the body frame (x forward, y left, z up,
// origin at the body centre at thigh-shaft height).
Vec3 homeFoot(uint8_t leg);
// Body frame -> that leg's LegTest frame (x outward, y counter-clockwise).
legtest::Point toLeg(uint8_t leg, const Vec3 &p);

class Gait {
public:
  Gait();
  // stride: mm per step (negative walks backwards).
  // turn: degrees per cycle (positive turns left).
  void walk(float stride, float turn);
  // Finish the current step and stop with all feet down.
  void stop();
  bool active() const { return active_; }
  bool stopping() const { return stopping_; }
  // Advance dt ms and write each foot's body-frame position. Does nothing
  // once inactive. Returns how many full cycles completed during this call.
  uint8_t update(uint16_t dtMs, Vec3 (&feet)[LegCount]);

private:
  float phase_, amplitude_;
  float stride_, turn_, wantStride_, wantTurn_;
  bool active_, stopping_;
};
} // namespace walk

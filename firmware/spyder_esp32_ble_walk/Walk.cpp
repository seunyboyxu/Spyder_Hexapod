#include "Walk.h"
#include "WalkConfig.h"
#include <math.h>
namespace walk {
static const float DegToRad = 3.14159265f / 180.0f;

static float mountRad(uint8_t leg) { return LegMountDeg[leg] * DegToRad; }

Vec3 homeFoot(uint8_t leg) {
  const float a = mountRad(leg), r = HipRadiusMm + StandReachMm;
  return Vec3{r * cosf(a), r * sinf(a), -StandHeightMm};
}

legtest::Point toLeg(uint8_t leg, const Vec3 &p) {
  const float a = mountRad(leg), c = cosf(a), s = sinf(a);
  const float dx = p.x - HipRadiusMm * c, dy = p.y - HipRadiusMm * s;
  return legtest::Point{dx * c + dy * s, -dx * s + dy * c, p.z};
}

static uint8_t stepSlot(uint8_t leg) {
  for (uint8_t k = 0; k < LegCount; ++k)
    if (StepOrder[k] == leg)
      return k;
  return 0;
}

Gait::Gait()
    : phase_(0), amplitude_(0), stride_(0), turn_(0), wantStride_(0),
      wantTurn_(0), active_(false), stopping_(false) {}

void Gait::walk(float stride, float turn) {
  wantStride_ = stride;
  wantTurn_ = turn;
  stopping_ = false;
  if (!active_) {
    active_ = true;
    phase_ = -0.25f; // first quarter cycle only leans, nothing lifts
    amplitude_ = 0;
    stride_ = stride;
    turn_ = turn;
  }
}

void Gait::stop() { stopping_ = true; }

uint8_t Gait::update(uint16_t dtMs, Vec3 (&feet)[LegCount]) {
  if (!active_)
    return 0;
  if (stopping_ && phase_ < 0) { // stopped before the first step
    active_ = false;
    return 0;
  }
  const float dPhase = float(dtMs) / CycleMs;
  const bool same = stride_ == wantStride_ && turn_ == wantTurn_;
  const float target = stopping_ || !same ? 0.0f : 1.0f;
  if (phase_ >= 0)
    amplitude_ = amplitude_ < target ? fminf(target, amplitude_ + dPhase)
                                     : fmaxf(target, amplitude_ - dPhase);
  if (amplitude_ == 0 && !same && !stopping_) {
    stride_ = wantStride_;
    turn_ = wantTurn_;
  }
  const float old = phase_;
  phase_ += dPhase;
  // Only stop exactly between two steps, when every foot is on the ground.
  if (stopping_ && amplitude_ == 0 && old >= 0 &&
      floorf(old * 4) != floorf(phase_ * 4)) {
    phase_ = floorf(phase_ * 4) / 4;
    active_ = false;
  }
  uint8_t cycles = 0;
  if (phase_ >= 1) {
    phase_ -= 1;
    ++cycles;
  }

  // Each leg owns a quarter of the cycle. In the middle of its quarter it
  // swings; around the quarter boundaries all four feet are down and the
  // body leans across, away from the next leg to lift.
  const float swingStart = 0.125f * FourFeetDown, swingEnd = 0.25f - swingStart;
  const float now = phase_ < 0 ? 0.0f : phase_;
  const uint8_t slot = uint8_t(now * 4) & 3;
  const float v = now - slot * 0.25f; // time into this leg's quarter
  uint8_t from = slot, to = slot;
  float blend = 0;
  if (v < swingStart) {
    from = (slot + 3) & 3;
    blend = 0.5f + 0.5f * v / swingStart;
  } else if (v >= swingEnd) {
    to = (slot + 1) & 3;
    blend = 0.5f * (v - swingEnd) / swingStart;
  }
  blend = blend * blend * (3 - 2 * blend);
  const float leanIn = phase_ < 0 ? 1.0f + phase_ * 4 : 1.0f;
  const float a0 = mountRad(StepOrder[from]), a1 = mountRad(StepOrder[to]);
  // Moving the body away from a leg moves every foot towards it.
  const float shiftX =
      BodyShiftMm * leanIn * ((1 - blend) * cosf(a0) + blend * cosf(a1));
  const float shiftY =
      BodyShiftMm * leanIn * ((1 - blend) * sinf(a0) + blend * sinf(a1));

  const float stance = 1 - (swingEnd - swingStart);
  for (uint8_t i = 0; i < LegCount; ++i) {
    float p = 0, lift = 0;
    if (phase_ >= 0) {
      float u = phase_ - stepSlot(i) * 0.25f;
      if (u < 0)
        u += 1;
      if (u >= swingStart && u < swingEnd) { // lift and carry forward
        const float s = (u - swingStart) / (swingEnd - swingStart);
        p = -0.5f + s * s * (3 - 2 * s);
        lift = LiftMm * sinf(3.14159265f * s);
      } else { // on the ground, pushing back
        float w = u - swingEnd;
        if (w < 0)
          w += 1;
        p = 0.5f - w / stance;
      }
    }
    p *= amplitude_;
    const Vec3 home = homeFoot(i);
    const float a = p * turn_ * DegToRad, c = cosf(a), s = sinf(a);
    feet[i].x = home.x * c - home.y * s + p * stride_ + shiftX;
    feet[i].y = home.x * s + home.y * c + shiftY;
    feet[i].z = home.z + lift;
  }
  return cycles;
}
} // namespace walk

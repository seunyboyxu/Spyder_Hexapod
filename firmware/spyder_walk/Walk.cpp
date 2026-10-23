#include "Walk.h"
#include "Config.h"
#include <math.h>
namespace walk {
static const float DegToRad = 3.14159265f / 180.0f;

static float mountRad(uint8_t leg) { return LEG_MOUNT_DEG[leg] * DegToRad; }

Vec3 homeFoot(uint8_t leg, float reach, float height) {
  const float a = mountRad(leg), r = HIP_RADIUS_MM + reach;
  return Vec3{r * cosf(a), r * sinf(a), -height};
}

Vec3 bodyToLeg(uint8_t leg, const Vec3 &p) {
  const float a = mountRad(leg), c = cosf(a), s = sinf(a);
  const float dx = p.x - HIP_RADIUS_MM * c, dy = p.y - HIP_RADIUS_MM * s;
  return Vec3{dx * c + dy * s, -dx * s + dy * c, p.z};
}

static float clampf(float v, float lo, float hi) {
  return v < lo ? lo : v > hi ? hi : v;
}

bool solveLeg(const Vec3 &foot, Angles &out) {
  const float F = FEMUR_MM, T = TIBIA_MM;
  out.hip = atan2f(foot.y, foot.x) / DegToRad;
  const float r = sqrtf(foot.x * foot.x + foot.y * foot.y) - COXA_MM;
  float d = sqrtf(r * r + foot.z * foot.z);
  const float dMin = fabsf(F - T) + 1.0f, dMax = F + T - 1.0f;
  const bool reachable = d >= dMin && d <= dMax;
  d = clampf(d, dMin, dMax);
  const float down = atan2f(foot.z, r);
  const float spread = acosf(clampf((F * F + d * d - T * T) / (2 * F * d), -1, 1));
  const float knee = acosf(clampf((F * F + T * T - d * d) / (2 * F * T), -1, 1));
  out.thigh = (down + spread) / DegToRad;
  out.foot = 90.0f - knee / DegToRad;
  return reachable;
}

static uint8_t stepSlot(uint8_t leg) {
  for (uint8_t k = 0; k < LegCount; ++k)
    if (STEP_ORDER[k] == leg)
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

void Gait::update(uint16_t dtMs, float height, Vec3 (&feet)[LegCount]) {
  if (!active_)
    return;
  if (stopping_ && phase_ < 0) { // stopped before the first step
    active_ = false;
    return;
  }
  const float dPhase = float(dtMs) / CYCLE_MS;
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
  if (phase_ >= 1)
    phase_ -= 1;

  // Each leg owns a quarter of the cycle. In the middle of its quarter it
  // swings; around the quarter boundaries all four feet are down and the
  // body leans across, away from the next leg to lift.
  const float swingStart = 0.125f * FOUR_FEET_DOWN, swingEnd = 0.25f - swingStart;
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
  const float a0 = mountRad(STEP_ORDER[from]), a1 = mountRad(STEP_ORDER[to]);
  // Moving the body away from a leg moves every foot towards it.
  const float shiftX =
      BODY_SHIFT_MM * leanIn * ((1 - blend) * cosf(a0) + blend * cosf(a1));
  const float shiftY =
      BODY_SHIFT_MM * leanIn * ((1 - blend) * sinf(a0) + blend * sinf(a1));

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
        lift = LIFT_MM * sinf(3.14159265f * s);
      } else { // on the ground, pushing back
        float w = u - swingEnd;
        if (w < 0)
          w += 1;
        p = 0.5f - w / stance;
      }
    }
    p *= amplitude_;
    const Vec3 home = homeFoot(i, STAND_REACH_MM, height);
    const float a = p * turn_ * DegToRad, c = cosf(a), s = sinf(a);
    feet[i].x = home.x * c - home.y * s + p * stride_ + shiftX;
    feet[i].y = home.x * s + home.y * c + shiftY;
    feet[i].z = home.z + lift;
  }
}
} // namespace walk

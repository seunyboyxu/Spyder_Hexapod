// SunFounder/quadpod crawl gait for Spyder's body.
//
// Sequence source: hpsaturn/quadpod src/servos.cpp (GPL-3.0), whose header
// credits SunFounder's "Body Movement Crawling robot" (2015) and Wilmar's
// remix. Only the step sequences (which leg lifts when, where each foot is
// placed, when the body moves) are reproduced here, re-expressed in Spyder's
// body frame; no quadpod code is copied. quadpod's servo mapping is replaced
// by Spyder's calibration (LegTest), and a lean before each lift replaces
// the balance that quadpod's square stance gave for free.
#include "Walk.h"
#include "WalkConfig.h"
#include <math.h>
namespace walk {
static const float DegToRad = 3.14159265f / 180.0f;
static const float LeanStepMm = 2.5f;

static float mountRad(uint8_t leg) { return LegMountDeg[leg] * DegToRad; }

// Stance direction: the mount direction turned StanceYawDeg towards the side.
static float stanceRad(uint8_t leg) {
  const float m = LegMountDeg[leg];
  const float side = m > 0 ? 1.0f : -1.0f, front = fabsf(m) < 90 ? 1.0f : -1.0f;
  return (m + side * front * StanceYawDeg) * DegToRad;
}

Vec3 homeFoot(uint8_t leg) {
  const float a = mountRad(leg);
  return Vec3{HipRadiusMm * cosf(a) + StandReachMm * cosf(a),
              HipRadiusMm * sinf(a) + StandReachMm * sinf(a), -StandHeightMm};
}

legtest::Point toLeg(uint8_t leg, const Vec3 &p) {
  const float a = mountRad(leg), c = cosf(a), s = sinf(a);
  const float dx = p.x - HipRadiusMm * c, dy = p.y - HipRadiusMm * s;
  return legtest::Point{dx * c + dy * s, -dx * s + dy * c, p.z};
}

float supportMargin(const Vec3 &a, const Vec3 &b, const Vec3 &c) {
  const Vec3 v[3] = {a, b, c};
  const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  if (fabsf(area) < 1e-3f)
    return -1e9f;
  float margin = 1e9f;
  for (int k = 0; k < 3; ++k) {
    const Vec3 &p = v[k], &q = v[(k + 1) % 3];
    const float d = ((q.x - p.x) * (0 - p.y) - (q.y - p.y) * (0 - p.x)) /
                    hypotf(q.x - p.x, q.y - p.y);
    margin = fminf(margin, area < 0 ? -d : d);
  }
  return margin;
}

static int8_t modeFor(float stride, float turn) {
  if (turn > 0) return 3;
  if (turn < 0) return 4;
  if (stride > 0) return 1;
  if (stride < 0) return 2;
  return 0;
}

static const uint8_t *orderFor(int8_t mode) {
  switch (mode) {
  case 2: return BackOrder;
  case 3: return LeftOrder;
  case 4: return RightOrder;
  default: return ForwardOrder;
  }
}

static uint8_t liftLeg(int8_t mode, uint8_t stage) {
  // Stages 0..5: lift, body, lift, lift, body, lift.
  static const uint8_t slot[6] = {0, 0, 1, 2, 0, 3};
  return orderFor(mode)[slot[stage]];
}

Gait::Gait()
    : elapsed_(0), moving_(false), mode_(0), want_(0), stride_(0), turn_(0),
      wantStride_(0), wantTurn_(0), stage_(0), steady_(false), pending_(-1),
      pendingEndsCycle_(false), homing_(false), active_(false),
      stopping_(false) {
  for (uint8_t i = 0; i < LegCount; ++i) {
    const Vec3 h = homeFoot(i);
    at_[i][0] = h.x;
    at_[i][1] = h.y;
    q_[i] = 0;
  }
  lean_[0] = lean_[1] = 0;
  pendingTo_[0] = pendingTo_[1] = 0;
  move_ = Move{};
}

void Gait::walk(float stride, float turn) {
  if (!active_) {
    for (uint8_t i = 0; i < LegCount; ++i) {
      const Vec3 h = homeFoot(i);
      at_[i][0] = h.x;
      at_[i][1] = h.y;
    }
    lean_[0] = lean_[1] = 0;
    elapsed_ = 0;
    moving_ = steady_ = false;
    pending_ = -1;
    mode_ = 0;
    homing_ = false;
    active_ = true;
  }
  stopping_ = false;
  want_ = modeFor(stride, turn);
  wantStride_ = stride;
  wantTurn_ = turn;
}

void Gait::stop() { stopping_ = true; }

void Gait::nominal(uint8_t leg, int8_t q, int8_t mode, float stride,
                   float turn, float (&p)[2]) const {
  const float a = mountRad(leg), s = stanceRad(leg);
  float x = HipRadiusMm * cosf(a) + StandReachMm * cosf(s);
  float y = HipRadiusMm * sinf(a) + StandReachMm * sinf(s);
  if (mode >= 3) { // rotate about the body centre
    const float r = q * turn * DegToRad, c = cosf(r), n = sinf(r);
    p[0] = x * c - y * n;
    p[1] = x * n + y * c;
  } else {
    p[0] = x + q * stride;
    p[1] = y;
  }
}

// Where each foot must be before the next steady cycle (or to stand).
void Gait::target(uint8_t leg, float (&p)[2]) const {
  if (stopping_ || want_ == 0 || homing_) {
    const Vec3 h = homeFoot(leg);
    p[0] = h.x;
    p[1] = h.y;
    return;
  }
  const uint8_t *order = orderFor(want_);
  int8_t q = 0;
  if (leg == order[0]) q = -1;
  if (leg == order[3]) q = 1;
  nominal(leg, q, want_, wantStride_, wantTurn_, p);
}

// Smallest body lean (all feet moved by the same vector) that keeps the body
// centre MinMarginMm inside the three supporting feet while `lifted` is up.
// Falls back to the largest margin reachable within MaxLeanMm.
bool Gait::bestLean(uint8_t lifted, const float (&support)[LegCount][2],
                    float (&lean)[2], float &margin) const {
  const int n = int(MaxLeanMm / LeanStepMm);
  bool met = false;
  float bestNorm = 1e9f;
  margin = -1e9f;
  lean[0] = lean[1] = 0;
  for (int ix = -n; ix <= n; ++ix)
    for (int iy = -n; iy <= n; ++iy) {
      const float lx = ix * LeanStepMm, ly = iy * LeanStepMm;
      const float norm = hypotf(lx, ly);
      if (norm > MaxLeanMm + 1e-3f) continue;
      Vec3 s[3];
      uint8_t k = 0;
      for (uint8_t j = 0; j < LegCount; ++j)
        if (j != lifted)
          s[k++] = Vec3{support[j][0] + lx, support[j][1] + ly, 0};
      const float m = supportMargin(s[0], s[1], s[2]);
      const bool ok = m >= MinMarginMm;
      bool better;
      if (ok != met) better = ok;
      else if (ok) better = norm < bestNorm - 1e-3f ||
                           (fabsf(norm - bestNorm) <= 1e-3f && m > margin);
      else better = m > margin;
      if (better) {
        met = ok;
        bestNorm = norm;
        margin = m;
        lean[0] = lx;
        lean[1] = ly;
      }
    }
  return met;
}

void Gait::startShift(const float (&to)[LegCount][2]) {
  float far = 0;
  for (uint8_t j = 0; j < LegCount; ++j) {
    move_.from[j][0] = at_[j][0];
    move_.from[j][1] = at_[j][1];
    move_.to[j][0] = to[j][0];
    move_.to[j][1] = to[j][1];
    far = fmaxf(far, hypotf(to[j][0] - at_[j][0], to[j][1] - at_[j][1]));
  }
  move_.kind = Shift;
  move_.leg = 0;
  move_.endsCycle = false;
  move_.ms = fmaxf(20.0f, 1000.0f * far / BodySpeedMmS);
  moving_ = true;
}

void Gait::startSwing(uint8_t leg, const float (&to)[2], bool endsCycle) {
  for (uint8_t j = 0; j < LegCount; ++j) {
    move_.from[j][0] = move_.to[j][0] = at_[j][0];
    move_.from[j][1] = move_.to[j][1] = at_[j][1];
  }
  move_.to[leg][0] = to[0];
  move_.to[leg][1] = to[1];
  const float carry = hypotf(to[0] - at_[leg][0], to[1] - at_[leg][1]);
  move_.kind = Swing;
  move_.leg = leg;
  move_.endsCycle = endsCycle;
  move_.ms = 1000.0f * (2 * LiftMm + carry) / LegSpeedMmS;
  moving_ = true;
}

// Lean if needed (all feet down), then lift `leg` to nominalTo + lean.
void Gait::lift(uint8_t leg, const float (&nominalTo)[2],
                const float (&lean)[2], bool endsCycle) {
  const float dx = lean[0] - lean_[0], dy = lean[1] - lean_[1];
  if (hypotf(dx, dy) > 0.05f) {
    float to[LegCount][2];
    for (uint8_t j = 0; j < LegCount; ++j) {
      to[j][0] = at_[j][0] + dx;
      to[j][1] = at_[j][1] + dy;
    }
    lean_[0] = lean[0];
    lean_[1] = lean[1];
    pending_ = int8_t(leg);
    pendingTo_[0] = nominalTo[0] + lean[0];
    pendingTo_[1] = nominalTo[1] + lean[1];
    pendingEndsCycle_ = endsCycle;
    startShift(to);
    return;
  }
  const float to[2] = {nominalTo[0] + lean_[0], nominalTo[1] + lean_[1]};
  startSwing(leg, to, endsCycle);
}

// Move towards target(): lift the off-target leg needing the least lean,
// one at a time, then remove the lean. False once everything is in place.
bool Gait::transition() {
  float base[LegCount][2], goal[LegCount][2];
  for (uint8_t j = 0; j < LegCount; ++j) {
    base[j][0] = at_[j][0] - lean_[0];
    base[j][1] = at_[j][1] - lean_[1];
    target(j, goal[j]);
  }
  int8_t best = -1;
  bool bestMet = false;
  float bestNorm = 1e9f, bestMargin = -1e9f, bestLeanV[2] = {0, 0};
  for (uint8_t i = 0; i < LegCount; ++i) {
    if (hypotf(base[i][0] - goal[i][0], base[i][1] - goal[i][1]) < 0.5f)
      continue;
    float lean[2], margin;
    const bool met = bestLean(i, base, lean, margin);
    const float norm = hypotf(lean[0], lean[1]);
    bool better;
    if (best < 0) better = true;
    else if (met != bestMet) better = met;
    else if (met) better = norm < bestNorm - 1e-3f ||
                         (fabsf(norm - bestNorm) <= 1e-3f && margin > bestMargin);
    else better = margin > bestMargin;
    if (better) {
      best = int8_t(i);
      bestMet = met;
      bestNorm = norm;
      bestMargin = margin;
      bestLeanV[0] = lean[0];
      bestLeanV[1] = lean[1];
    }
  }
  if (best >= 0) {
    lift(uint8_t(best), goal[best], bestLeanV, false);
    return true;
  }
  if (hypotf(lean_[0], lean_[1]) > 0.05f) {
    lean_[0] = lean_[1] = 0;
    startShift(goal);
    return true;
  }
  return false;
}

void Gait::next() {
  for (uint8_t guard = 0; guard < 4; ++guard) {
    if (pending_ >= 0) {
      const uint8_t leg = uint8_t(pending_);
      pending_ = -1;
      startSwing(leg, pendingTo_, pendingEndsCycle_);
      return;
    }
    const bool changing = stopping_ || want_ == 0 || !steady_ ||
                          want_ != mode_ || wantStride_ != stride_ ||
                          wantTurn_ != turn_;
    if (changing) {
      // Leaving a steady gait: stand first, then start the new direction, so
      // no foot is ever placed for a mix of two directions.
      if (steady_) homing_ = true;
      steady_ = false;
      if (transition()) return;
      if (homing_ && !stopping_ && want_ != 0) {
        homing_ = false;
        continue;
      }
      homing_ = false;
      if (stopping_ || want_ == 0) {
        active_ = false;
        return;
      }
      // In the start pose: begin quadpod's cycle.
      steady_ = true;
      mode_ = want_;
      stride_ = wantStride_;
      turn_ = wantTurn_;
      stage_ = 0;
      const uint8_t *order = orderFor(mode_);
      q_[order[0]] = -1;
      q_[order[1]] = 0;
      q_[order[2]] = 0;
      q_[order[3]] = 1;
      continue;
    }
    if (stage_ == 1 || stage_ == 4) { // body moves: every foot one step back
      float to[LegCount][2];
      for (uint8_t j = 0; j < LegCount; ++j) {
        --q_[j];
        nominal(j, q_[j], mode_, stride_, turn_, to[j]);
        to[j][0] += lean_[0];
        to[j][1] += lean_[1];
      }
      ++stage_;
      startShift(to);
      return;
    }
    const uint8_t leg = liftLeg(mode_, stage_);
    float base[LegCount][2], dest[2], lean[2], margin;
    for (uint8_t j = 0; j < LegCount; ++j)
      nominal(j, q_[j], mode_, stride_, turn_, base[j]);
    q_[leg] = int8_t(q_[leg] + 2); // -1 -> +1
    nominal(leg, q_[leg], mode_, stride_, turn_, dest);
    bestLean(leg, base, lean, margin);
    const bool endsCycle = stage_ == 5;
    stage_ = endsCycle ? 0 : uint8_t(stage_ + 1);
    lift(leg, dest, lean, endsCycle);
    return;
  }
}

uint8_t Gait::update(uint16_t dtMs, Vec3 (&feet)[LegCount]) {
  if (!active_)
    return 0;
  uint8_t cycles = 0;
  elapsed_ += dtMs;
  for (uint8_t guard = 0; guard < 16; ++guard) {
    if (!moving_) {
      next();
      if (!active_) break;
    }
    if (elapsed_ < move_.ms) break;
    elapsed_ -= move_.ms;
    for (uint8_t j = 0; j < LegCount; ++j) {
      at_[j][0] = move_.to[j][0];
      at_[j][1] = move_.to[j][1];
    }
    if (move_.endsCycle) ++cycles;
    moving_ = false;
  }
  for (uint8_t j = 0; j < LegCount; ++j)
    feet[j] = Vec3{at_[j][0], at_[j][1], -StandHeightMm};
  if (!active_ || !moving_) {
    elapsed_ = 0;
    return cycles;
  }
  if (move_.kind == Shift) {
    const float t = fminf(1.0f, elapsed_ / move_.ms);
    for (uint8_t j = 0; j < LegCount; ++j) {
      feet[j].x = move_.from[j][0] + (move_.to[j][0] - move_.from[j][0]) * t;
      feet[j].y = move_.from[j][1] + (move_.to[j][1] - move_.from[j][1]) * t;
    }
    return cycles;
  }
  // Swing: straight up, across, straight down at LegSpeedMmS (as quadpod).
  const uint8_t i = move_.leg;
  const float up = 1000.0f * LiftMm / LegSpeedMmS;
  const float across = move_.ms - 2 * up, e = elapsed_;
  float t = 0, z = 0;
  if (e < up) z = LiftMm * e / up;
  else if (e < up + across) { z = LiftMm; t = across > 0 ? (e - up) / across : 1; }
  else { z = LiftMm * fmaxf(0.0f, 1 - (e - up - across) / up); t = 1; }
  feet[i].x = move_.from[i][0] + (move_.to[i][0] - move_.from[i][0]) * t;
  feet[i].y = move_.from[i][1] + (move_.to[i][1] - move_.from[i][1]) * t;
  feet[i].z = -StandHeightMm + z;
  return cycles;
}
} // namespace walk

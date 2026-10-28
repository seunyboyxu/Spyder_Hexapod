// Host checks for firmware/spyder_esp32_ble_crawl: the SunFounder/quadpod
// crawl sequences in Spyder's body frame, and the walking commands in its
// copy of FourLegTest. No hardware.
#include "FourLegTest.h"
#include "WalkConfig.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace fourleg;
using walk::Vec3;

struct Hardware : Channels {
  uint16_t active = 0, pulse[12] = {};
  unsigned writes = 0;
  void attach(uint8_t c, uint16_t p) override { active |= 1U << c; write(c, p); }
  void write(uint8_t c, uint16_t p) override {
    assert(c < 12 && p >= 1000 && p <= 2000);
    pulse[c] = p;
    ++writes;
  }
  void detach(uint8_t c) override { active &= uint16_t(~(1U << c)); }
};
static bool send(FourLegTest &rig, const char *text, uint32_t now) {
  char line[64];
  strcpy(line, text);
  return !strcmp(rig.command(line, now), "OK");
}
static void calibrate(FourLegTest &rig, uint32_t &now) {
  assert(send(rig, "leg all", now));
  assert(send(rig, "cal all", now));
  for (int i = 0; i < 2000 && rig.busy(); ++i) rig.tick(now += 20);
  assert(send(rig, "check all", now));
}

struct Stats {
  float margin = 1e9f, angle = 0, rate = 0, cycleS = 0;
  int cycles = 0, lifts = 0;
};

// Run a gait (optionally changing to `then` part-way) and check every 20 ms
// frame: one foot up at a time, body centre inside the other three, joints
// inside the default +-30 deg limits and under the walking speed cap.
static Stats run(float stride, float turn, int stopAfter, float thenStride = 0,
                 float thenTurn = 0, int changeAfter = -1) {
  walk::Gait g;
  Vec3 feet[4];
  float last[4][3];
  for (int i = 0; i < 4; ++i) {
    feet[i] = walk::homeFoot(i);
    assert(legtest::solve(walk::toLeg(i, feet[i]), last[i]));
  }
  g.walk(stride, turn);
  Stats s;
  int firstCycleAt = -1, lastLifted = -1;
  for (int n = 0; n < 200000 && g.active(); ++n) {
    const uint8_t done = g.update(20, feet);
    if (done) {
      s.cycles += done;
      if (firstCycleAt < 0) firstCycleAt = n;
      else if (s.cycleS == 0) s.cycleS = (n - firstCycleAt) * 0.02f;
      if (s.cycles == changeAfter) g.walk(thenStride, thenTurn);
      if (s.cycles >= stopAfter) g.stop();
    }
    int lifted = -1, count = 0;
    for (int i = 0; i < 4; ++i) {
      float a[3];
      assert(legtest::solve(walk::toLeg(i, feet[i]), a));
      for (int j = 0; j < 3; ++j) {
        s.angle = fmaxf(s.angle, fabsf(a[j]));
        s.rate = fmaxf(s.rate, fabsf(a[j] - last[i][j]));
        last[i][j] = a[j];
      }
      if (feet[i].z > -StandHeightMm + 0.5f) { lifted = i; ++count; }
    }
    assert(count <= 1);
    if (lifted >= 0) {
      if (lifted != lastLifted) ++s.lifts;
      Vec3 t[3];
      int k = 0;
      for (int i = 0; i < 4; ++i) if (i != lifted) t[k++] = feet[i];
      s.margin = fminf(s.margin, walk::supportMargin(t[0], t[1], t[2]));
    }
    lastLifted = lifted;
  }
  assert(!g.active());
  for (int i = 0; i < 4; ++i) {
    const Vec3 h = walk::homeFoot(i);
    assert(fabsf(feet[i].x - h.x) < 1e-3f && fabsf(feet[i].y - h.y) < 1e-3f &&
           fabsf(feet[i].z - h.z) < 1e-3f);
  }
  return s;
}

static void gaitGeometry(float stride, float turn, const char *name) {
  const Stats s = run(stride, turn, 3);
  printf("%-6s margin %.1f mm, largest joint %.1f deg, fastest joint %.0f deg/s, "
         "cycle %.1f s, %d lifts for 3 cycles\n",
         name, s.margin, s.angle, s.rate * 50, s.cycleS, s.lifts);
  fflush(stdout);
  assert(s.cycles == 3);
  assert(s.margin >= MinMarginMm - 0.01f);
  assert(s.angle < 29);
  assert(s.rate * 10 < WalkMaxStepTenths);
}

// The cycle is quadpod's: in steady walking the legs lift in the configured
// order, and each foot moves -1 -> +1 steps relative to its stance point.
static void sequenceOrder(float stride, float turn, const uint8_t (&order)[4]) {
  walk::Gait g;
  Vec3 feet[4];
  g.walk(stride, turn);
  int seen[64], n = 0, cycles = 0, lastLifted = -1;
  for (int i = 0; i < 100000 && cycles < 3; ++i) {
    cycles += g.update(20, feet);
    int lifted = -1;
    for (int k = 0; k < 4; ++k)
      if (feet[k].z > -StandHeightMm + 0.5f) lifted = k;
    if (lifted >= 0 && lifted != lastLifted && n < 64) seen[n++] = lifted;
    lastLifted = lifted;
  }
  // The final 8 lifts are two steady cycles.
  assert(n >= 8);
  for (int k = 0; k < 8; ++k) assert(seen[n - 8 + k] == order[k % 4]);
}

static void changeDirection(float s0, float t0, float s1, float t1, const char *name) {
  const Stats s = run(s0, t0, 4, s1, t1, 2);
  printf("%-13s margin %.1f mm, largest joint %.1f deg, fastest joint %.0f deg/s\n",
         name, s.margin, s.angle, s.rate * 50);
  fflush(stdout);
  assert(s.margin >= MinMarginMm - 0.01f && s.angle < 29);
  assert(s.rate * 10 < WalkMaxStepTenths);
}

static void transitions() {
  changeDirection(StrideMm, 0, -StrideMm, 0, "walk->back");
  changeDirection(-StrideMm, 0, StrideMm, 0, "back->walk");
  changeDirection(0, TurnDeg, 0, -TurnDeg, "left->right");
  changeDirection(StrideMm, 0, 0, TurnDeg, "walk->left");
  changeDirection(0, -TurnDeg, -StrideMm, 0, "right->back");
  // Stop requested during start-up still finishes standing.
  walk::Gait g;
  Vec3 feet[4];
  g.walk(StrideMm, 0);
  for (int i = 0; i < 30; ++i) g.update(20, feet);
  g.stop();
  for (int i = 0; i < 100000 && g.active(); ++i) g.update(20, feet);
  assert(!g.active());
  puts("PASS transitions: forward<->back, left<->right, forward->left, early stop");
}

static void controller() {
  Hardware hw;
  FourLegTest rig(hw);
  uint32_t now = 0;
  rig.begin(now);
  assert(!send(rig, "walk", now)); // nothing attached or checked
  calibrate(rig, now);
  assert(hw.active == 0x0fff);
  assert(send(rig, "stand", now));
  for (int i = 0; i < 5000 && rig.busy(); ++i) rig.tick(now += 20);
  uint16_t stand[12];
  memcpy(stand, hw.pulse, sizeof(stand));

  // Finite walk: walks, then returns to the same standing pulses.
  assert(send(rig, "walk 3", now) && rig.walking());
  assert(!send(rig, "stand", now) && !send(rig, "leg 1", now));
  bool moved = false;
  int updates = 0;
  for (; updates < 50000 && rig.busy(); ++updates) {
    rig.tick(now += 20);
    for (int c = 0; c < 12; ++c) moved = moved || hw.pulse[c] != stand[c];
  }
  assert(moved && !rig.walking() && !rig.busy());
  for (int c = 0; c < 12; ++c) assert(abs(hw.pulse[c] - stand[c]) <= 2);
  printf("walk 3 finished standing after %.1f s\n", updates * 0.02);

  // Direction change with a finite count while walking passes preflight.
  assert(send(rig, "back", now));
  for (int i = 0; i < 400; ++i) rig.tick(now += 20);
  assert(rig.walking() && send(rig, "walk 3", now));
  for (int i = 0; i < 50000 && rig.busy(); ++i) rig.tick(now += 20);
  assert(!rig.busy());
  for (int c = 0; c < 12; ++c) assert(abs(hw.pulse[c] - stand[c]) <= 2);

  // halt finishes the current move and stands; stop holds immediately.
  assert(send(rig, "left", now));
  for (int i = 0; i < 300; ++i) rig.tick(now += 20);
  assert(rig.walking() && send(rig, "halt", now));
  for (int i = 0; i < 50000 && rig.busy(); ++i) rig.tick(now += 20);
  assert(!rig.busy());
  for (int c = 0; c < 12; ++c) assert(abs(hw.pulse[c] - stand[c]) <= 2);
  assert(send(rig, "right", now));
  for (int i = 0; i < 300; ++i) rig.tick(now += 20);
  rig.stop();
  assert(!rig.walking() && !rig.busy() && hw.active == 0x0fff);
  const unsigned writes = hw.writes;
  rig.tick(now += 5000);
  assert(hw.writes == writes);
  rig.release();
  assert(hw.active == 0 && !send(rig, "walk", now));

  // Limits too tight for the gait: rejected before anything moves.
  auto settings = rig.settings();
  settings.leg[2].joint[0].minimum = 880;
  settings.leg[2].joint[0].maximum = 920;
  assert(rig.applySettings(settings));
  calibrate(rig, now);
  const unsigned before = hw.writes;
  assert(!send(rig, "walk", now) && !rig.busy() && hw.writes == before);
  puts("PASS crawl controller: preflight, finite walks, direction change, halt, stop, release");
}

int main() {
  gaitGeometry(StrideMm, 0, "walk");
  gaitGeometry(-StrideMm, 0, "back");
  gaitGeometry(0, TurnDeg, "left");
  gaitGeometry(0, -TurnDeg, "right");
  sequenceOrder(StrideMm, 0, ForwardOrder);
  sequenceOrder(-StrideMm, 0, BackOrder);
  sequenceOrder(0, TurnDeg, LeftOrder);
  sequenceOrder(0, -TurnDeg, RightOrder);
  puts("PASS steady lift order matches quadpod for all four directions");
  transitions();
  controller();
}

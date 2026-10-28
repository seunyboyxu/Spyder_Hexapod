// Host checks for firmware/spyder_esp32_ble_walk: gait geometry and the
// walking commands in its copy of FourLegTest. No hardware.
#include "FourLegTest.h"
#include "WalkConfig.h"
#include <cassert>
#include <cmath>
#include <cstdio>
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
  const char *r = rig.command(line, now);
  return !strcmp(r, "OK");
}
static void calibrate(FourLegTest &rig, uint32_t now) {
  assert(send(rig, "leg all", now));
  assert(send(rig, "cal all", now));
  for (int i = 0; i < 2000 && rig.busy(); ++i) rig.tick(now += 20);
  assert(send(rig, "check all", now));
}

// Signed distance from the body centre into triangle abc (x/y only).
static float insideMargin(const Vec3 &a, const Vec3 &b, const Vec3 &c) {
  const Vec3 v[3] = {a, b, c};
  const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  float margin = 1e9f;
  for (int k = 0; k < 3; ++k) {
    const Vec3 &p = v[k], &q = v[(k + 1) % 3];
    float d = ((q.x - p.x) * (0 - p.y) - (q.y - p.y) * (0 - p.x)) /
              hypotf(q.x - p.x, q.y - p.y);
    margin = fminf(margin, area < 0 ? -d : d);
  }
  return margin;
}

// Gait alone: one foot up at a time, balanced, inside the default +-30 deg
// joint limits, and slow enough for the walking rate limit.
static void gaitGeometry(float stride, float turn, const char *name) {
  walk::Gait g;
  Vec3 feet[4];
  float lastAngle[4][3];
  for (int i = 0; i < 4; ++i) {
    float a[3];
    assert(legtest::solve(walk::toLeg(i, walk::homeFoot(i)), a));
    for (int j = 0; j < 3; ++j) lastAngle[i][j] = a[j];
  }
  g.walk(stride, turn);
  float margin = 1e9f, worstAngle = 0, worstRate = 0;
  int updates = 0;
  for (; updates < 6 * CycleMs / 20; ++updates) {
    if (updates == 4 * CycleMs / 20) g.stop();
    g.update(20, feet);
    if (!g.active()) break;
    int lifted = -1, liftedCount = 0;
    for (int i = 0; i < 4; ++i) {
      float a[3];
      assert(legtest::solve(walk::toLeg(i, feet[i]), a));
      for (int j = 0; j < 3; ++j) {
        worstAngle = fmaxf(worstAngle, fabsf(a[j]));
        worstRate = fmaxf(worstRate, fabsf(a[j] - lastAngle[i][j]));
        lastAngle[i][j] = a[j];
      }
      if (feet[i].z > -StandHeightMm + 1) { lifted = i; ++liftedCount; }
    }
    assert(liftedCount <= 1);
    if (lifted >= 0) {
      Vec3 s[3];
      int n = 0;
      for (int i = 0; i < 4; ++i) if (i != lifted) s[n++] = feet[i];
      margin = fminf(margin, insideMargin(s[0], s[1], s[2]));
    }
  }
  assert(!g.active());
  for (int i = 0; i < 4; ++i) assert(fabsf(feet[i].z + StandHeightMm) < 1e-3f);
  printf("%-8s balance margin %.1f mm, largest joint angle %.1f deg, fastest joint %.1f deg/s\n",
         name, margin, worstAngle, worstRate * 50);
  fflush(stdout);
  assert(margin > 5);              // body centre well inside the support triangle
  assert(worstAngle < 29);         // default limits are 60..120 = +-30 deg
  assert(worstRate * 10 < WalkMaxStepTenths); // tracking never lags the gait
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

  // Two forward cycles: walks, then returns to the same standing pulses.
  assert(send(rig, "walk 2", now) && rig.walking() && rig.busy());
  assert(!send(rig, "stand", now) && !send(rig, "leg 1", now));
  bool moved = false;
  int updates = 0;
  for (; updates < 20000 && rig.busy(); ++updates) {
    rig.tick(now += 20);
    for (int c = 0; c < 12; ++c) moved = moved || hw.pulse[c] != stand[c];
  }
  assert(moved && !rig.walking() && !rig.busy());
  for (int c = 0; c < 12; ++c) assert(abs(hw.pulse[c] - stand[c]) <= 2);
  printf("walk 2 finished standing after %.1f s\n", updates * 0.02);

  // halt finishes the current step and stands; stop holds immediately.
  assert(send(rig, "left", now));
  for (int i = 0; i < 300; ++i) rig.tick(now += 20);
  assert(rig.walking() && send(rig, "halt", now));
  for (int i = 0; i < 20000 && rig.busy(); ++i) rig.tick(now += 20);
  assert(!rig.busy());
  for (int c = 0; c < 12; ++c) assert(abs(hw.pulse[c] - stand[c]) <= 2);
  assert(send(rig, "back", now));
  for (int i = 0; i < 300; ++i) rig.tick(now += 20);
  assert(send(rig, "right", now)); // change direction while walking
  for (int i = 0; i < 300; ++i) rig.tick(now += 20);
  rig.stop();
  assert(!rig.walking() && !rig.busy() && hw.active == 0x0fff);
  const unsigned writes = hw.writes;
  rig.tick(now += 5000);
  assert(hw.writes == writes); // held: nothing moves after stop
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
  puts("PASS walk controller: preflight, walk/back/left/right, halt, stop, release");
}

int main() {
  gaitGeometry(StrideMm, 0, "walk");
  gaitGeometry(-StrideMm, 0, "back");
  gaitGeometry(0, TurnDeg, "left");
  gaitGeometry(0, -TurnDeg, "right");
  controller();
}

// PC checks for firmware/spyder_walk's kinematics and gait.
// Run: bash scripts/test-walk.sh
#include "Config.h"
#include "Walk.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace walk;

static const float Rad = 3.14159265f / 180.0f;

// Forward kinematics matching the conventions in Config.h.
static Vec3 forward(const Angles &a) {
  const float thigh = a.thigh * Rad;
  const float shin = thigh - (90.0f + a.foot) * Rad; // lower leg direction
  const float r = COXA_MM + FEMUR_MM * cosf(thigh) + TIBIA_MM * cosf(shin);
  const float z = FEMUR_MM * sinf(thigh) + TIBIA_MM * sinf(shin);
  return Vec3{r * cosf(a.hip * Rad), r * sinf(a.hip * Rad), z};
}

static bool near(float a, float b, float tolerance = 0.05f) {
  return fabsf(a - b) < tolerance;
}

static void referencePoseIsZero() {
  Angles a;
  assert(solveLeg(Vec3{COXA_MM + FEMUR_MM, 0, -TIBIA_MM}, a));
  assert(near(a.hip, 0) && near(a.thigh, 0) && near(a.foot, 0));
}

static void inverseMatchesForward() {
  for (float x = 40; x <= 130; x += 15)
    for (float y = -40; y <= 40; y += 20)
      for (float z = -90; z <= -10; z += 20) {
        Angles a;
        if (!solveLeg(Vec3{x, y, z}, a))
          continue;
        const Vec3 p = forward(a);
        assert(near(p.x, x, 0.1f) && near(p.y, y, 0.1f) && near(p.z, z, 0.1f));
      }
}

static void homeFootIsStraightOut() {
  for (uint8_t i = 0; i < LegCount; ++i) {
    const Vec3 p = bodyToLeg(i, homeFoot(i, STAND_REACH_MM, STAND_HEIGHT_MM));
    assert(near(p.x, STAND_REACH_MM) && near(p.y, 0) &&
           near(p.z, -STAND_HEIGHT_MM));
    Angles a;
    assert(solveLeg(p, a));
    assert(near(a.hip, 0));
  }
}

// Signed distance from the origin (body centre) into triangle abc.
static float insideMargin(const Vec3 &a, const Vec3 &b, const Vec3 &c) {
  const Vec3 v[3] = {a, b, c};
  const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  float margin = 1e9f;
  for (int k = 0; k < 3; ++k) {
    const Vec3 &p = v[k], &q = v[(k + 1) % 3];
    const float len = hypotf(q.x - p.x, q.y - p.y);
    float d = ((q.x - p.x) * (0 - p.y) - (q.y - p.y) * (0 - p.x)) / len;
    if (area < 0)
      d = -d;
    margin = fminf(margin, d);
  }
  return margin;
}

static void walkCycle(float stride, float turn, const char *name) {
  Gait g;
  Vec3 feet[LegCount], last[LegCount];
  for (uint8_t i = 0; i < LegCount; ++i)
    feet[i] = last[i] = homeFoot(i, STAND_REACH_MM, STAND_HEIGHT_MM);
  g.walk(stride, turn);
  float worstMargin = 1e9f, worstJump = 0;
  const float ground = -STAND_HEIGHT_MM;
  int ticks = 0;
  for (; ticks < 6 * CYCLE_MS / UPDATE_MS; ++ticks) {
    if (ticks == 4 * CYCLE_MS / UPDATE_MS)
      g.stop();
    g.update(UPDATE_MS, STAND_HEIGHT_MM, feet);
    if (!g.active())
      break;
    int lifted = -1, liftedCount = 0;
    for (uint8_t i = 0; i < LegCount; ++i) {
      worstJump = fmaxf(worstJump, hypotf(hypotf(feet[i].x - last[i].x,
                                                 feet[i].y - last[i].y),
                                          feet[i].z - last[i].z));
      Angles a;
      assert(solveLeg(bodyToLeg(i, feet[i]), a)); // every foot reachable
      assert(feet[i].z >= ground - 1e-3f);
      if (feet[i].z > ground + 1.0f) {
        lifted = i;
        ++liftedCount;
      }
      last[i] = feet[i];
    }
    assert(liftedCount <= 1); // never more than one foot in the air
    if (lifted >= 0) {
      Vec3 support[3];
      int n = 0;
      for (uint8_t i = 0; i < LegCount; ++i)
        if (i != lifted)
          support[n++] = feet[i];
      worstMargin = fminf(worstMargin,
                          insideMargin(support[0], support[1], support[2]));
    }
  }
  assert(!g.active()); // stop finished within two cycles
  for (uint8_t i = 0; i < LegCount; ++i)
    assert(near(feet[i].z, ground, 1e-3f)); // all feet down when stopped
  printf("%-8s stability margin %.1f mm, largest foot move per update %.2f "
         "mm, stopped after %d updates\n",
         name, worstMargin, worstJump, ticks);
  fflush(stdout);
  assert(worstMargin > 0); // body centre stays over the support triangle
  assert(worstJump < 6);   // no sudden jumps
}

// Over one full cycle the feet on the ground carry the body forward: summed
// over the cycle they slide backwards relative to the body. (The side lean
// moves all feet together and cancels out over a whole cycle.)
static void forwardMovesBodyForward() {
  Gait g;
  Vec3 feet[LegCount];
  g.walk(STRIDE_MM, 0);
  // Skip the lean-in quarter and one cycle of ramp-up.
  for (int t = 0; t < (CYCLE_MS + CYCLE_MS / 4) / UPDATE_MS; ++t)
    g.update(UPDATE_MS, STAND_HEIGHT_MM, feet);
  float travel = 0;
  for (int t = 0; t < CYCLE_MS / UPDATE_MS; ++t) {
    Vec3 before[LegCount];
    for (uint8_t i = 0; i < LegCount; ++i)
      before[i] = feet[i];
    g.update(UPDATE_MS, STAND_HEIGHT_MM, feet);
    float sum = 0;
    int n = 0;
    for (uint8_t i = 0; i < LegCount; ++i)
      if (feet[i].z <= -STAND_HEIGHT_MM + 1e-3f &&
          before[i].z <= -STAND_HEIGHT_MM + 1e-3f) {
        sum += feet[i].x - before[i].x;
        ++n;
      }
    travel -= sum / n;
  }
  printf("forward travel over one cycle %.1f mm (stride %.1f)\n", travel,
         STRIDE_MM);
  fflush(stdout);
  // Each foot pushes back STRIDE_MM over the 85% of the cycle it is down.
  assert(near(travel, STRIDE_MM / (1 - 0.25f * (1 - FOUR_FEET_DOWN)), 0.5f));
}

int main() {
  referencePoseIsZero();
  inverseMatchesForward();
  homeFootIsStraightOut();
  walkCycle(STRIDE_MM, 0, "forward");
  walkCycle(-STRIDE_MM, 0, "back");
  walkCycle(0, TURN_DEG, "left");
  walkCycle(0, -TURN_DEG, "right");
  forwardMovesBodyForward();
  puts("walk tests passed");
}

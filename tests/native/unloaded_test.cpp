#include "Control.h"
#include <cassert>
#include <cstdio>
using namespace spyder;
struct UnloadedOutputs : Outputs {
  bool attached[12] = {};
  uint16_t pulse[12] = {};
  void attach(uint8_t i, uint8_t pin, uint16_t p) override {
    assert(pin == i + 2);
    attached[i] = true;
    pulse[i] = p;
  }
  void write(uint8_t i, uint16_t p) override {
    assert(attached[i]);
    pulse[i] = p;
  }
  void detach(uint8_t i) override { attached[i] = false; }
};
int main() {
  UnloadedOutputs out;
  Controller c(out);
  c.begin(0);
  assert(c.center(0, 0) == Result::State);
  c.resume(0);
  assert(c.center(12, 0) == Result::Invalid);
  c.configure(4, JointConfig{1400, 1600, 1450, -1, true});
  assert(c.center(4, 0) == Result::Ok);
  assert(c.benchJoint() == 4 && c.sweepPhase() == 0);
  assert(c.joint(4).commanded == 1500 && c.joint(4).target == 1500);
  for (int i = 0; i < 12; ++i)
    assert(out.attached[i] == (i == 4));
  const auto &cfg = c.config().joints[4];
  assert(cfg.minimum == 1400 && cfg.maximum == 1600 && cfg.initial == 1450 &&
         cfg.direction == -1 && !cfg.calibrated);
  assert(!c.config().standingValid);
  assert(c.captureStanding() == Result::State);
  assert(c.stand(50) == Result::State);
  assert(c.arm(5, 1500, 0) == Result::State);
  assert(c.move(4, 1510, 100) == Result::State);
  assert(c.center(5, 0) == Result::State);
  assert(c.sweep(5, 1000, 2000, 100) == Result::State);
  assert(c.sweep(4, 1500, 2000, 100) == Result::Range);
  assert(c.sweep(4, 1000, 1500, 100) == Result::Range);
  assert(c.sweep(4, 1000, 2000, 101) == Result::Range);
  assert(c.sweep(4, 1000, 2000, 100) == Result::Ok);
  assert(c.sweepPhase() == 1 && c.joint(4).target == 1000);
  for (uint32_t t = 20; t <= 20000; t += 20) {
    c.heartbeat(t);
    c.tick(t);
    assert(out.pulse[4] >= 1000 && out.pulse[4] <= 2000);
    for (int i = 0; i < 12; ++i)
      assert(out.attached[i] == (i == 4));
    if (t == 5000)
      assert(out.pulse[4] == 1000 && c.sweepPhase() == 2 &&
             c.joint(4).target == 2000);
    if (t == 15000)
      assert(out.pulse[4] == 2000 && c.sweepPhase() == 3 &&
             c.joint(4).target == 1500);
  }
  assert(c.sweepPhase() == 0 && c.joint(4).commanded == 1500 &&
         c.joint(4).target == 1500);
  assert(c.sweep(4, 1000, 2000, 100) == Result::Ok);
  c.heartbeat(20020);
  c.tick(20020);
  const uint16_t held = c.joint(4).commanded;
  c.stop();
  assert(c.sweepPhase() == 0 && c.joint(4).target == held);
  c.resume(20020);
  c.heartbeat(20120);
  c.tick(20120);
  assert(c.joint(4).commanded == held &&
         c.sweep(4, 1000, 2000, 100) == Result::State);
  assert(c.center(4, 20120) == Result::Ok && c.joint(4).target == 1500);
  c.heartbeat(20140);
  c.tick(20140);
  assert(c.joint(4).commanded == 1500);
  assert(c.sweep(4, 1000, 2000, 100) == Result::Ok);
  c.tick(22000);
  assert(c.stopped() && c.sweepPhase() == 0 &&
         c.joint(4).target == c.joint(4).commanded);
  c.release();
  assert(c.benchJoint() == -1 && c.sweepPhase() == 0 && c.allDetached());
  c.resume(22000);
  c.arm(0, 1500, 22000);
  assert(c.center(0, 22000) == Result::State);
  c.release();
  c.tick(24000);
  assert(c.center(0, 24000) == Result::State && c.allDetached());
  c.resume(24000);
  assert(c.center(11, 24000) == Result::Ok);
  c.begin(24001);
  assert(c.benchJoint() == -1 && c.allDetached() && c.stopped());
  std::puts("unloaded: individual centering, bounded one-cycle sweep, "
            "isolation, limits preservation, STOP/watchdog/reset PASS");
}

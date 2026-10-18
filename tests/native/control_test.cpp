#include "Control.h"
#include <cassert>
#include <cstdio>
#include <set>
using namespace spyder;
struct Output : Outputs {
  int attaches = 0, writes = 0, detaches = 0;
  uint16_t pulses[12] = {};
  void attach(uint8_t i, uint8_t pin, uint16_t p) override {
    assert(pin == i + 2);
    ++attaches;
    pulses[i] = p;
  }
  void write(uint8_t i, uint16_t p) override {
    ++writes;
    pulses[i] = p;
  }
  void detach(uint8_t) override { ++detaches; }
};
int main() {
  {
    Output boundary;
    Controller lease(boundary);
    lease.begin(0);
    assert(lease.stopped());
    assert(lease.arm(0, 1500, 0) == Result::State);
    lease.resume(0);
    lease.tick(10000);
    assert(lease.stopped() && lease.allDetached());
    assert(lease.arm(0, 1500, 10000) == Result::State);
    lease.resume(10000);
    lease.arm(0, 1500, 10000);
    lease.begin(10001);
    assert(lease.arm(0, 1500, 10002) == Result::State);
    assert(boundary.attaches == 1);
  }
  Output o;
  Controller c(o);
  c.begin(0);
  c.resume(0);
  assert(o.attaches == 0 && o.writes == 0 && c.allDetached());
  std::set<int> pins;
  for (int i = 0; i < 12; ++i) {
    pins.insert(Controller::pin(i));
    const auto &j = c.config().joints[i];
    assert(j.minimum == 1400 && j.maximum == 1600 && j.initial == 1500 &&
           !j.calibrated);
  }
  assert(pins.size() == 12 && *pins.begin() == 2 && *pins.rbegin() == 13);
  assert(c.arm(12, 1500, 0) == Result::Invalid);
  assert(c.arm(0, 1399, 0) == Result::Range);
  assert(o.attaches == 0);
  JointConfig j = {1400, 1600, 1500, 1, true};
  assert(c.configure(0, j) == Result::Ok);
  for (auto bad : {JointConfig{1600, 1400, 1500, 1, true},
                   JointConfig{999, 1600, 1500, 1, true},
                   JointConfig{1400, 2001, 1500, 1, true},
                   JointConfig{1400, 1600, 1700, 1, true},
                   JointConfig{1400, 1600, 1500, 0, true}}) {
    assert(c.configure(0, bad) != Result::Ok);
    assert(c.config().joints[0].minimum == 1400);
  }
  Config bad = c.config();
  bad.joints[11].maximum = 2100;
  assert(c.applyConfig(bad) != Result::Ok);
  assert(c.arm(0, 1500, 0) == Result::Ok && o.attaches == 1);
  assert(c.configure(0, j) == Result::State);
  c.release();
  assert(c.allDetached());
  assert(c.arm(0, 1500, 0) == Result::Ok);
  assert(c.move(0, 1600, 50) == Result::Ok);
  for (uint32_t t = 20; t <= 2000; t += 20) {
    c.heartbeat(t);
    c.tick(t);
  }
  assert(c.joint(0).commanded == 1600);
  assert(c.move(0, 1601, 50) == Result::Range);
  assert(c.move(0, 1500, 9) == Result::Range);
  assert(c.move(0, 1500, 10) == Result::Ok);
  for (uint32_t t = 2020; t <= 2080; t += 20) {
    c.heartbeat(t);
    c.tick(t);
  }
  assert(c.joint(0).commanded == 1600);
  c.heartbeat(2100);
  c.tick(2100);
  assert(c.joint(0).commanded == 1599);
  c.stop();
  assert(c.stopped() && c.joint(0).target == 1599);
  assert(c.move(0, 1500, 50) == Result::State);
  c.heartbeat(2200);
  assert(c.stopped());
  assert(c.arm(1, 1500, 2200) == Result::State);
  c.resume(2200);
  c.tick(2220);
  assert(c.joint(0).commanded == 1599);
  c.move(0, 1500, 100);
  c.tick(3700);
  assert(c.stopped() && c.joint(0).commanded == 1599);
  c.release();
  assert(c.stopped() && c.allDetached());
  c.resume(UINT32_MAX - 50);
  c.arm(0, 1500, UINT32_MAX - 50);
  c.move(0, 1600, 50);
  c.heartbeat(10);
  c.tick(10);
  assert(!c.stopped() && c.joint(0).commanded == 1501);
  c.heartbeat(1000);
  c.tick(1000);
  assert(c.joint(0).commanded == 1502);
  assert(c.captureStanding() != Result::Ok);
  c.release();
  uint16_t pose[12];
  for (int i = 0; i < 12; ++i)
    pose[i] = 1510;
  assert(c.setStanding(pose) == Result::Calibration);
  for (int i = 0; i < 12; ++i)
    assert(c.configure(i, j) == Result::Ok);
  int before = o.attaches;
  assert(c.setStanding(pose) == Result::Ok && o.attaches == before);
  pose[11] = 1700;
  assert(c.setStanding(pose) == Result::Range);
  assert(c.config().standing[0] == 1510 && c.config().standing[11] == 1510);
  for (int i = 0; i < 12; ++i)
    c.arm(i, 1500, 1000);
  assert(c.captureStanding() == Result::Ok);
  assert(c.stand(50) == Result::Ok);
  c.release();
  c.configure(0, j);
  assert(!c.config().standingValid);
  assert(c.stand(50) != Result::Ok);
  std::puts(
      "control: startup/mapping/limits/config, interpolation, STOP, "
      "watchdog, rollover, pose PASS (9 groups, including reset/idle lease)");
}

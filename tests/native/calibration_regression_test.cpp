#include "Control.h"
#include <cassert>
#include <cstdio>
using namespace spyder;
struct Output : Outputs {
  void attach(uint8_t, uint8_t, uint16_t) override {}
  void write(uint8_t, uint16_t) override {}
  void detach(uint8_t) override {}
};
int main() {
  Output output;
  Controller single(output), repeated(output);
  single.begin(0);
  repeated.begin(0);
  single.resume(0);
  repeated.resume(0);
  single.arm(0, 1500, 0);
  repeated.arm(0, 1500, 0);
  single.move(0, 1550, 10);
  repeated.move(0, 1505, 10);
  for (uint32_t t = 20; t <= 400; t += 20) {
    single.heartbeat(t);
    repeated.heartbeat(t);
    single.tick(t);
    repeated.tick(t);
    if (t % 80 == 0)
      repeated.move(0, 1505 + (t / 80) * 5, 10);
  }
  assert(single.joint(0).commanded == 1504);
  assert(repeated.joint(0).commanded == 1504);
  // Reversal discards the old direction's unfinished fraction.
  repeated.move(0, 1490, 10);
  for (uint32_t t = 420; t <= 480; t += 20) {
    repeated.heartbeat(t);
    repeated.tick(t);
  }
  assert(repeated.joint(0).commanded == 1504);
  repeated.heartbeat(500);
  repeated.tick(500);
  assert(repeated.joint(0).commanded == 1503);
  repeated.stop();
  repeated.resume(500);
  repeated.move(0, 1490, 10);
  repeated.heartbeat(520);
  repeated.tick(520);
  assert(repeated.joint(0).commanded == 1503);
  std::puts("calibration regressions: cadence-independent progress, reversal, "
            "STOP fraction PASS");
}

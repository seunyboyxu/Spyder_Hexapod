#include "LegTest.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vector>
using namespace legtest;
struct Output : Outputs {
  struct Event { uint8_t joint; uint16_t pulse; };
  std::vector<Event> writes;
  unsigned attaches = 0, detaches = 0;
  void attach(uint8_t i, uint16_t p) override { ++attaches; write(i,p); }
  void write(uint8_t i, uint16_t p) override {
    assert(i < 3 && p >= 1000 && p <= 2000);
    writes.push_back(Event{i,p});
  }
  void detach(uint8_t i) override { assert(i < 3); ++detaches; }
};
static bool send(LegTest &leg, const char *text) {
  char line[80]; assert(strlen(text) < sizeof(line)); strcpy(line,text);
  return strcmp(leg.command(line),"OK") == 0;
}
static void settle(LegTest &leg, uint32_t &now) {
  for (int i=0; i<20000 && leg.busy(); ++i) { now+=20; leg.tick(now); }
  assert(!leg.busy());
}
static void prepare(LegTest &leg) {
  for (int i=0;i<3;++i) {
    char cmd[20]; snprintf(cmd,sizeof(cmd),"cal %d",i); assert(send(leg,cmd));
    snprintf(cmd,sizeof(cmd),"zero %d",i); assert(send(leg,cmd));
    snprintf(cmd,sizeof(cmd),"check %d",i); assert(send(leg,cmd));
  }
}
static void calibrationAndBounds() {
  Output out; LegTest leg(out); leg.begin(0); uint32_t now=0;
  assert(out.attaches==0 && !leg.anyAttached());
  assert(!send(leg,"stand"));
  assert(!send(leg,"cal -1") && !send(leg,"cal 3"));
  assert(!send(leg,"cal 0junk") && !send(leg,"cal 0 extra"));
  assert(!send(leg,"move 0 80"));
  assert(send(leg,"cal 0")); assert(out.attaches==1);
  assert(!leg.attached(1) && !leg.attached(2));
  assert(!send(leg,"move 0 nan") && !send(leg,"move 0 inf"));
  assert(!send(leg,"move 0 121") && !send(leg,"move 0 70x"));
  assert(send(leg,"move 0 100"));
  leg.tick(20); now=20;
  assert(leg.position(0)>900 && leg.position(0)<=902);
  assert(!send(leg,"zero 0") && !send(leg,"check 0"));
  settle(leg,now); assert(leg.position(0)==1000);
  assert(send(leg,"zero 0") && leg.settings().joint[0].zero==1000);
  assert(send(leg,"test 0 5")); settle(leg,now);
  assert(leg.position(0)==1050);
  assert(send(leg,"test 0 0")); settle(leg,now);
  assert(send(leg,"flip 0") && send(leg,"test 0 5")); settle(leg,now);
  assert(leg.position(0)==950);
  assert(!send(leg,"check 0"));
  assert(send(leg,"test 0 0")); settle(leg,now);
  assert(send(leg,"check 0"));
  assert(send(leg,"flip 0") && !leg.settings().joint[0].checked);
  assert(send(leg,"off"));
  assert(!send(leg,"limits 0 0 180"));
  assert(send(leg,"limits 0 70 110"));
  Settings bad=leg.settings(); bad.joint[0].direction=0;
  assert(!leg.applySettings(bad));
  bad=leg.settings(); bad.joint[1].checked=2; assert(!validSettings(bad));
  puts("calibration, isolation, input validation and output bounds passed");
}
static void kinematics() {
  float a[3];
  assert(solve(Point{HipLength+ThighLength,0,-ShinLength},a));
  for(int i=0;i<3;++i) assert(fabsf(a[i])<0.01f);
  assert(!solve(Point{1000,0,0},a));
  assert(!solve(Point{NAN,0,0},a));
  assert(!solve(Point{HipLength,0,0},a));
  const Point points[]={{85,0,-60},{130,0,-20},{85,-10,-60},{85,-10,-48},{85,10,-48},{85,10,-60}};
  const float rad=3.14159265358979323846f/180;
  for(const Point &p:points) {
    assert(solve(p,a));
    const float shin=(a[1]-90-a[2])*rad;
    const float r=HipLength+ThighLength*cosf(a[1]*rad)+ShinLength*cosf(shin);
    assert(fabsf(r*cosf(a[0]*rad)-p.x)<0.01f);
    assert(fabsf(r*sinf(a[0]*rad)-p.y)<0.01f);
    assert(fabsf(ThighLength*sinf(a[1]*rad)+ShinLength*sinf(shin)-p.z)<0.01f);
  }
  puts("reference pose, unreachable targets and independent forward kinematics passed");
}
static void posesAndCycles() {
  Output out; LegTest leg(out); leg.begin(0); uint32_t now=0;
  prepare(leg);
  assert(send(leg,"stand")); settle(leg,now);
  assert(send(leg,"straight")); settle(leg,now);
  assert(send(leg,"step 2")); assert(leg.stepping());
  assert(!send(leg,"step 4") && !send(leg,"move 0 95"));
  settle(leg,now); assert(!leg.stepping());
  assert(send(leg,"stand")); assert(!leg.busy());
  for(const auto &e:out.writes) assert(e.pulse>=1000 && e.pulse<=2000);
  // A checked but narrow joint must reject the WHOLE pose before any joint moves.
  assert(send(leg,"off"));
  Settings narrow=leg.settings(); narrow.joint[0].minimum=890;
  narrow.joint[0].maximum=910; assert(leg.applySettings(narrow));
  prepare(leg);
  assert(!send(leg,"step 1")); assert(!leg.busy());
  puts("standing, partial extension, finite cycles and atomic pose preflight passed");
}
static void stopTimingAndRelease() {
  Output out; LegTest leg(out); leg.begin(0); uint32_t now=0; prepare(leg);
  assert(send(leg,"step 3"));
  leg.tick(20); now=20;
  assert(send(leg,"stop")); assert(!leg.busy());
  const size_t n=out.writes.size(); now+=10000; leg.tick(now);
  assert(out.writes.size()==n && leg.anyAttached());
  assert(send(leg,"step 1")); assert(send(leg,"off"));
  assert(!leg.anyAttached() && !leg.busy() && out.detaches==3);
  leg.tick(now+10000); assert(out.writes.size()==n);
  assert(!send(leg,"speed 0") && !send(leg,"speed 21"));
  assert(send(leg,"cal 0") && send(leg,"speed 1") && send(leg,"move 0 100"));
  leg.tick(now+20000); // A delayed update makes only one normal step.
  assert(leg.position(0)<=901);
  now+=20000;
  for(int i=0;i<50;++i) { now+=20; leg.tick(now); }
  assert(leg.position(0)>=910 && leg.position(0)<=911);
  leg.begin(0xfffffff0U); assert(send(leg,"cal 0") && send(leg,"move 0 100"));
  leg.tick(4); assert(leg.position(0)<=901); // millis wrap.
  puts("immediate hold/release, finite cancellation, fractional rates and timer wrap passed");
}
static void storage() {
  Output out; LegTest leg(out); leg.begin(0); prepare(leg);
  uint8_t packet[SavedSize]; assert(encodeSettings(leg.settings(),packet));
  Settings restored;
  assert(decodeSettings(packet,restored));
  assert(validSettings(restored));
  for(int i=0;i<3;++i) {
    assert(restored.joint[i].zero==900 && restored.joint[i].checked==1);
    assert(restored.joint[i].minimum==600 && restored.joint[i].maximum==1200);
  }
  for(int i=0;i<SavedSize;++i) {
    for(int bit=0;bit<8;++bit) {
      packet[i]^=uint8_t(1<<bit);
      assert(!decodeSettings(packet,restored));
      packet[i]^=uint8_t(1<<bit);
    }
  }
  assert(!leg.applySettings(restored)); // Loading cannot alter armed outputs.
  leg.release(); assert(leg.applySettings(restored)); assert(!leg.anyAttached());
  Settings bad=restored; bad.joint[2].maximum=1800;
  assert(!encodeSettings(bad,packet));
  puts("saved calibration round trip, all 256 single-bit corruptions and detached-only load passed");
}
int main() { calibrationAndBounds(); kinematics(); posesAndCycles(); stopTimingAndRelease(); storage(); }

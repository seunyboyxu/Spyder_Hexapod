#include "LegTest.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
namespace legtest {
static void put16(uint8_t *b, uint16_t n) { b[0] = uint8_t(n); b[1] = uint8_t(n >> 8); }
static uint16_t get16(const uint8_t *b) { return uint16_t(b[0]) | uint16_t(b[1]) << 8; }
static uint16_t checksum(const uint8_t *b, uint8_t length) {
  uint16_t crc = 0xffff;
  while (length--) {
    crc ^= uint16_t(*b++) << 8;
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = crc & 0x8000 ? uint16_t((crc << 1) ^ 0x1021) : uint16_t(crc << 1);
  }
  return crc;
}
bool encodeSettings(const Settings &s, uint8_t (&b)[SavedSize]) {
  if (!validSettings(s)) return false;
  memset(b,0,SavedSize);
  b[0]='L'; b[1]='G'; b[2]='T'; b[3]=1;
  for (uint8_t i=0;i<JointCount;++i) {
    uint8_t *p=b+4+8*i; const Calibration &c=s.joint[i];
    put16(p,c.zero); put16(p+2,c.minimum); put16(p+4,c.maximum);
    p[6]=uint8_t(c.direction); p[7]=c.checked;
  }
  put16(b+30,checksum(b,30));
  return true;
}
bool decodeSettings(const uint8_t (&b)[SavedSize], Settings &s) {
  if (b[0]!='L' || b[1]!='G' || b[2]!='T' || b[3]!=1 || b[28] || b[29] ||
      get16(b+30)!=checksum(b,30)) return false;
  Settings candidate;
  for (uint8_t i=0;i<JointCount;++i) {
    const uint8_t *p=b+4+8*i;
    candidate.joint[i]=Calibration{int16_t(get16(p)),int16_t(get16(p+2)),
      int16_t(get16(p+4)),int8_t(p[6]),p[7]};
  }
  if (!validSettings(candidate)) return false;
  s=candidate;
  return true;
}
static const float Rad = 3.14159265358979323846f / 180.0f;
static const Point Stand = {85, 0, -60};
// Partial extension only: retain a knee bend, away from full extension.
static const Point Straight = {125, 0, -30};
static const Point Step[] = {{85, 0, -60}, {85, -10, -60}, {85, -10, -48},
                             {85, 10, -48}, {85, 10, -60}, {85, -10, -60},
                             {85, 0, -60}};
static const uint8_t LastPhase = sizeof(Step) / sizeof(Step[0]) - 1;

uint16_t pulseFor(int16_t angle) {
  // Arduino Servo's DEFAULT 544..2400 mapping, rounded to nearest us.
  // Independently restrict all outputs to the agreed 1000..2000 us envelope.
  return uint16_t(544 + (int32_t(angle) * 1856 + 900) / 1800);
}
bool validSettings(const Settings &s) {
  for (uint8_t i = 0; i < JointCount; ++i) {
    const Calibration &c = s.joint[i];
    if (c.minimum < 450 || c.maximum > 1400 || c.minimum >= c.maximum ||
        c.zero < c.minimum || c.zero > c.maximum ||
        (c.direction != 1 && c.direction != -1) || c.checked > 1 ||
        pulseFor(c.minimum) < 1000 || pulseFor(c.maximum) > 2000)
      return false;
  }
  return true;
}
bool solve(const Point &p, float (&a)[JointCount]) {
  if (!isfinite(p.x) || !isfinite(p.y) || !isfinite(p.z) || p.x <= 0)
    return false;
  const float r = sqrtf(p.x * p.x + p.y * p.y) - HipLength;
  const float d = sqrtf(r * r + p.z * p.z);
  if (!isfinite(d) || d < fabsf(ThighLength - ShinLength) + 1 ||
      d > ThighLength + ShinLength - 1)
    return false;
  const float c1 = (ThighLength * ThighLength + d * d - ShinLength * ShinLength) /
                   (2 * ThighLength * d);
  const float c2 = (ThighLength * ThighLength + ShinLength * ShinLength - d * d) /
                   (2 * ThighLength * ShinLength);
  if (c1 < -1 || c1 > 1 || c2 < -1 || c2 > 1)
    return false;
  a[0] = atan2f(p.y, p.x) / Rad;
  a[1] = (atan2f(p.z, r) + acosf(c1)) / Rad;
  a[2] = 90 - acosf(c2) / Rad;
  return isfinite(a[0]) && isfinite(a[1]) && isfinite(a[2]);
}

LegTest::LegTest(Outputs &o)
    : out_(&o), lastTick_(0), remainder_(0), speed_(10), cyclesLeft_(0), phase_(0) {
  for (uint8_t i = 0; i < JointCount; ++i) {
    settings_.joint[i] = Calibration{900, 600, 1200, 1, 0};
    attached_[i] = false;
    position_[i] = target_[i] = 900;
  }
}
void LegTest::begin(uint32_t now) { release(); lastTick_ = now; }
void LegTest::stop() {
  cyclesLeft_ = 0;
  remainder_ = 0;
  for (uint8_t i = 0; i < JointCount; ++i) target_[i] = position_[i];
}
void LegTest::release() {
  stop();
  for (uint8_t i = 0; i < JointCount; ++i) {
    if (attached_[i]) out_->detach(i);
    attached_[i] = false;
  }
}
bool LegTest::anyAttached() const {
  for (uint8_t i = 0; i < JointCount; ++i) if (attached_[i]) return true;
  return false;
}
bool LegTest::busy() const {
  if (cyclesLeft_) return true;
  for (uint8_t i = 0; i < JointCount; ++i)
    if (attached_[i] && position_[i] != target_[i]) return true;
  return false;
}
bool LegTest::applySettings(const Settings &s) {
  if (anyAttached() || !validSettings(s)) return false;
  settings_ = s;
  return true;
}
bool LegTest::ready() const {
  if (busy()) return false;
  for (uint8_t i = 0; i < JointCount; ++i)
    if (!attached_[i] || !settings_.joint[i].checked) return false;
  return true;
}
bool LegTest::targetsFor(const Point &p, int16_t (&target)[JointCount]) const {
  float angle[JointCount];
  if (!solve(p, angle)) return false;
  for (uint8_t i = 0; i < JointCount; ++i) {
    const Calibration &c = settings_.joint[i];
    const float raw = c.zero + c.direction * angle[i] * 10;
    if (!isfinite(raw) || raw < c.minimum || raw > c.maximum) return false;
    target[i] = int16_t(lroundf(raw));
  }
  return true;
}
void LegTest::setTargets(const int16_t (&target)[JointCount]) {
  for (uint8_t i = 0; i < JointCount; ++i) target_[i] = target[i];
}
bool LegTest::startPose(const Point &p) {
  int16_t target[JointCount];
  if (!ready() || !targetsFor(p, target)) return false;
  setTargets(target);
  return true;
}
bool LegTest::startSteps(uint8_t cycles) {
  if (!ready() || cycles < 1 || cycles > 3) return false;
  int16_t target[JointCount];
  // Reject the entire sequence before any target changes.
  for (uint8_t i = 0; i <= LastPhase; ++i)
    if (!targetsFor(Step[i], target)) return false;
  cyclesLeft_ = cycles;
  phase_ = 0;
  targetsFor(Step[0], target);
  setTargets(target);
  return true;
}
void LegTest::tick(uint32_t now,bool advance) {
  if (uint32_t(now - lastTick_) < 20) return;
  lastTick_ = now; // No catch-up jumps after a delayed loop.
  remainder_ += speed_ * 200; // tenths of a degree per second * 20 ms.
  const int16_t step = remainder_ / 1000;
  remainder_ %= 1000;
  bool moving = false;
  for (uint8_t i = 0; i < JointCount; ++i) {
    if (!attached_[i]) continue;
    const int16_t distance = abs(target_[i] - position_[i]);
    const int16_t delta = distance < step ? distance : step;
    if (delta) {
      position_[i] += target_[i] > position_[i] ? delta : -delta;
      out_->write(i, pulseFor(position_[i]));
    }
    moving = moving || position_[i] != target_[i];
  }
  if (!moving && cyclesLeft_ && advance) advanceStep();
}
bool LegTest::atTarget() const {
  for(uint8_t i=0;i<JointCount;++i)
    if(attached_[i] && position_[i]!=target_[i]) return false;
  return true;
}
bool LegTest::canStart(Motion motion) const {
  if(!ready()) return false;
  int16_t target[JointCount];
  if(motion==Motion::Stand) return targetsFor(Stand,target);
  if(motion==Motion::Straight) return targetsFor(Straight,target);
  if(motion!=Motion::Step) return false;
  for(uint8_t i=0;i<=LastPhase;++i) if(!targetsFor(Step[i],target)) return false;
  return true;
}
bool LegTest::canTrack(const Point &p) const {
  int16_t target[JointCount];
  return ready() && targetsFor(p, target);
}
bool LegTest::track(const Point &p, int16_t maxStep, bool &arrived) {
  int16_t target[JointCount];
  if (!ready() || !targetsFor(p, target)) return false;
  arrived = true;
  for (uint8_t i = 0; i < JointCount; ++i) {
    int16_t delta = target[i] - position_[i];
    if (delta > maxStep) delta = maxStep;
    if (delta < -maxStep) delta = -maxStep;
    if (delta) {
      position_[i] += delta;
      out_->write(i, pulseFor(position_[i]));
    }
    target_[i] = position_[i]; // Stay idle between walking updates.
    arrived = arrived && position_[i] == target[i];
  }
  return true;
}
void LegTest::advanceStep() {
  if(!cyclesLeft_ || !atTarget()) return;
  if(phase_==LastPhase) {
    if(--cyclesLeft_==0) return;
    phase_=1;
  } else ++phase_;
  int16_t target[JointCount];
  if(!targetsFor(Step[phase_],target)) { stop(); return; }
  setTargets(target);
}

static bool integer(const char *s, int &value, int lo, int hi) {
  char *end;
  errno = 0;
  const long n = strtol(s, &end, 10);
  if (!*s || *end || errno || n < lo || n > hi) return false;
  value = int(n);
  return true;
}
static bool degrees(const char *s, int16_t &value) {
  char *end;
  errno = 0;
  const double n = strtod(s, &end);
  if (!*s || *end || errno || !isfinite(n) || n < -180 || n > 180) return false;
  value = int16_t(lround(n * 10));
  return true;
}
const char *LegTest::validateCommand(const char *line) const {
  if (strlen(line)>=64) return "ERR command too long";
  class SilentOutputs : public Outputs {
    void attach(uint8_t,uint16_t) override {}
    void write(uint8_t,uint16_t) override {}
    void detach(uint8_t) override {}
  } silent;
  // A complete value copy preserves positions, targets, flags and timing.
  // Redirect its only external side effect before invoking the real parser.
  LegTest candidate(*this); candidate.out_=&silent;
  char copy[64]; strcpy(copy,line);
  return candidate.command(copy);
}
const char *LegTest::command(char *line) {
  char *args[5]; uint8_t count = 0;
  for (char *p = strtok(line, " \t"); p; p = strtok(nullptr, " \t")) {
    if (count == 5) return "ERR too many arguments";
    args[count++] = p;
  }
  if (!count) return "ERR empty command";
  const char *name = args[0];
  if (count == 1 && !strcmp(name,"stop")) { stop(); return "OK"; }
  if (count == 1 && !strcmp(name,"off")) { release(); return "OK"; }
  if (count == 1 && (!strcmp(name,"stand") || !strcmp(name,"straight")))
    return startPose(!strcmp(name,"stand") ? Stand : Straight) ? "OK" :
      "ERR check/attach all joints, wait until still, and check pose limits";
  if (!strcmp(name,"step") && (count == 1 || count == 2)) {
    int cycles = 1;
    if (count == 2 && !integer(args[1],cycles,1,3)) return "ERR cycles must be 1..3";
    return startSteps(uint8_t(cycles)) ? "OK" : "ERR check/attach all joints; pose outside limits or busy";
  }
  if (!strcmp(name,"speed") && count == 2) {
    int speed;
    if (busy() || !integer(args[1],speed,1,20)) return "ERR wait until still; speed 1..20 deg/s";
    speed_ = uint8_t(speed); remainder_ = 0; return "OK";
  }
  int index;
  if (count < 2 || !integer(args[1],index,0,2)) return "ERR joint must be 0..2";
  const uint8_t i = uint8_t(index);
  Calibration &c = settings_.joint[i];
  if (stepping()) return "ERR stop the sequence first";
  if (!strcmp(name,"cal") && count == 2) {
    target_[i] = c.zero;
    if (!attached_[i]) {
      position_[i] = c.zero;
      out_->attach(i,pulseFor(position_[i])); attached_[i] = true;
    }
    return "OK";
  }
  if ((!strcmp(name,"move") || !strcmp(name,"test")) && count == 3) {
    int16_t angle;
    if (!attached_[i] || !degrees(args[2],angle)) return "ERR attach joint and supply finite angle";
    if (!strcmp(name,"test")) angle = c.zero + c.direction * angle;
    if (angle < c.minimum || angle > c.maximum) return "ERR outside joint limits";
    target_[i] = angle; return "OK";
  }
  if (!strcmp(name,"limits") && count == 4) {
    int16_t lo,hi;
    if (anyAttached() || !degrees(args[2],lo) || !degrees(args[3],hi))
      return "ERR release all outputs first; supply min and max degrees";
    Settings s = settings_;
    s.joint[i].minimum = lo; s.joint[i].maximum = hi; s.joint[i].checked = 0;
    return applySettings(s) ? "OK" : "ERR invalid limits (45..140, containing zero)";
  }
  if ((!strcmp(name,"zero") || !strcmp(name,"flip") || !strcmp(name,"check")) && count == 2) {
    if (!attached_[i] || position_[i] != target_[i]) return "ERR attach joint and wait until still";
    if (!strcmp(name,"zero")) { c.zero = position_[i]; c.checked = 0; }
    if (!strcmp(name,"flip")) { c.direction = -c.direction; c.checked = 0; }
    if (!strcmp(name,"check")) {
      if (position_[i] != c.zero) return "ERR return to test joint 0 (reference pose) first";
      c.checked = 1;
    }
    return "OK";
  }
  return "ERR unknown command or wrong argument count";
}
} // namespace legtest

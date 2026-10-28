#include "ServoBench.h"
#include <string.h>

namespace servobench {
uint16_t pulseTicks(uint16_t us) {
  if (us < 1000 || us > 2000) return 0;
  // Tick = 122/25 us = 4.88 us. Nearest tick, clamped inward at endpoints.
  uint16_t ticks = (uint32_t(us) * 25 + 61) / 122;
  if (ticks < 205) ticks = 205; // 1000.4 us
  if (ticks > 409) ticks = 409; // 1995.92 us
  return ticks;
}
Bench::Bench(Output &out) : output_(out), armed_(false), fault_(false),
  mode_(Mode::Idle), phase_(0), pulse_(1500), target_(1500), lastTick_(0), testStarted_(0) {}
void Bench::hold() { target_ = pulse_; mode_ = Mode::Idle; }
void Bench::release() { output_.disable(); armed_ = false; hold(); }
void Bench::fault() { release(); fault_ = true; }
bool Bench::apply(uint16_t pulse) {
  if (pulse < 1000 || pulse > 2000 || !output_.write(pulse)) { fault(); return false; }
  pulse_ = pulse;
  return true;
}
static bool integer(const char *s, uint16_t &value) {
  if (!*s) return false;
  uint32_t n=0;
  for (; *s; ++s) {
    if (*s<'0' || *s>'9') return false;
    n=n*10+uint8_t(*s-'0');
    if(n>2000) return false;
  }
  value=uint16_t(n); return true;
}
Result Bench::command(const char *line, uint32_t now) {
  if(strlen(line)>=64) return Result::Invalid;
  char copy[64]; strcpy(copy,line);
  char *context=nullptr;
  char *verb=strtok_r(copy," \t",&context);
  char *arg=strtok_r(nullptr," \t",&context);
  if(!verb || strtok_r(nullptr," \t",&context)) return Result::Invalid;
  uint16_t value=0;
  const bool angle=!strcmp(verb,"angle"), pulse=!strcmp(verb,"pulse");
  if(angle || pulse) {
    if(!arg || !integer(arg,value) || (angle ? value>180 : value<1000)) return Result::Invalid;
    if(angle) value=1000+(uint32_t(value)*1000+90)/180;
  } else if(arg) return Result::Invalid;
  if(!strcmp(verb,"help")) return Result::Help;
  if(!strcmp(verb,"show")) return Result::Show;
  if(!strcmp(verb,"off")) { release(); return Result::Released; }
  if(!strcmp(verb,"stop")) { hold(); return Result::Held; }
  const bool arm=!strcmp(verb,"arm"), center=!strcmp(verb,"center"), sweep=!strcmp(verb,"sweep");
  const bool test=verb[0]>='1' && verb[0]<='3' && !verb[1];
  if(!arm && !center && !sweep && !test && !angle && !pulse) return Result::Invalid;
  if(fault_) return Result::Fault;
  if(arm) {
    if(armed_) return Result::Busy;
    if(!output_.enable(1500)) {
      if(output_.interrupted()) { release(); return Result::Released; }
      fault(); return Result::Fault;
    }
    armed_=true; pulse_=target_=1500; mode_=Mode::Idle; lastTick_=now;
    return Result::Ok;
  }
  if(!armed_) return Result::Unarmed;
  if(busy()) return Result::Busy;
  lastTick_=now;
  if(test) {
    value=uint16_t(1300+(verb[0]-'1')*200);
    if(!apply(value)) return Result::Fault;
    target_=pulse_; mode_=Mode::Test; testStarted_=now;
  } else if(sweep) {
    mode_=Mode::Sweep;
    phase_=pulse_==1500 ? 1 : 0;
    target_=phase_==1 ? 1000 : 1500;
  } else target_=center ? 1500 : value;
  return Result::Ok;
}
void Bench::tick(uint32_t now) {
  if(!armed_ || fault_) return;
  if(mode_==Mode::Test) {
    if(uint32_t(now-testStarted_)>=2000) release();
    return;
  }
  if(uint32_t(now-lastTick_)<20) return;
  lastTick_=now; // One step only, even after a delayed loop.
  if(pulse_!=target_ && !apply(pulse_+(target_>pulse_ ? 1 : -1))) return;
  if(mode_==Mode::Sweep && pulse_==target_) {
    if(phase_==3) { mode_=Mode::Idle; return; }
    ++phase_;
    target_=phase_==1 ? 1000 : phase_==2 ? 2000 : 1500;
  }
}
Inputs::Inputs(Bench &b) : lines_{}, bench_(b) {}
void Inputs::clear() {
  for(auto &line:lines_) { line.discard=line.discard || line.used!=0; line.used=0; }
}
void Inputs::resetSource(uint8_t source) { if(source<2) lines_[source]=Line{}; }
void Inputs::disconnect() { bench_.release(); clear(); }
void Inputs::cancel(uint8_t source, bool release, bool discardCurrent) {
  clear();
  if(discardCurrent) lines_[source].discard=true;
  if(release) bench_.release(); else bench_.hold();
}
Result Inputs::feed(uint8_t source, char byte, uint32_t now) {
  if(source>=2) return Result::None;
  Line &line=lines_[source];
  const uint8_t c=uint8_t(byte);
  if(c=='!') { cancel(source,true,true); return Result::Released; }
  if(c=='\n' || c=='\r') {
    if(line.discard) { line.discard=false; line.used=0; return Result::None; }
    if(!line.used) return Result::None;
    line.text[line.used]=0; line.used=0;
    Result result=bench_.command(line.text,now);
    if(result==Result::Held || result==Result::Released) cancel(source,result==Result::Released,false);
    return result;
  }
  if(line.discard) return Result::None;
  if((c<32 && c!='\t') || c>126 || line.used>=sizeof(line.text)-1) {
    cancel(source,false,true); return Result::InputError;
  }
  line.text[line.used++]=byte; line.last=now;
  return Result::None;
}
Result Inputs::expire(uint32_t now) {
  for(uint8_t i=0;i<2;++i) {
    if(lines_[i].used && uint32_t(now-lines_[i].last)>2000) {
      cancel(i,false,true); return Result::InputError;
    }
  }
  return Result::None;
}
} // namespace servobench

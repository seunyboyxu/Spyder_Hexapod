#include "ServoBench.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
using namespace servobench;
struct Motor : Output {
 bool enabled=false, fail=false, cancelled=false; unsigned calls=0; std::vector<int> pulses;
 bool enable(uint16_t us) override { ++calls; if(fail || cancelled) return false; enabled=true; pulses.push_back(us); return true; }
 bool write(uint16_t us) override { ++calls; if(fail) return false; pulses.push_back(us); return true; }
 bool interrupted() const override { return cancelled; }
 void disable() override { enabled=false; }
};
static void tickFor(Bench &b,uint32_t &now,uint32_t duration) { for(uint32_t i=0;i<duration;i+=20) b.tick(now+=20); }
static Result send(Inputs &in,int source,const char *s,uint32_t now=0) {
 Result last=Result::None;
 for(;*s;++s) { auto r=in.feed(source,*s,now); if(r!=Result::None) last=r; }
 return last;
}
static void bootAndManual() {
 Motor m; Bench b(m); uint32_t now=0;
 assert(!b.armed() && !m.enabled);
 assert(b.command("center",now)==Result::Unarmed);
 assert(b.command("arm",now)==Result::Ok && m.enabled && b.pulse()==1500);
 assert(b.command("arm",now)==Result::Busy);
 assert(b.command("angle 0",now)==Result::Ok);
 tickFor(b,now,10000); assert(b.pulse()==1000);
 assert(b.command("angle 180",now)==Result::Ok);
 tickFor(b,now,20000); assert(b.pulse()==2000);
 assert(b.command("angle 90",now)==Result::Ok);
 tickFor(b,now,10000); assert(b.pulse()==1500);
 for(auto s:{"angle -1","angle 181","angle 9.0","angle 9 extra","pulse 999","pulse 2001","pulse 9999999999999999999999","arm extra","bogus"}) {
  assert(b.command(s,now)==Result::Invalid); assert(b.pulse()==1500 && b.target()==1500);
 }
 assert(b.command("pulse 1505",now)==Result::Ok);
 b.tick(now+=1000); assert(b.pulse()==1501); // Late loop advances one 20ms step only.
 b.hold(); tickFor(b,now,500); assert(b.pulse()==1501 && m.enabled);
 b.release(); assert(!b.armed() && !m.enabled);
 assert(b.command("pulse 1500",now)==Result::Unarmed);
 puts("PASS released boot, strict commands, degree mapping, bounded timing and stop");
}
static void testsAndSweep() {
 Motor m; Bench b(m); uint32_t now=0;
 b.command("arm",now);
 assert(b.command("1",now)==Result::Ok && b.pulse()==1300);
 assert(b.command("center",now)==Result::Busy);
 b.tick(now=1999); assert(b.armed()); b.tick(now=2000); assert(!b.armed());
 b.command("arm",now); b.command("3",now); assert(b.pulse()==1700);
 b.hold(); b.tick(now+=3000); assert(b.armed() && b.pulse()==1700);
 b.command("center",now); tickFor(b,now,4000); assert(b.pulse()==1500);
 assert(b.command("sweep",now)==Result::Ok);
 assert(b.command("pulse 1550",now)==Result::Busy);
 tickFor(b,now,40000); assert(!b.busy() && b.armed() && b.pulse()==1500);
 bool low=false,high=false;
 for(int p:m.pulses) { assert(p>=1000 && p<=2000); low|=p==1000; high|=p==2000; }
 assert(low && high);
 auto n=m.pulses.size(); tickFor(b,now,2000); assert(m.pulses.size()==n);
 b.command("sweep",now); tickFor(b,now,2000); b.hold(); int p=b.pulse();
 tickFor(b,now,20000); assert(b.pulse()==p && !b.busy());
 puts("PASS timed type tests, finite sweep, busy exclusion and cancellation");
}
static void failureAndWrap() {
 assert(pulseTicks(999)==0 && pulseTicks(2001)==0);
 assert(pulseTicks(1000)==205 && pulseTicks(1500)==307 && pulseTicks(2000)==409);
 for(int us=1000; us<=2000; ++us) { auto t=pulseTicks(us); assert(t*122>=25000 && t*122<=50000); }
 Motor cancelled; Bench cb(cancelled); cancelled.cancelled=true;
 assert(cb.command("arm",0)==Result::Released && !cb.armed() && !cb.faulted());
 cancelled.cancelled=false; assert(cb.command("arm",0)==Result::Ok);
 Motor m; Bench b(m); uint32_t now=0xfffffff0U;
 b.command("arm",now); b.command("pulse 1510",now); b.tick(now+=20); assert(b.pulse()==1501);
 m.fail=true; b.tick(now+=20); assert(b.faulted() && !b.armed() && !m.enabled);
 m.fail=false; assert(b.command("arm",now)==Result::Fault);
 Motor m2; Bench b2(m2); m2.fail=true; b2.command("arm",0); assert(b2.faulted() && !m2.enabled);
 puts("PASS timer wrap and latched output faults");
}
static void inputs() {
 Motor m; Bench b(m); Inputs in(b);
 assert(send(in,0,"arm\r\n")==Result::Ok);
 assert(send(in,0,"pulse 16")==Result::None);
 assert(send(in,1,"show\n")==Result::Show); // Sources never concatenate.
 assert(send(in,0,"00\n")==Result::Ok && b.target()==1600);
 b.tick(20); assert(b.pulse()==1501);
 assert(send(in,1,"!pulse 2000\n")==Result::Released && !b.armed());
 assert(send(in,0,"arm\n",40)==Result::Ok);
 assert(send(in,0,"pulse 1700\n",40)==Result::Ok);
 send(in,1,"pulse 1",40); assert(send(in,0,"stop\n",40)==Result::Held);
 assert(send(in,1,"900\n",40)==Result::None); // Interrupted partial tail is discarded.
 assert(b.target()==b.pulse());
 send(in,0,"pulse 1800\n",40); send(in,1,"bad",40);
 assert(in.expire(2041)==Result::InputError && b.target()==b.pulse());
 send(in,1,"\n",2041);
 auto over=std::string(90,'x')+"\n";
 assert(send(in,0,over.c_str(),2050)==Result::InputError);
 assert(send(in,0,"pulse 1600\n",2050)==Result::Ok);
 const char invalid[]={char(0x80),'\n',0}; assert(send(in,1,invalid,2050)==Result::InputError);
 send(in,0,"pulse 199",2050); in.disconnect(); assert(!b.armed());
 assert(send(in,0,"9\n",2050)==Result::None);
 assert(send(in,0,"arm\n",2050)==Result::Ok);
 send(in,1,"partial",2050); in.disconnect(); in.resetSource(1);
 assert(send(in,1,"show\n",2050)==Result::Show);
 puts("PASS independent inputs, urgent release, stale/overflow rejection and disconnect");
}
int main() { bootAndManual(); testsAndSweep(); failureAndWrap(); inputs(); puts("All ESP32 servo controller groups passed"); }

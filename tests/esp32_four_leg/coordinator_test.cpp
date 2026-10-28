#include "FourLegTest.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace fourleg;
struct Hardware : Channels {
 struct Event { uint8_t channel; uint16_t pulse; };
 std::vector<Event> events; uint16_t active=0;
 void attach(uint8_t channel,uint16_t pulse) override { active|=1U<<channel; write(channel,pulse); }
 void write(uint8_t channel,uint16_t pulse) override { assert(channel<12 && pulse>=1000 && pulse<=2000); events.push_back({channel,pulse}); }
 void detach(uint8_t channel) override { active&=uint16_t(~(1U<<channel)); }
};
static bool send(FourLegTest &rig,const char *text,uint32_t now) {
 char line[64]; assert(strlen(text)<sizeof(line)); strcpy(line,text);
 return !strcmp(rig.command(line,now),"OK");
}
static void settle(FourLegTest &rig,uint32_t &now) {
 for(int i=0;i<200000 && rig.busy();++i) rig.tick(now+=20);
 assert(!rig.busy());
}
static void prepare(FourLegTest &rig,uint32_t now) {
 for(int l=1;l<=4;++l) {
  char text[32]; snprintf(text,sizeof(text),"leg %d",l); assert(send(rig,text,now));
  for(int j=0;j<3;++j) {
   snprintf(text,sizeof(text),"cal %d",j); assert(send(rig,text,now));
   snprintf(text,sizeof(text),"check %d",j); assert(send(rig,text,now));
  }
 }
}
int main() {
 Hardware hw; FourLegTest rig(hw); uint32_t now=0; rig.begin(now);
 assert(!rig.anyAttached() && rig.selected()==0);
 assert(!send(rig,"leg 0",now) && !send(rig,"leg 5",now));
 assert(send(rig,"leg all",now)); assert(send(rig,"cal 0",now)); rig.release();
 assert(!send(rig,"armall",now)); assert(!send(rig,"stand",now));
 prepare(rig,now); assert(hw.active==0x0fff);
 assert(send(rig,"leg all",now));
 assert(send(rig,"stand",now)); settle(rig,now);
 int16_t standKnee=rig.leg(0).position(1);
 assert(send(rig,"straight",now)); settle(rig,now);
 assert(send(rig,"stand",now)); settle(rig,now);
 assert(send(rig,"leg 4",now)); assert(send(rig,"speed 1",now));
 assert(send(rig,"leg all",now)); assert(send(rig,"step 1",now));
 for(int n=0;n<60;++n) rig.tick(now+=20);
 assert(rig.leg(0).position(0)==833); // atan(-10/85) about -6.7deg.
 assert(rig.leg(3).position(0)>833); // Slower leg has not reached the same waypoint.
 assert(rig.leg(0).position(1)==standKnee); // Fast leg must not lift early.
 assert(!send(rig,"leg 2",now) && !send(rig,"speed 20",now));
 settle(rig,now); assert(!rig.leg(0).stepping() && !rig.leg(3).stepping());
 assert(send(rig,"speed 20",now)); assert(send(rig,"step 3",now)); rig.tick(now+=20);
 rig.stop(); assert(!rig.busy() && hw.active==0x0fff);
 auto count=hw.events.size(); rig.tick(now+=10000); assert(hw.events.size()==count);
 rig.release(); assert(!rig.anyAttached() && hw.active==0);
 assert(send(rig,"armall",now)); assert(rig.arming());
 rig.tick(now+=99); assert(hw.active==0); rig.tick(++now); assert(hw.active==1);
 rig.tick(now+=100); assert(hw.active==3); rig.stop();
 rig.tick(now+=10000); assert(hw.active==3 && !rig.arming());
 rig.release(); assert(send(rig,"armall",now)); settle(rig,now); assert(hw.active==0x0fff);
 rig.release();
 auto settings=rig.settings(); settings.leg[3].joint[0].minimum=890; settings.leg[3].joint[0].maximum=910;
 assert(rig.applySettings(settings)); prepare(rig,now); assert(send(rig,"leg all",now));
 count=hw.events.size(); assert(!send(rig,"step 1",now));
 assert(!rig.busy() && hw.events.size()==count); // Atomic rejection for the fourth leg.
 rig.release();
 uint8_t packet[SavedSize]; assert(encodeSettings(rig.settings(),packet));
 Settings restored; assert(decodeSettings(packet,restored));
 for(unsigned byte=0;byte<SavedSize;++byte) for(int bit=0;bit<8;++bit) {
  packet[byte]^=uint8_t(1<<bit); assert(!decodeSettings(packet,restored)); packet[byte]^=uint8_t(1<<bit);
 }
 assert(rig.applySettings(restored) && !rig.anyAttached());
 puts("PASS four-leg mapping, calibrated group preflight, shared phase barriers, finite cycles, staged arming, global safety and storage");
}

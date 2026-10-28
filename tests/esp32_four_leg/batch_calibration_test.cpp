#include "FourLegTest.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace fourleg;
struct Hardware : Channels {
 uint16_t active=0; std::vector<uint8_t> writes;
 void attach(uint8_t ch,uint16_t pulse) override { active|=1U<<ch; write(ch,pulse); }
 void write(uint8_t ch,uint16_t pulse) override { assert(ch<12 && pulse>=1000 && pulse<=2000); writes.push_back(ch); }
 void detach(uint8_t ch) override { active&=uint16_t(~(1U<<ch)); }
};
static bool send(FourLegTest &rig,const char *s,uint32_t now) {
 char line[64]; strcpy(line,s); return !strcmp(rig.command(line,now),"OK");
}
static void settle(FourLegTest &rig,uint32_t &now) {
 for(int i=0;i<200000 && rig.busy();++i) rig.tick(now+=20);
 assert(!rig.busy());
}
static void allChecked(const FourLegTest &rig,bool expected) {
 for(unsigned l=0;l<4;++l) for(unsigned j=0;j<3;++j) assert(bool(rig.leg(l).settings().joint[j].checked)==expected);
}
int main() {
 Hardware hw; FourLegTest rig(hw); uint32_t now=0; rig.begin(now);
 assert(send(rig,"leg all",now));
 // Missing bulk calibration must fail here on the old controller.
 assert(send(rig,"cal 0",now)); assert(hw.active==0x0249); assert(hw.writes.size()==4);
 assert(!send(rig,"check all",now)); allChecked(rig,false); // No partial checks if knees/feet are detached.
 assert(send(rig,"check 0",now));
 for(unsigned l=0;l<4;++l) assert(rig.leg(l).settings().joint[0].checked);
 assert(send(rig,"move 0 88",now)); assert(rig.coordinated());
 assert(!send(rig,"zero 0",now) && !send(rig,"leg 2",now));
 settle(rig,now); assert(send(rig,"zero 0",now));
 for(unsigned l=0;l<4;++l) assert(rig.leg(l).settings().joint[0].zero==880 && !rig.leg(l).settings().joint[0].checked);
 assert(send(rig,"flip 0",now)); assert(send(rig,"test 0 5",now)); settle(rig,now);
 for(unsigned l=0;l<4;++l) assert(rig.leg(l).position(0)==830);
 assert(!send(rig,"check 0",now));
 assert(send(rig,"test 0 0",now)); settle(rig,now); assert(send(rig,"check 0",now));
 rig.release(); assert(send(rig,"limits 0 65 115",now));
 for(unsigned l=0;l<4;++l) assert(rig.leg(l).settings().joint[0].minimum==650 && !rig.leg(l).settings().joint[0].checked);
 auto settings=rig.settings(); settings.leg[3].joint[0].maximum=900;
 settings.leg[3].joint[0].zero=890; settings.leg[3].joint[0].direction=1;
 assert(rig.applySettings(settings)); assert(send(rig,"cal 0",now));
 auto count=hw.writes.size();
 assert(!send(rig,"move 0 91",now)); assert(!rig.busy() && hw.writes.size()==count);
 for(unsigned l=0;l<3;++l) assert(rig.leg(l).position(0)==880);
 assert(rig.leg(3).position(0)==890); // Retain separate zeros.
 assert(!send(rig,"test 0 5",now)); assert(!rig.busy() && hw.writes.size()==count);
 assert(send(rig,"test 0 1",now)); settle(rig,now);
 for(unsigned l=0;l<3;++l) assert(rig.leg(l).position(0)==870);
 assert(rig.leg(3).position(0)==900); // Retain separate direction signs.
 rig.release();
 // Late validation failure must not alter earlier legs' limits or checked flags.
 settings=rig.settings(); settings.leg[3].joint[1].zero=1150; assert(rig.applySettings(settings));
 assert(!send(rig,"limits 1 60 110",now));
 for(unsigned l=0;l<4;++l) assert(rig.leg(l).settings().joint[1].maximum==1200);
 for(const char *bad:{"cal 3","check all extra","cal all extra","move 0 nan","flip 0 extra","limits 0 100 60"})
  assert(!send(rig,bad,now));
 // Unchecked staged arming is cancellable before and after first attachment.
 assert(!send(rig,"armall",now)); assert(send(rig,"cal all",now));
 assert(rig.arming() && hw.active==0);
 assert(!send(rig,"check all",now)); rig.stop(); rig.tick(now+=1000); assert(hw.active==0);
 assert(send(rig,"cal all",now)); rig.tick(now+=99); assert(hw.active==0);
 rig.tick(++now); assert(hw.active==1); rig.stop(); rig.tick(now+=1000); assert(hw.active==1);
 assert(!send(rig,"cal all",now)); rig.release(); assert(send(rig,"cal all",now));
 rig.tick(now+=100); rig.release(); rig.tick(now+=1000); assert(hw.active==0 && !rig.busy());
 assert(send(rig,"cal all",now)); settle(rig,now); assert(hw.active==0x0fff);
 assert(send(rig,"check all",now)); allChecked(rig,true);
 assert(send(rig,"test 2 5",now)); settle(rig,now);
 // At reference checks are atomic even if only the last leg is away.
 assert(send(rig,"test 2 0",now)); settle(rig,now);
 assert(send(rig,"zero 0",now)); // Clear earlier recipients so partial confirmation would be observable.
 assert(send(rig,"leg 4",now)); assert(send(rig,"move 2 91",now)); settle(rig,now);
 assert(send(rig,"leg all",now));
 auto before=rig.settings(); assert(!send(rig,"check all",now));
 for(unsigned l=0;l<4;++l) for(unsigned j=0;j<3;++j)
  assert(rig.leg(l).settings().joint[j].checked==before.leg[l].joint[j].checked);
 assert(send(rig,"test 2 0",now)); settle(rig,now); assert(send(rig,"check all",now));
 assert(send(rig,"off",now)); assert(send(rig,"armall",now)); settle(rig,now);
 assert(hw.active==0x0fff); allChecked(rig,true);
 puts("PASS bulk calibration: matching-joint broadcast, independent references/directions, atomic rejection, explicit checks and cancellable unchecked staging");
}

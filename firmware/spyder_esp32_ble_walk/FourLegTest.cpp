#include "FourLegTest.h"
#include "WalkConfig.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
namespace fourleg {
static uint16_t crc(const uint8_t *data,uint16_t length) {
  uint16_t value=0xffff;
  while(length--) {
    value^=uint16_t(*data++)<<8;
    for(uint8_t bit=0;bit<8;++bit) value=value&0x8000?uint16_t((value<<1)^0x1021):uint16_t(value<<1);
  }
  return value;
}
bool encodeSettings(const Settings &settings,uint8_t (&bytes)[SavedSize]) {
  bytes[0]='F'; bytes[1]='L'; bytes[2]='T'; bytes[3]=1;
  for(uint8_t i=0;i<LegCount;++i) {
    uint8_t record[legtest::SavedSize];
    if(!legtest::encodeSettings(settings.leg[i],record)) return false;
    memcpy(bytes+4+i*legtest::SavedSize,record,sizeof(record));
  }
  uint16_t sum=crc(bytes,SavedSize-2);
  bytes[SavedSize-2]=uint8_t(sum); bytes[SavedSize-1]=uint8_t(sum>>8); return true;
}
bool decodeSettings(const uint8_t (&bytes)[SavedSize],Settings &settings) {
  if(bytes[0]!='F' || bytes[1]!='L' || bytes[2]!='T' || bytes[3]!=1 ||
     crc(bytes,SavedSize-2)!=(uint16_t(bytes[SavedSize-2])|(uint16_t(bytes[SavedSize-1])<<8))) return false;
  Settings candidate;
  for(uint8_t i=0;i<LegCount;++i) {
    uint8_t record[legtest::SavedSize]; memcpy(record,bytes+4+i*legtest::SavedSize,sizeof(record));
    if(!legtest::decodeSettings(record,candidate.leg[i])) return false;
  }
  settings=candidate; return true;
}
FourLegTest::FourLegTest(Channels &channels)
 : maps_{{channels,0},{channels,3},{channels,6},{channels,9}},
   legs_{legtest::LegTest(maps_[0]),legtest::LegTest(maps_[1]),legtest::LegTest(maps_[2]),legtest::LegTest(maps_[3])},
   selected_(0),armIndex_(0),group_(false),groupStep_(false),arming_(false),lastArm_(0),
   walkMode_(NotWalking),cyclesLeft_(0),stride_(0),turn_(0),lastWalk_(0),walkError_(false) {}
void FourLegTest::begin(uint32_t now) { stop(); for(auto &leg:legs_) leg.begin(now); }
void FourLegTest::stop() {
  group_=false; groupStep_=false; arming_=false; armIndex_=0;
  walkMode_=NotWalking; gait_=walk::Gait(); cyclesLeft_=0;
  for(auto &leg:legs_) leg.stop();
}
void FourLegTest::release() { stop(); for(auto &leg:legs_) leg.release(); }
bool FourLegTest::anyAttached() const { for(const auto &leg:legs_) if(leg.anyAttached()) return true; return false; }
bool FourLegTest::busy() const { if(group_ || arming_ || walkMode_) return true; for(const auto &leg:legs_) if(leg.busy()) return true; return false; }
Settings FourLegTest::settings() const { Settings result; for(uint8_t i=0;i<LegCount;++i) result.leg[i]=legs_[i].settings(); return result; }
bool FourLegTest::applySettings(const Settings &settings) {
  if(anyAttached() || busy()) return false;
  for(const auto &leg:settings.leg) if(!legtest::validSettings(leg)) return false;
  for(uint8_t i=0;i<LegCount;++i) if(!legs_[i].applySettings(settings.leg[i])) return false;
  return true;
}
static bool number(const char *text,int &value,int low,int high) {
  if(!text || !*text) return false;
  errno=0; char *end=nullptr; long n=strtol(text,&end,10);
  if(errno || *end || n<low || n>high) return false;
  value=int(n); return true;
}
const char *FourLegTest::command(char *line,uint32_t now) {
  if(strlen(line)>=64) return "ERR command too long";
  char copy[64]; strcpy(copy,line); char *args[5],*context=nullptr; uint8_t count=0;
  for(char *p=strtok_r(copy," \t",&context);p;p=strtok_r(nullptr," \t",&context)) {
    if(count==5) return "ERR too many arguments";
    args[count++]=p;
  }
  if(!count) return "ERR empty command";
  if(count==1 && !strcmp(args[0],"stop")) { stop(); return "OK"; }
  if(count==1 && !strcmp(args[0],"off")) { release(); return "OK"; }
  const char *walking=walkCommand(args,count,now);
  if(walking) return walking;
  if(walkMode_) return "ERR walking; send halt, stop or off first";
  if(group_ || arming_) return "ERR group busy; stop or off first";
  if(!strcmp(args[0],"leg")) {
    if(count!=2 || busy()) return "ERR select only when stationary: leg 1..4 or leg all";
    if(!strcmp(args[1],"all")) selected_=All;
    else { int index; if(!number(args[1],index,1,4)) return "ERR leg must be 1..4 or all"; selected_=uint8_t(index-1); }
    return "OK";
  }
  const bool calibrateAll=selected_==All && count==2 && !strcmp(args[0],"cal") && !strcmp(args[1],"all");
  if(!strcmp(args[0],"armall") || calibrateAll) {
    if((!calibrateAll && count!=1) || anyAttached() || busy()) return "ERR all outputs must be off and stationary before staging";
    if(!calibrateAll) for(const auto &leg:legs_) for(const auto &joint:leg.settings().joint)
      if(!joint.checked) return "ERR calibrate/check all12 joints first; use leg all then cal all to calibrate";
    arming_=true; armIndex_=0; lastArm_=now; return "OK";
  }
  if(selected_!=All) return legs_[selected_].command(line);
  const bool checkAll=count==2 && !strcmp(args[0],"check") && !strcmp(args[1],"all");
  const bool calibration=!strcmp(args[0],"cal") || !strcmp(args[0],"move") ||
    !strcmp(args[0],"test") || !strcmp(args[0],"zero") || !strcmp(args[0],"flip") ||
    !strcmp(args[0],"check") || !strcmp(args[0],"limits");
  if(calibration) {
    if(busy()) return "ERR wait until all legs are stationary";
    // Complete preflight before changing any output, target or calibration.
    for(const auto &leg:legs_) {
      if(checkAll) {
        for(uint8_t j=0;j<3;++j) {
          char text[12]; snprintf(text,sizeof(text),"check %u",unsigned(j));
          const char *result=leg.validateCommand(text);
          if(strcmp(result,"OK")) return result;
        }
      } else {
        const char *result=leg.validateCommand(line);
        if(strcmp(result,"OK")) return result;
      }
    }
    for(auto &leg:legs_) {
      if(checkAll) {
        for(uint8_t j=0;j<3;++j) {
          char text[12]; snprintf(text,sizeof(text),"check %u",unsigned(j)); leg.command(text);
        }
      } else { char text[64]; strcpy(text,line); leg.command(text); }
      leg.resetClock(now);
    }
    group_=busy(); groupStep_=false;
    return "OK";
  }
  if(!strcmp(args[0],"speed")) {
    int speed;
    if(count!=2 || busy() || !number(args[1],speed,1,20)) return "ERR stationary only; speed 1..20";
    for(auto &leg:legs_) { char text[24]; snprintf(text,sizeof(text),"speed %d",speed); leg.command(text); }
    return "OK";
  }
  legtest::Motion motion;
  if(!strcmp(args[0],"stand") && count==1) motion=legtest::Motion::Stand;
  else if(!strcmp(args[0],"straight") && count==1) motion=legtest::Motion::Straight;
  else if(!strcmp(args[0],"step") && (count==1 || count==2)) {
    int cycles=1; if(count==2 && !number(args[1],cycles,1,3)) return "ERR cycles must be 1..3";
    motion=legtest::Motion::Step;
  } else return "ERR unknown group command or arguments; send help";
  for(const auto &leg:legs_) if(!leg.canStart(motion)) return "ERR all12 must be checked, attached, still and within path limits";
  for(auto &leg:legs_) {
    char text[64]; strcpy(text,line);
    if(strcmp(leg.command(text),"OK")) { stop(); return "ERR group preflight failed"; }
    leg.resetClock(now);
  }
  group_=true; groupStep_=motion==legtest::Motion::Step; return "OK";
}
// ---- Walking ----
// Check a value copy of the requested lifecycle, including direction changes
// and finite stopping. Runtime still validates every frame before any write.
bool FourLegTest::lifecycleFits(walk::Gait trial,uint16_t cycles,bool stopping) const {
  for(uint8_t i=0;i<LegCount;++i)
    if(!legs_[i].canTrack(walk::toLeg(i,walk::homeFoot(i)))) return false;
  walk::Vec3 feet[LegCount];
  // Three upcoming boundaries suffice for a down-ramp, up-ramp and steady
  // phase after a change at an arbitrary phase. Later steady cycles repeat.
  uint16_t remaining=cycles>3?3:cycles;
  const uint32_t updates=uint32_t(5)*CycleMs/20+2;
  for(uint16_t n=0;n<updates;++n) {
    uint8_t completed=trial.update(20,feet);
    if(!trial.active()) return true; // The live controller next targets home.
    for(uint8_t i=0;i<LegCount;++i)
      if(!legs_[i].canTrack(walk::toLeg(i,feet[i]))) return false;
    if(completed && remaining && !trial.stopping()) {
      remaining=remaining>completed?uint16_t(remaining-completed):0;
      if(!remaining) trial.stop();
    }
  }
  return cycles==0 && !stopping;
}
bool FourLegTest::walkFits(float stride,float turn,uint16_t cycles) const {
  walk::Gait trial=walkMode_==Gait?gait_:walk::Gait();
  trial.walk(stride,turn);
  return lifecycleFits(trial,cycles,false);
}
bool FourLegTest::trackAll(const walk::Vec3 (&feet)[LegCount],bool &arrived) {
  for(uint8_t i=0;i<LegCount;++i)
    if(!legs_[i].canTrack(walk::toLeg(i,feet[i]))) return false;
  arrived=true;
  for(uint8_t i=0;i<LegCount;++i) {
    bool there=false;
    if(!legs_[i].track(walk::toLeg(i,feet[i]),WalkMaxStepTenths,there)) return false;
    arrived=arrived && there;
  }
  return true;
}
const char *FourLegTest::walkCommand(char **args,uint8_t count,uint32_t now) {
  const char *name=args[0];
  if(!strcmp(name,"halt")) {
    if(count!=1 || !walkMode_) return "ERR not walking";
    if(walkMode_==Gait) {
      walk::Gait trial=gait_; trial.stop();
      if(!lifecycleFits(trial,0,true)) return "ERR unsafe halt path; use stop/off; current walk unchanged";
    }
    if(walkMode_==Gait) gait_.stop(); else walkMode_=FromGait;
    return "OK";
  }
  float stride=0,turn=0;
  if(!strcmp(name,"walk")) stride=StrideMm;
  else if(!strcmp(name,"back")) stride=-StrideMm;
  else if(!strcmp(name,"left")) turn=TurnDeg;
  else if(!strcmp(name,"right")) turn=-TurnDeg;
  else return nullptr;
  int cycles=0;
  if(count>2 || (count==2 && !number(args[1],cycles,1,100))) return "ERR cycles must be 1..100 (none = until halt)";
  if(group_ || arming_ || (!walkMode_ && busy())) return "ERR wait until all legs are stationary";
  if(walkMode_==FromGait) return "ERR finishing a walk; wait for Ready";
  if(!walkFits(stride,turn,uint16_t(cycles)))
    return "ERR requested walk/transition outside limits or joints not ready; current motion unchanged";
  stride_=stride; turn_=turn; cyclesLeft_=uint16_t(cycles);
  if(walkMode_==Gait) gait_.walk(stride,turn); // Changes direction smoothly.
  else if(!walkMode_) {
    for(uint8_t i=0;i<LegCount;++i) feet_[i]=walk::homeFoot(i);
    walkMode_=ToStand; lastWalk_=now;
  }
  return "OK";
}
void FourLegTest::tickWalk(uint32_t now) {
  if(uint32_t(now-lastWalk_)<20) return;
  uint32_t dt=now-lastWalk_; if(dt>40) dt=40; // No catch-up jumps.
  lastWalk_=now;
  bool arrived=false;
  if(walkMode_==Gait) {
    const uint8_t finished=gait_.update(uint16_t(dt),feet_);
    if(finished && cyclesLeft_ && !gait_.stopping()) {
      cyclesLeft_=cyclesLeft_>finished?uint16_t(cyclesLeft_-finished):0;
      if(!cyclesLeft_) gait_.stop();
    }
    if(!gait_.active()) {
      walkMode_=FromGait;
      for(uint8_t i=0;i<LegCount;++i) feet_[i]=walk::homeFoot(i);
    }
  }
  if(!trackAll(feet_,arrived)) { stop(); walkError_=true; return; }
  if(!arrived || walkMode_==Gait) return;
  if(walkMode_==ToStand) { walkMode_=Gait; gait_.walk(stride_,turn_); }
  else walkMode_=NotWalking;
}

void FourLegTest::tick(uint32_t now) {
  if(walkMode_) { tickWalk(now); return; }
  if(arming_) {
    if(uint32_t(now-lastArm_)<100) return;
    lastArm_=now;
    char text[12]; snprintf(text,sizeof(text),"cal %u",unsigned(armIndex_%3));
    if(strcmp(legs_[armIndex_/3].command(text),"OK")) { stop(); return; }
    if(++armIndex_==ChannelCount) arming_=false;
    return;
  }
  if(!group_) { for(auto &leg:legs_) leg.tick(now); return; }
  bool reached=true;
  for(auto &leg:legs_) { leg.tick(now,false); reached=reached && leg.atTarget(); }
  if(!reached) return;
  if(groupStep_) for(auto &leg:legs_) leg.advanceStep();
  bool moving=false; for(const auto &leg:legs_) moving=moving || leg.busy();
  if(!moving) group_=false;
}
}

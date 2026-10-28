#include "PcaLegOutput.h"
#include <cassert>
#include <cstdio>
uint32_t fakeNow=0; int oe=-1; std::vector<int> levels; TwoWire Wire;
int main() {
 PcaLegOutput out; assert(out.begin() && oe==HIGH);
 for(int ch=0;ch<16;++ch) assert(Wire.registers[9+4*ch]==0x10);
 out.attach(0,1472); assert(oe==LOW && !out.failed());
 assert(Wire.registers[8]==46 && Wire.registers[9]==1); // 302ticks,1473.76us.
 out.attach(1,1500); out.attach(2,1600);
 assert(out.activeMask()==7);
 for(int ch=3;ch<16;++ch) assert(Wire.registers[9+4*ch]==0x10);
 out.detach(1); assert(out.activeMask()==5 && oe==LOW && Wire.registers[13]==0x10);
 auto hipLow=Wire.registers[8],footLow=Wire.registers[16];
 out.write(2,1700); assert(Wire.registers[8]==hipLow && Wire.registers[16]!=footLow);
 out.disableAll(); assert(oe==HIGH && out.activeMask()==0);
 for(int ch=0;ch<3;++ch) assert(Wire.registers[9+4*ch]==0x10);
 Wire.afterWrite=[&] { out.radioClosed(); }; out.attach(1,1472);
 assert(oe==HIGH && !out.failed() && out.takeRadioRelease());
 out.disableAll(); out.attach(1,1472); assert(oe==LOW && out.activeMask()==2);
 Wire.writeFail=true; out.write(1,1550); assert(out.failed() && oe==HIGH);
 Wire.writeFail=false; out.attach(0,1472); assert(oe==HIGH);
 Wire=TwoWire(); PcaLegOutput invalid; assert(invalid.begin()); invalid.attach(3,1500);
 assert(invalid.failed() && oe==HIGH);
 Wire=TwoWire(); PcaLegOutput missing; Wire.present=false; assert(!missing.begin() && oe==HIGH);
 puts("PASS PCA leg channel isolation, per-joint release, global inhibition, disconnect race and fault latching");
}

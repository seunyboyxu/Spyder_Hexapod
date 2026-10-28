#include "PcaOutputs.h"
#include <cassert>
#include <cstdio>
uint32_t fakeNow=0; int oe=-1; std::vector<int> levels; TwoWire Wire;
int main() {
 PcaOutputs out; assert(out.begin() && oe==HIGH);
 for(int ch=0;ch<16;++ch) assert(Wire.registers[9+4*ch]==16);
 for(int ch=0;ch<12;++ch) out.attach(ch,1472);
 assert(out.activeMask()==0x0fff && oe==LOW);
 for(int ch=0;ch<12;++ch) assert(Wire.registers[8+4*ch]==46 && Wire.registers[9+4*ch]==1);
 for(int ch=12;ch<16;++ch) assert(Wire.registers[9+4*ch]==16);
 out.detach(8); assert(out.activeMask()==0x0eff && Wire.registers[41]==16 && oe==LOW);
 out.write(11,1700); assert(Wire.registers[52]!=46 && Wire.registers[8]==46);
 out.disableAll(); assert(out.activeMask()==0 && oe==HIGH);
 for(int ch=0;ch<16;++ch) assert(Wire.registers[9+4*ch]==16);
 Wire.afterWrite=[&] { out.radioClosed(); }; out.attach(11,1472);
 assert(oe==HIGH && out.takeRadioRelease() && !out.failed()); out.disableAll();
 out.attach(11,1472); assert(out.activeMask()==0x0800 && oe==LOW);
 Wire.writeFail=true; out.write(11,1500); assert(out.failed() && oe==HIGH);
 Wire.writeFail=false; out.attach(0,1472); assert(oe==HIGH);
 Wire=TwoWire(); PcaOutputs invalid; assert(invalid.begin()); invalid.attach(12,1500);
 assert(invalid.failed() && oe==HIGH && Wire.registers[57]==16);
 puts("PASS12-channel mask, mapped registers, per-joint/global release, reserved outputs and fault/disconnect inhibition");
}

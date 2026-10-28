#include "PcaOutput.h"
#include <cassert>
#include <cstdio>
uint32_t fakeNow=0; int oe=-1; std::vector<int> levels; TwoWire Wire;
int main() {
 PcaOutput out; assert(out.begin());
 assert(oe==HIGH && Wire.sda==21 && Wire.scl==22 && Wire.timeout==25);
 assert((Wire.registers[0]&0x7f)==0x20 && Wire.registers[1]==4 && Wire.registers[254]==121);
 for(int ch=0;ch<16;++ch) assert(Wire.registers[9+4*ch]==0x10);
 assert(out.enable(1500) && oe==LOW);
 assert(Wire.registers[8]==51 && Wire.registers[9]==1); // 307 ticks.
 for(int ch=1;ch<16;++ch) assert(Wire.registers[9+4*ch]==0x10);
 out.radioClosed(); assert(oe==HIGH); assert(!out.enable(1500) && out.interrupted());
 assert(out.takeRadioRelease()); assert(!out.takeRadioRelease());
 Wire.afterWrite=[&] { out.radioClosed(); };
 assert(!out.enable(1500) && oe==HIGH && out.interrupted());
 out.takeRadioRelease(); assert(out.enable(1500));
 Wire.writeFail=true; assert(!out.write(1600) && oe==HIGH); Wire.writeFail=false;
 assert(!out.enable(1500)); // Fault latches until reset/reinitialization.
 Wire=TwoWire(); PcaOutput nack; Wire.writeFail=true; assert(!nack.begin() && oe==HIGH);
 Wire=TwoWire(); PcaOutput missing; Wire.present=false; assert(!missing.begin() && oe==HIGH);
 Wire=TwoWire(); PcaOutput bad; Wire.badReadback=true; assert(!bad.begin() && oe==HIGH);
 Wire=TwoWire(); PcaOutput readfail; assert(readfail.begin()); Wire.readFail=true;
 assert(!readfail.healthy() && oe==HIGH);
 Wire=TwoWire(); PcaOutput range; assert(range.begin()); assert(!range.write(999) && oe==HIGH);
 puts("PASS checked PCA9685 configuration, channel isolation, OE/disconnect race and latched I2C failures");
}

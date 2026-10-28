#include "PcaOutputs.h"
#include <driver/gpio.h>

bool PcaOutputs::fail() { bad_=true; activeMask_=0; inhibit(); return false; }
void PcaOutputs::inhibit() {
  portENTER_CRITICAL(&mux_);
  gpio_set_level(GPIO_NUM_23,1);
  portEXIT_CRITICAL(&mux_);
}
void PcaOutputs::radioClosed() {
  portENTER_CRITICAL(&mux_);
  radioRelease_=true;
  gpio_set_level(GPIO_NUM_23,1);
  portEXIT_CRITICAL(&mux_);
}
bool PcaOutputs::takeRadioRelease() {
  portENTER_CRITICAL(&mux_);
  bool pending=radioRelease_; radioRelease_=false;
  portEXIT_CRITICAL(&mux_);
  return pending;
}
bool PcaOutputs::writeRegister(uint8_t reg,uint8_t value) {
  Wire.beginTransmission(Address); Wire.write(reg); Wire.write(value);
  return Wire.endTransmission()==0;
}
bool PcaOutputs::readRegister(uint8_t reg,uint8_t &value) {
  Wire.beginTransmission(Address); Wire.write(reg);
  if(Wire.endTransmission(false)!=0 || Wire.requestFrom(Address,uint8_t(1))!=1) return false;
  value=uint8_t(Wire.read()); return true;
}
bool PcaOutputs::healthy() {
  if(bad_) return false;
  uint8_t mode1=0,mode2=0,prescale=0;
  if(!readRegister(0x00,mode1) || !readRegister(0x01,mode2) || !readRegister(0xfe,prescale)
     || (mode1&0x7f)!=0x20 || mode2!=0x04 || prescale!=121) return fail();
  return true;
}
bool PcaOutputs::begin() {
  // Latch HIGH before changing GPIO to output. External pull-up covers reset.
  digitalWrite(OePin,HIGH); pinMode(OePin,OUTPUT);
  if(!Wire.begin(21,22,100000)) return fail();
  Wire.setTimeOut(25);
  if(!driver_.begin()) return fail();
  // Library begin/setPWMFreq hide individual write errors. Configure explicitly
  // under inhibited OE, check every write, and read back critical settings.
  if(!writeRegister(0x00,0x30) || !writeRegister(0x01,0x04)
     || !writeRegister(0xfe,121) || !writeRegister(0x00,0x20)) return fail();
  delay(1); // Internal oscillator startup >500us, only during disabled setup.
  if(!writeRegister(0x00,0xa0) || !healthy()) return fail();
  for(uint8_t channel=0;channel<16;++channel)
    if(driver_.setPWM(channel,0,4096)!=0) return fail();
  initialized_=true;
  return true;
}
static uint16_t pulseTicks(uint16_t us) {
  if(us<1000 || us>2000) return 0;
  uint16_t ticks=(uint32_t(us)*25+61)/122;
  if(ticks<205) ticks=205;
  if(ticks>409) ticks=409;
  return ticks;
}
bool PcaOutputs::setPulse(uint8_t joint,uint16_t us) {
  const uint16_t ticks=pulseTicks(us);
  if(bad_ || !initialized_ || joint>=fourleg::ChannelCount || !ticks || driver_.setPWM(joint,0,ticks)!=0) return fail();
  return true;
}
void PcaOutputs::attach(uint8_t joint,uint16_t us) {
  if(!healthy() || !setPulse(joint,us)) return;
  portENTER_CRITICAL(&mux_);
  if(!radioRelease_) {
    activeMask_|=uint16_t(1U<<joint);
    gpio_set_level(GPIO_NUM_23,0);
  }
  portEXIT_CRITICAL(&mux_);
}
void PcaOutputs::write(uint8_t joint,uint16_t us) {
  if(joint>=fourleg::ChannelCount || !(activeMask_&(1U<<joint))) { fail(); return; }
  setPulse(joint,us);
}
void PcaOutputs::detach(uint8_t joint) {
  if(joint>=fourleg::ChannelCount) { fail(); return; }
  if(!(activeMask_&(1U<<joint))) return;
  activeMask_&=uint16_t(~(1U<<joint));
  if(!activeMask_) inhibit();
  if(!bad_ && driver_.setPWM(joint,0,4096)!=0) fail();
}
void PcaOutputs::disableAll() {
  inhibit(); activeMask_=0;
  if(!initialized_ || bad_) return;
  for(uint8_t joint=0;joint<fourleg::ChannelCount;++joint)
    if(driver_.setPWM(joint,0,4096)!=0) { fail(); return; }
}

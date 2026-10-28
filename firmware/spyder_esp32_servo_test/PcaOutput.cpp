#include "PcaOutput.h"
#include <driver/gpio.h>

bool PcaOutput::fail() { bad_=true; disable(); return false; }
void PcaOutput::disable() {
  portENTER_CRITICAL(&mux_);
  gpio_set_level(GPIO_NUM_23,1);
  portEXIT_CRITICAL(&mux_);
}
void PcaOutput::radioClosed() {
  portENTER_CRITICAL(&mux_);
  radioRelease_=true;
  gpio_set_level(GPIO_NUM_23,1);
  portEXIT_CRITICAL(&mux_);
}
bool PcaOutput::takeRadioRelease() {
  portENTER_CRITICAL(&mux_);
  bool pending=radioRelease_; radioRelease_=false;
  portEXIT_CRITICAL(&mux_);
  return pending;
}
bool PcaOutput::writeRegister(uint8_t reg,uint8_t value) {
  Wire.beginTransmission(Address); Wire.write(reg); Wire.write(value);
  return Wire.endTransmission()==0;
}
bool PcaOutput::readRegister(uint8_t reg,uint8_t &value) {
  Wire.beginTransmission(Address); Wire.write(reg);
  if(Wire.endTransmission(false)!=0 || Wire.requestFrom(Address,uint8_t(1))!=1) return false;
  value=uint8_t(Wire.read()); return true;
}
bool PcaOutput::healthy() {
  if(bad_) return false;
  uint8_t mode1=0,mode2=0,prescale=0;
  if(!readRegister(0x00,mode1) || !readRegister(0x01,mode2) || !readRegister(0xfe,prescale)
     || (mode1&0x7f)!=0x20 || mode2!=0x04 || prescale!=121) return fail();
  return true;
}
bool PcaOutput::begin() {
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
  return true;
}
bool PcaOutput::write(uint16_t pulseUs) {
  const uint16_t ticks=servobench::pulseTicks(pulseUs);
  if(bad_ || !ticks || driver_.setPWM(0,0,ticks)!=0) return fail();
  return true;
}
bool PcaOutput::enable(uint16_t pulseUs) {
  cancelled_=false;
  disable();
  if(!healthy() || !write(pulseUs)) return false;
  portENTER_CRITICAL(&mux_);
  if(radioRelease_) cancelled_=true;
  else gpio_set_level(GPIO_NUM_23,0);
  portEXIT_CRITICAL(&mux_);
  return !cancelled_;
}

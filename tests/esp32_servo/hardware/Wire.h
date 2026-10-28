#pragma once
#include <stdint.h>
#include <array>
#include <vector>
#include <functional>
class TwoWire {
public:
 std::array<uint8_t,256> registers{};
 std::vector<uint8_t> bytes;
 bool present=true, writeFail=false, readFail=false, badReadback=false;
 uint8_t reg=0; uint16_t timeout=0; int sda=-1,scl=-1;
 std::function<void()> afterWrite;
 bool begin(int a,int b,uint32_t) { sda=a; scl=b; return present; }
 void setTimeOut(uint16_t t) { timeout=t; }
 void beginTransmission(uint8_t) { bytes.clear(); }
 unsigned write(uint8_t b) { bytes.push_back(b); return 1; }
 uint8_t endTransmission(bool=true) {
  if(!present || writeFail) return 4;
  if(bytes.empty()) return 0;
  reg=bytes[0];
  for(unsigned i=1;i<bytes.size();++i) registers[uint8_t(reg+i-1)]=bytes[i];
  return 0;
 }
 uint8_t requestFrom(uint8_t,uint8_t n) { return readFail || !present ? 0 : n; }
 int read() { return badReadback ? 255 : registers[reg++]; }
};
extern TwoWire Wire;

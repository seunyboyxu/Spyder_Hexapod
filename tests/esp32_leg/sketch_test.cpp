#include <string>
#include "Arduino.h"
int oe=-1; uint32_t fakeNow=0; std::vector<int> levels; FakeSerial Serial;
#include "spyder_esp32_leg_test.ino"
#include <cassert>
TwoWire Wire;
std::vector<uint8_t> savedNvs;
bool failNvs=false;
static void clearReplies() { txQueue->messages.clear(); }
static void usb(const char *s) { Serial.receive(s); loop(); clearReplies(); }
static void radio(const std::string &s) { SerialBT.receivePacket(s); loop(); clearReplies(); }
static void settle() {
 for(int i=0;i<30000 && leg.busy();++i) { fakeNow+=20; loop(); clearReplies(); }
 assert(!leg.busy());
}
static void calibrate() {
 for(int i=0;i<3;++i) {
  char text[32]; snprintf(text,sizeof(text),"cal %d\n",i); usb(text); settle();
  snprintf(text,sizeof(text),"check %d\n",i); usb(text);
  assert(leg.settings().joint[i].checked);
 }
}
int main() {
 // A valid saved record may restore checks, but must never activate a servo.
 auto bootSettings=leg.settings();
 for(auto &joint:bootSettings.joint) joint.checked=1;
 uint8_t bootBytes[legtest::SavedSize]; assert(legtest::encodeSettings(bootSettings,bootBytes));
 savedNvs.assign(bootBytes,bootBytes+sizeof(bootBytes));
 setup(); clearReplies(); assert(!leg.anyAttached() && oe==HIGH);
 assert(leg.settings().joint[0].checked && outputs.activeMask()==0);
 savedNvs.clear();
 usb("stand\n"); assert(!leg.anyAttached());
 usb("cal 0\n"); assert(leg.attached(0) && !leg.attached(1) && !leg.attached(2));
 assert(Wire.registers[8]==46 && Wire.registers[9]==1 && Wire.registers[13]==16);
 usb("move 0 95\n"); settle(); assert(leg.position(0)==950);
 usb("test 0 0\n"); settle(); assert(leg.position(0)==900);
 usb("off\n"); assert(!leg.anyAttached() && oe==HIGH);
 calibrate(); assert(outputs.activeMask()==7);
 usb("save\n"); assert(savedNvs.empty()); // Must release before storage.
 usb("stand\n"); settle();
 auto stand0=leg.position(0),stand1=leg.position(1),stand2=leg.position(2);
 usb("straight\n"); settle(); assert(leg.position(1)!=stand1 || leg.position(2)!=stand2);
 usb("step 1\n"); settle();
 assert(leg.position(0)==stand0 && leg.position(1)==stand1 && leg.position(2)==stand2);
 usb("step 3\n"); assert(leg.stepping()); usb("stop\n"); assert(!leg.busy() && leg.anyAttached());
 usb("off\n"); usb("save\n"); assert(savedNvs.size()==32);
 auto savedBeforeFailure=savedNvs; failNvs=true; usb("save\n");
 assert(savedNvs==savedBeforeFailure && !leg.anyAttached()); failNvs=false;
 usb("limits 0 70 110\n"); assert(!leg.settings().joint[0].checked);
 usb("load\n"); assert(leg.settings().joint[0].checked && !leg.anyAttached());
 auto good=savedNvs; savedNvs[4]^=1;
 usb("limits 0 70 110\n"); usb("load\n"); assert(!leg.settings().joint[0].checked);
 savedNvs=good; usb("load\n");
 calibrate(); usb("step 1\n"); assert(leg.busy());
 SerialBT.connected=true; bluetoothEvent(ESP_SPP_SRV_OPEN_EVT,nullptr); loop(); clearReplies();
 SerialBT.receivePacket("show\n"); loop();
 assert(!txQueue->messages.empty());
 TxMessage routed{}; memcpy(&routed,txQueue->messages.back().data(),sizeof(routed));
 assert(routed.source==1 && strstr(routed.text,"J2/ch2 foot")); clearReplies();
 radio(std::string(512,'\n')+"!"); assert(!leg.busy() && leg.anyAttached());
 radio("\n"); usb("step 1\n"); assert(leg.busy());
 assert(Serial.errorCb); Serial.errorCb(1); loop(); clearReplies(); assert(!leg.busy());
 radio("!\n"); assert(!leg.anyAttached() && oe==HIGH); radio("\n");
 Wire.afterWrite=[] { bluetoothEvent(ESP_SPP_CLOSE_EVT,nullptr); };
 usb("cal 0\n"); assert(!leg.anyAttached() && !hasFault() && oe==HIGH);
 usb("cal 1\n"); assert(leg.attached(1) && outputs.activeMask()==2);
 assert(Wire.registers[9]==16 && Wire.registers[17]==16);
 bluetoothEvent(ESP_SPP_CLOSE_EVT,nullptr); assert(oe==HIGH);
 loop(); clearReplies(); assert(!leg.anyAttached());
 usb("cal 0\n"); Wire.writeFail=true; usb("move 0 100\n"); fakeNow+=20; loop(); clearReplies();
 assert(hasFault() && !leg.anyAttached() && oe==HIGH);
 Wire.writeFail=false; usb("cal 0\n"); assert(!leg.anyAttached());
 for(int ch=3;ch<16;++ch) assert(Wire.registers[9+4*ch]==16);
 assert(Serial.written==0 && SerialBT.written==0);
 delete txQueue; delete bluetoothRx;
 puts("PASS actual ESP32 leg sketch: calibration, poses/finite steps, storage, stop/release, transport and I2C faults");
}

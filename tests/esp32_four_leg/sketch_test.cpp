#include <string>
#include "Arduino.h"
int oe=-1; uint32_t fakeNow=0; std::vector<int> levels; FakeSerial Serial;
#include "spyder_esp32_ble_four_leg_test.ino"
#include <cassert>
TwoWire Wire;
std::vector<uint8_t> savedNvs; bool failNvs=false;
static void clearReplies() { txQueue->messages.clear(); }
static void usb(const char *s) { Serial.receive(s); loop(); clearReplies(); }
static BLECharacteristic *rx() { return blefake::model().server.getService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")->getCharacteristic("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"); }
static BLECharacteristic *tx() { return blefake::model().server.getService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")->getCharacteristic("6E400003-B5A3-F393-E0A9-E50E24DCCA9E"); }
static void wireless(const std::string &s) { rx()->clientWrite(s); loop(); clearReplies(); }
static void settle() {
 for(int i=0;i<200000 && rig.busy();++i) { fakeNow+=20; loop(); clearReplies(); }
 assert(!rig.busy());
}
int main() {
 auto settings=rig.settings();
 for(auto &leg:settings.leg) for(auto &joint:leg.joint) joint.checked=1;
 uint8_t record[fourleg::SavedSize]; assert(fourleg::encodeSettings(settings,record));
 savedNvs.assign(record,record+sizeof(record));
 setup(); clearReplies(); assert(!rig.anyAttached() && outputs.activeMask()==0 && oe==HIGH);
 assert(rig.leg(3).settings().joint[2].checked);
 blefake::model().server.clientConnect(); loop(); clearReplies();
 wireless("cal 0\n"); assert(!rig.anyAttached());
 tx()->clientSubscribe(true); loop(); clearReplies();
 wireless("leg all\n"); wireless("limits 0 60 120\n");
 for(unsigned l=0;l<4;++l) assert(!rig.leg(l).settings().joint[0].checked);
 wireless("armall\n"); assert(!rig.arming()); // Calibration must not silently bypass checked arming.
 wireless("cal all\n"); assert(rig.arming());
 fakeNow+=100; loop(); clearReplies(); assert(outputs.activeMask()==1);
 tx()->clientSubscribe(false); assert(oe==HIGH); loop(); clearReplies();
 fakeNow+=2000; loop(); clearReplies(); assert(!rig.anyAttached() && !rig.arming());
 tx()->clientSubscribe(true); loop(); clearReplies();
 wireless("cal 0\n"); assert(outputs.activeMask()==0x0249);
 wireless("check all\n");
 for(unsigned l=0;l<4;++l) assert(!rig.leg(l).settings().joint[0].checked);
 wireless("off\n"); wireless("cal all\n"); settle(); assert(outputs.activeMask()==0x0fff);
 wireless("speed 5\n"); wireless("test 0 5\n"); assert(rig.coordinated()); settle();
 for(unsigned l=0;l<4;++l) assert(rig.leg(l).position(0)==950);
 wireless("test 0 0\n"); settle(); wireless("check all\n");
 for(unsigned l=0;l<4;++l) for(unsigned j=0;j<3;++j) assert(rig.leg(l).settings().joint[j].checked);
 wireless("off\n");
 wireless("leg 4\n"); wireless("cal 2\n"); assert(outputs.activeMask()==0x0800);
 wireless("off\n"); assert(!rig.anyAttached() && oe==HIGH);
 wireless("leg all\n"); wireless("armall\n"); assert(rig.arming());
 savedNvs.clear(); usb("save\n"); assert(savedNvs.empty()); // No storage during pre-attach stage.
 fakeNow+=100; loop(); clearReplies(); assert(outputs.activeMask()==1);
 wireless("stop\n"); fakeNow+=10000; loop(); clearReplies(); assert(outputs.activeMask()==1 && !rig.arming());
 wireless("off\n"); wireless("armall\n"); settle(); assert(outputs.activeMask()==0x0fff);
 wireless("stand\n"); settle();
 wireless("straight\n"); settle();
 wireless("step 1\n"); assert(rig.coordinated());
 rx()->clientWrite("show all\n"); loop();
 assert(rig.busy() && txQueue->messages.size()==13);
 TxMessage last{}; memcpy(&last,txQueue->messages.back().data(),sizeof(last));
 assert(last.source==1 && strstr(last.text,"L4 J2/ch11")); clearReplies();
 settle();
 wireless("step 3\n"); wireless("stop\n"); assert(!rig.busy() && outputs.activeMask()==0x0fff);
 wireless("off\n"); wireless("save\n"); assert(savedNvs.size()==fourleg::SavedSize);
 auto good=savedNvs;
 wireless("leg 4\n"); wireless("limits 2 65 115\n"); assert(!rig.leg(3).settings().joint[2].checked);
 wireless("load\n"); assert(rig.leg(3).settings().joint[2].checked && !rig.anyAttached());
 savedNvs[100]^=1; wireless("limits 2 65 115\n"); wireless("load\n"); assert(!rig.leg(3).settings().joint[2].checked);
 savedNvs=good; wireless("load\n"); wireless("armall\n"); settle();
 wireless("leg all\n"); wireless("step 1\n");
 rx()->clientWrite(std::string(512,'\n')); rx()->clientWrite("!"); loop(); clearReplies(); assert(!rig.busy() && rig.anyAttached());
 wireless("\n"); wireless("step 1\n"); assert(rig.busy());
 Serial.errorCb(1); loop(); clearReplies(); assert(!rig.busy());
 wireless("!\n"); assert(!rig.anyAttached() && oe==HIGH); wireless("\n");
 wireless("armall\n"); fakeNow+=100; loop(); clearReplies(); assert(outputs.activeMask()==1);
 tx()->clientSubscribe(false); assert(oe==HIGH); loop(); clearReplies();
 fakeNow+=10000; loop(); clearReplies(); assert(!rig.anyAttached() && !rig.arming());
 tx()->clientSubscribe(true); loop(); clearReplies();
 wireless("armall\n"); settle(); blefake::model().server.clientDisconnect(); assert(oe==HIGH);
 loop(); clearReplies(); assert(!rig.anyAttached());
 usb("armall\n"); settle(); usb("leg all\n"); usb("straight\n");
 Wire.writeFail=true; fakeNow+=20; loop(); clearReplies(); assert(hasFault() && !rig.anyAttached() && oe==HIGH);
 Wire.writeFail=false; usb("armall\n"); fakeNow+=1000; loop(); clearReplies(); assert(!rig.anyAttached());
 for(int ch=12;ch<16;++ch) assert(Wire.registers[9+4*ch]==16);
 delete txQueue; delete bluetoothRx;
 puts("PASS four-leg BLE sketch: 12-output activation/motion, streamed status, save/load, global stop/release and transport/I2C failure paths");
}

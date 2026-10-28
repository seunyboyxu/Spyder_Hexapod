// The walk sketch end to end with fake ESP32/BLE/PCA9685 hardware.
#include <string>
#include "Arduino.h"
int oe=-1; uint32_t fakeNow=0; std::vector<int> levels; FakeSerial Serial;
#include "spyder_esp32_ble_walk.ino"
#include <cassert>
TwoWire Wire;
std::vector<uint8_t> savedNvs; bool failNvs=false;
static void clearReplies() { txQueue->messages.clear(); }
static BLECharacteristic *rx() { return blefake::model().server.getService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")->getCharacteristic("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"); }
static BLECharacteristic *tx() { return blefake::model().server.getService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")->getCharacteristic("6E400003-B5A3-F393-E0A9-E50E24DCCA9E"); }
static void wireless(const std::string &s) { rx()->clientWrite(s); loop(); clearReplies(); }
static bool lastReplyHas(const char *text) {
 if(txQueue->messages.empty()) return false;
 TxMessage m{}; memcpy(&m,txQueue->messages.back().data(),sizeof(m)); return strstr(m.text,text)!=nullptr;
}
static void settle() {
 for(int i=0;i<200000 && rig.busy();++i) { fakeNow+=20; loop(); clearReplies(); }
 assert(!rig.busy());
}
int main() {
 // Calibration saved by the four-leg test sketch (same storage) is used.
 auto settings=rig.settings();
 for(auto &leg:settings.leg) for(auto &joint:leg.joint) joint.checked=1;
 uint8_t record[fourleg::SavedSize]; assert(fourleg::encodeSettings(settings,record));
 savedNvs.assign(record,record+sizeof(record));
 setup(); clearReplies(); assert(!rig.anyAttached() && oe==HIGH);
 blefake::model().server.clientConnect(); loop(); clearReplies();
 tx()->clientSubscribe(true); loop(); clearReplies();
 rx()->clientWrite("help\n"); loop(); assert(lastReplyHas("walk/back/left/right")); clearReplies();
 wireless("walk\n"); assert(!rig.walking()); // outputs off: rejected
 wireless("armall\n"); settle(); assert(outputs.activeMask()==0x0fff);
 wireless("walk 1\n"); assert(rig.walking());
 rx()->clientWrite("show\n"); loop(); assert(txQueue->messages.size()>=1);
 { TxMessage m{}; memcpy(&m,txQueue->messages.front().data(),sizeof(m)); assert(strstr(m.text,"WALKING")); }
 clearReplies();
 settle(); assert(!rig.walking() && outputs.activeMask()==0x0fff);
 wireless("left\n"); for(int i=0;i<200;++i) { fakeNow+=20; loop(); clearReplies(); }
 wireless("halt\n"); settle();
 wireless("back\n"); for(int i=0;i<200;++i) { fakeNow+=20; loop(); clearReplies(); }
 wireless("stop\n"); assert(!rig.busy() && outputs.activeMask()==0x0fff);
 // A runtime sample between preflight frames reports ERR, never Ready.
 wireless("off\n");
 auto narrow=rig.settings(); narrow.leg[3].joint[2].minimum=744;
 assert(rig.applySettings(narrow));
 wireless("armall\n"); settle(); wireless("leg all\n"); wireless("stand\n"); settle();
 wireless("walk\n"); fakeNow+=19; loop(); clearReplies();
 bool rejected=false;
 for(int i=0;i<2000 && rig.walking();++i) {
  fakeNow+=21; loop();
  if(lastReplyHas("ERR walking target rejected")) {
   assert(!rig.walking() && !rig.busy() && outputs.activeMask()==0x0fff);
   rejected=true;
  }
  assert(!lastReplyHas("Ready")); clearReplies();
 }
 assert(rejected);
 wireless("off\n"); wireless("load\n"); wireless("armall\n"); settle();
 wireless("walk\n"); for(int i=0;i<100;++i) { fakeNow+=20; loop(); clearReplies(); }
 tx()->clientSubscribe(false); assert(oe==HIGH); loop(); clearReplies();
 assert(!rig.anyAttached() && !rig.walking()); // radio loss releases everything
 delete txQueue; delete bluetoothRx;
 puts("PASS walk BLE sketch: shared calibration, checked arming, walk/left/back, halt, stop and BLE-loss release");
}

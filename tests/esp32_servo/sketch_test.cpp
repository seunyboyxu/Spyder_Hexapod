#include <string>
#include "Arduino.h"
int oe=-1; uint32_t fakeNow=0; std::vector<int> levels; FakeSerial Serial;
#include "spyder_esp32_servo_test.ino"
#include <cassert>
TwoWire Wire;
static void clearReplies() { txQueue->messages.clear(); }
static void usb(const char *s) { Serial.receive(s); loop(); clearReplies(); }
static void radio(const std::string &s) { SerialBT.receivePacket(s); loop(); clearReplies(); }
int main() {
 setup(); assert(!bench.armed() && oe==HIGH);
 for(int level:levels) assert(level==HIGH);
 clearReplies();
 usb("arm\n"); assert(bench.armed() && oe==LOW);
 usb("pulse 1600\n"); fakeNow+=20; loop(); clearReplies(); assert(bench.pulse()==1501);
 SerialBT.connected=true; bluetoothEvent(ESP_SPP_SRV_OPEN_EVT,nullptr); loop(); clearReplies();
 SerialBT.receivePacket("show\n"); loop();
 assert(!txQueue->messages.empty());
 TxMessage message{}; memcpy(&message,txQueue->messages.back().data(),sizeof(message));
 assert(message.source==1 && strstr(message.text,"pulse=1501us")); clearReplies();
 // Dropping a transport packet must hold before the old target takes another step.
 radio(std::string(512,'\n')+"!");
 assert(bench.target()==bench.pulse() && !bench.busy());
 radio("\n"); usb("pulse 1700\n");
 assert(bench.target()==1700);
 if(Serial.errorCb) Serial.errorCb(1);
 loop(); clearReplies();
 assert(bench.target()==bench.pulse());
 usb("\n"); usb("pulse 1700\n");
 bluetoothEvent(ESP_SPP_CLOSE_EVT,nullptr); assert(oe==HIGH);
 loop(); clearReplies(); assert(!bench.armed());
 usb("arm\n");
 radio("!\n"); assert(!bench.armed() && oe==HIGH);
 radio("\n");
 // Trigger a close at the exact preload/enable boundary.
 Wire.afterWrite=[] { bluetoothEvent(ESP_SPP_CLOSE_EVT,nullptr); };
 usb("arm\n"); assert(!bench.armed() && !bench.faulted() && oe==HIGH);
 usb("arm\n"); assert(bench.armed());
 assert(Serial.written==0 && SerialBT.written==0); // Loop hands replies to worker.
 Wire.present=false; fakeNow+=250; loop(); clearReplies();
 assert(bench.faulted() && !bench.armed() && oe==HIGH);
 usb("arm\n"); assert(!bench.armed());
 delete txQueue; delete bluetoothRx;
 puts("PASS actual sketch command routing, transport overflow/errors, disconnect/arm race and I2C fault");
}

// Original ESP32 + PCA9685: four legs, channels0..11, hip/knee/foot triples.
// The four-leg BLE test controller plus the SunFounder/quadpod crawl gait
// (walk, back, left, right, halt). Uses the same saved calibration as the four-leg test.
// Wiring, calibration and walking steps are in README.md in this folder.
#include <Arduino.h>
#include "BleUart.h"
#include <SPI.h> // Adafruit BusIO also includes its SPI helper.
#include <atomic>
#include "PcaOutputs.h"
#include "FourLegTest.h"
#include "CommandInputs.h"
#include "FourLegSafety.h"
#include "NvSettings.h"

#if !defined(CONFIG_IDF_TARGET_ESP32) || !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error "Select the verified original ESP32 Dev Module target with its bundled BLE library."
#endif

BleUart radio;
PcaOutputs outputs;
fourleg::FourLegTest rig(outputs);
FourLegSafety safety(rig,outputs);
legtest::CommandInputs inputs(safety);
bool systemFault=false, faultReported=false;
uint8_t motionSource=0;

// Reply transmission must never block the loop that handles safety commands.
// Custom types live in headers to avoid Arduino IDE auto-prototype problems.
struct TxMessage {
  uint32_t epoch;
  uint32_t radioEpoch;
  uint8_t source;
  char text[320];
};
QueueHandle_t txQueue=nullptr;
QueueHandle_t bluetoothRx=nullptr;
std::atomic<bool> inputCorrupt{false};
std::atomic<uint32_t> replyEpoch{0};
std::atomic<bool> radioOpened{false};
uint32_t lastHealth=0;
bool bluetoothReady=false;

void transmitReplies(void *unused) {
  (void)unused;
  TxMessage message;
  for(;;) {
    if(xQueueReceive(txQueue,&message,portMAX_DELAY)!=pdTRUE) continue;
    const size_t length=strlen(message.text);
    for(size_t offset=0;offset<length;) {
      if(message.epoch!=replyEpoch.load()) break;
      size_t count=length-offset;
      if(count>32) count=32;
      if(message.source==1) {
        if(!radio.ready()) break;
        // BLE notifications are20-byte chunks, paced by the radio adapter.
        size_t written=radio.write(reinterpret_cast<const uint8_t *>(message.text+offset),count,message.radioEpoch);
        if(!written) break;
        offset+=written;
      } else {
        int room=Serial.availableForWrite();
        if(room<=0) { vTaskDelay(pdMS_TO_TICKS(2)); continue; }
        if(count>size_t(room)) count=size_t(room);
        offset+=Serial.write(reinterpret_cast<const uint8_t *>(message.text+offset),count);
      }
    }
  }
}
void drainInputs() {
  // A bounded snapshot: do not wait for silence on a continuously busy port.
  int count=Serial.available(); while(count-->0) Serial.read();
  if(bluetoothRx) {
    count=uxQueueMessagesWaiting(bluetoothRx);
    uint8_t ignored;
    while(count-->0) xQueueReceive(bluetoothRx,&ignored,0);
  }
}
void invalidateReplies() { replyEpoch.fetch_add(1); }
void replyText(uint8_t source,const char *text) {
  if(!txQueue || (source==1 && !radio.ready())) return;
  TxMessage message{};
  message.epoch=replyEpoch.load(); message.radioEpoch=radio.epoch(); message.source=source;
  snprintf(message.text,sizeof(message.text),"%s\r\n",text);
  if(xQueueSend(txQueue,&message,0)!=pdTRUE) {
    // Overwhelming replies must not stall motor/input servicing.
    inputs.cancelAll(false); invalidateReplies(); drainInputs();
  }
}
bool hasFault() { return systemFault || outputs.failed(); }
void show(uint8_t source,bool all) {
  char text[256],selection[12];
  if(rig.selected()==fourleg::All) strcpy(selection,"all");
  else snprintf(selection,sizeof(selection),"leg %u",unsigned(rig.selected()+1));
  snprintf(text,sizeof(text),"Spyder crawl BLE v1.0 | selected=%s | %s | %s\r\n"
    "PCA legs1=0..2 2=3..5 3=6..8 4=9..11 | SDA21 SCL22 OE23",
    selection,hasFault()?"FAULT: reset required":rig.anyAttached()?"OUTPUTS ACTIVE":"OFF",
    rig.arming()?"ARMING":rig.walking()?"WALKING":rig.coordinated()?"SYNC TEST":rig.busy()?"MOVING":"STILL");
  replyText(source,text);
  const bool every=all || rig.selected()==fourleg::All;
  uint8_t first=every?0:rig.selected(),last=every?fourleg::LegCount:uint8_t(first+1);
  for(uint8_t l=first;l<last;++l) for(uint8_t j=0;j<3;++j) {
    const auto &leg=rig.leg(l); const auto &cal=leg.settings().joint[j];
    snprintf(text,sizeof(text),"L%u J%u/ch%u %s | attached=%u checked=%u | command=%.1f deg (%uus) | zero=%.1f limits=%.1f..%.1f dir=%d speed=%u",
      unsigned(l+1),unsigned(j),unsigned(3*l+j),j==0?"hip":j==1?"knee":"foot",unsigned(leg.attached(j)),unsigned(cal.checked),
      leg.position(j)/10.0,unsigned(legtest::pulseFor(leg.position(j))),cal.zero/10.0,
      cal.minimum/10.0,cal.maximum/10.0,int(cal.direction),unsigned(leg.speed()));
    replyText(source,text);
  }
}
void uartReceiveError(hardwareSerial_error_t error) {
  if(error!=0) inputCorrupt.store(true);
}
void bluetoothData(const uint8_t *data,size_t size) {
  // Bound complete BLE writes; preserve fragmented ASCII lines in the parser.
  // One producer (Bluetooth event task); consumer can only increase free space.
  if(!bluetoothRx || size>uxQueueSpacesAvailable(bluetoothRx)) {
    inputCorrupt.store(true); return; // Reject the entire packet.
  }
  for(size_t i=0;i<size;++i) {
    if(xQueueSend(bluetoothRx,data+i,0)!=pdTRUE) { inputCorrupt.store(true); return; }
  }
}
void bleConnected() { radioOpened.store(true); }
void bleReleased() {
  outputs.radioClosed(); // Guarded immediate inhibition on disconnect/unsubscribe.
  radioOpened.store(false);
}
void serviceSafetyEvents() {
  if(outputs.takeRadioRelease()) {
    inputs.cancelAll(true); invalidateReplies(); drainInputs();
    replyText(0,"OFF: BLE disconnected or notifications disabled; select leg N then cal J, or checked armall");
  }
  if(radio.takeError()) inputCorrupt.store(true);
  if(inputCorrupt.exchange(false)) {
    inputs.cancelAll(false); invalidateReplies(); drainInputs();
    replyText(0,"ERR transport integrity; held. Send blank newline then help.");
    replyText(1,"ERR transport integrity; held. Send blank newline then help.");
  }
  if(outputs.failed() && !faultReported) {
    faultReported=true; systemFault=true; inputs.cancelAll(true); invalidateReplies(); drainInputs();
    replyText(0,"ERR PCA9685/I2C fault: outputs OFF; fix wiring with servo power off, reset.");
    replyText(1,"ERR PCA9685/I2C fault: outputs OFF; fix wiring with servo power off, reset.");
  }
  if(radioOpened.exchange(false)) {
    inputs.resetSource(1);
    replyText(1,"Spyder-Crawl-BLE ready; outputs never activate automatically. Send show/help.");
  }
}
void processNotices() {
  for(uint8_t source=0;source<2;++source) {
    uint8_t notice=inputs.notice(source);
    if(!notice) continue;
    invalidateReplies(); drainInputs();
    if(notice&legtest::CommandInputs::Released) replyText(source,"OFF: all12 signals disabled. Use checked armall or individual cal J.");
    else if(notice&legtest::CommandInputs::Held) replyText(source,"STOP: holding current commands; sequence cancelled.");
    else replyText(source,"ERR input integrity/timeout; held. Send blank newline then help.");
  }
}
void processCommand(uint8_t source,char *line) {
  if(!strcmp(line,"help")) {
    replyText(source,"leg 1..4/all | show [all] | cal J/all | move J DEG | zero J\r\n"
      "test J OFFSET | flip J | check J/all | limits J MIN MAX\r\n"
      "speed 1..20 | armall | stand | straight | step [1..3]\r\n"
      "walk/back/left/right [cycles] | halt: finish move, stand\r\n"
      "stop/off/! act on ALL legs | save/load: all OFF\r\n"
      "J=0 hip,1 knee,2 foot; leg all applies to all four. Support body."); return;
  }
  if(!strcmp(line,"show") || !strcmp(line,"show all")) { show(source,!strcmp(line,"show all")); return; }
  if(source==1 && !radio.ready()) {
    replyText(0,"ERR BLE command rejected: enable TX notifications in the phone app first."); return;
  }
  if(hasFault()) { replyText(source,"ERR setup/I2C fault: outputs OFF, reset required."); return; }
  if(!strcmp(line,"save") || !strcmp(line,"load")) {
    if(rig.anyAttached() || rig.busy()) { replyText(source,"ERR send off before save/load; staging must be stopped."); return; }
    if(!strcmp(line,"save")) replyText(source,fourleg::saveCalibration(rig.settings())?"OK calibration saved; outputs OFF.":"ERR calibration not saved.");
    else {
      fourleg::Settings settings;
      replyText(source,fourleg::loadCalibration(settings) && rig.applySettings(settings)?"OK calibration loaded; outputs OFF.":"ERR no valid saved calibration; settings unchanged.");
    }
    return;
  }
  const uint16_t generation=inputs.generation();
  const char *result=rig.command(line,millis());
  serviceSafetyEvents();
  if(hasFault()) { replyText(source,"ERR output fault: signals disabled, reset required."); return; }
  if(generation!=inputs.generation()) return; // Safety event superseded the result.
  if(!strcmp(result,"OK") && rig.busy()) motionSource=source;
  replyText(source,result);
}
void readPort(uint8_t source) {
  for(uint8_t n=0;n<24;++n) {
    serviceSafetyEvents();
    uint8_t byte=0;
    int value=source==0 ? Serial.read() :
      (bluetoothRx && xQueueReceive(bluetoothRx,&byte,0)==pdTRUE ? int(byte) : -1);
    if(value<0) break;
    inputs.feed(source,char(value),millis());
    processNotices();
    serviceSafetyEvents();
    char line[legtest::CommandInputs::LineSize];
    if(inputs.take(source,line)) processCommand(source,line);
  }
}
void setup() {
  // A verified external OE pull-up inhibits outputs through reset/bootloader.
  digitalWrite(23,HIGH); pinMode(23,OUTPUT);
  Serial.begin(115200);
  Serial.onReceiveError(uartReceiveError);
  bluetoothRx=xQueueCreate(512,sizeof(uint8_t));
  txQueue=xQueueCreate(32,sizeof(TxMessage));
  if(!bluetoothRx || !txQueue || xTaskCreate(transmitReplies,"servo-replies",4096,nullptr,1,nullptr)!=pdPASS) {
    systemFault=true; safety.release(); return;
  }
  if(!outputs.begin()) systemFault=true;
  rig.begin(millis());
  fourleg::Settings stored;
  if(!systemFault && fourleg::loadCalibration(stored)) rig.applySettings(stored);
  bluetoothReady=radio.begin("Spyder-Crawl-BLE",bluetoothData,bleConnected,bleReleased);
  if(!bluetoothReady) { systemFault=true; safety.release(); }
  replyText(0,"Spyder crawl BLE v1.0; USB 115200/Newline; BLE Spyder-Crawl-BLE (connect inside iPhone app)");
  replyText(0,bluetoothReady?"All outputs OFF. Four legs use consecutive triples0..11. Send help/show.":"FAULT: Bluetooth failed to start; reset required.");
  show(0,false);
}
void loop() {
  if(!txQueue || !bluetoothRx) { outputs.inhibit(); delay(1); return; }
  serviceSafetyEvents();
  radio.poll();
  readPort(0); readPort(1);
  inputs.expire(millis()); processNotices();
  const uint32_t now=millis();
  if(!hasFault() && uint32_t(now-lastHealth)>=250) {
    lastHealth=now;
    if(!outputs.healthy()) serviceSafetyEvents();
  }
  serviceSafetyEvents();
  if(!hasFault()) {
    const bool wasBusy=rig.busy();
    rig.tick(millis());
    serviceSafetyEvents();
    const bool walkError=rig.takeWalkError();
    if(walkError) replyText(motionSource,"ERR walking target rejected; all legs held. Check limits; use stop/off.");
    else if(wasBusy && !rig.busy() && rig.anyAttached() && !hasFault()) replyText(motionSource,"Ready: target/sequence completed (commanded positions).");
  }
  delay(1);
}

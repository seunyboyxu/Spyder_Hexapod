// Original ESP32 + PCA9685, ONE LEG: hip=0, knee=1, foot=2.
// Full wiring and test procedure are in README.md in this sketch folder.
#include <Arduino.h>
#include <BluetoothSerial.h>
#include <SPI.h> // Adafruit BusIO also includes its SPI helper.
#include <atomic>
#include "PcaLegOutput.h"
#include "LegTest.h"
#include "CommandInputs.h"
#include "LegSafety.h"
#include "NvSettings.h"

#if !defined(CONFIG_IDF_TARGET_ESP32) || !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED) || !defined(CONFIG_BT_SPP_ENABLED)
#error "Select an original ESP32 board with Bluetooth Classic SPP (e.g. ESP32 Dev Module). C3/S3/S2 are unsupported."
#endif

BluetoothSerial SerialBT;
PcaLegOutput outputs;
legtest::LegTest leg(outputs);
LegSafety safety(leg,outputs);
legtest::CommandInputs inputs(safety);
bool systemFault=false, faultReported=false;
uint8_t motionSource=0;

// Reply transmission must never block the loop that handles safety commands.
// Custom types live in headers to avoid Arduino IDE auto-prototype problems.
struct TxMessage {
  uint32_t epoch;
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
        if(!SerialBT.hasClient()) break;
        // BluetoothSerial can wait on congestion. Only this worker waits.
        size_t written=SerialBT.write(reinterpret_cast<const uint8_t *>(message.text+offset),count);
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
  if(!txQueue || (source==1 && !SerialBT.hasClient())) return;
  TxMessage message{};
  message.epoch=replyEpoch.load(); message.source=source;
  snprintf(message.text,sizeof(message.text),"%s\r\n",text);
  if(xQueueSend(txQueue,&message,0)!=pdTRUE) {
    // Overwhelming replies must not stall motor/input servicing.
    inputs.cancelAll(false); invalidateReplies(); drainInputs();
  }
}
bool hasFault() { return systemFault || outputs.failed(); }
void show(uint8_t source) {
  char text[240];
  snprintf(text,sizeof(text),"Spyder ESP32 leg test v1 | %s | %s | speed=%u deg/s\r\n"
    "PCA9685 hip=0 knee=1 foot=2 | SDA=GPIO21 SCL=GPIO22 OE=GPIO23",
    hasFault()?"FAULT: reset required":leg.anyAttached()?"OUTPUTS ACTIVE":"OFF",
    leg.busy()?"MOVING":"STILL",unsigned(leg.speed()));
  replyText(source,text);
  for(uint8_t i=0;i<3;++i) {
    const legtest::Calibration &cal=leg.settings().joint[i];
    snprintf(text,sizeof(text),"J%u/ch%u %s | attached=%u checked=%u | command=%.1f deg (%uus) | zero=%.1f limits=%.1f..%.1f dir=%d",
      unsigned(i),unsigned(i),i==0?"hip":i==1?"knee":"foot",unsigned(leg.attached(i)),unsigned(cal.checked),
      leg.position(i)/10.0,unsigned(legtest::pulseFor(leg.position(i))),cal.zero/10.0,
      cal.minimum/10.0,cal.maximum/10.0,int(cal.direction));
    replyText(source,text);
  }
}
void uartReceiveError(hardwareSerial_error_t error) {
  if(error!=0) inputCorrupt.store(true);
}
void bluetoothData(const uint8_t *data,size_t size) {
  // onData bypasses BluetoothSerial's silently overflowing default RX queue.
  // One producer (Bluetooth event task); consumer can only increase free space.
  if(!bluetoothRx || size>uxQueueSpacesAvailable(bluetoothRx)) {
    inputCorrupt.store(true); return; // Reject the entire packet.
  }
  for(size_t i=0;i<size;++i) {
    if(xQueueSend(bluetoothRx,data+i,0)!=pdTRUE) { inputCorrupt.store(true); return; }
  }
}
void bluetoothEvent(esp_spp_cb_event_t event,esp_spp_cb_param_t *parameter) {
  (void)parameter;
  if(event==ESP_SPP_CLOSE_EVT) {
    outputs.radioClosed(); // Immediate OE inhibition, serialized with enable.
    radioOpened.store(false);
  } else if(event==ESP_SPP_SRV_OPEN_EVT) radioOpened.store(true);
}
void serviceSafetyEvents() {
  if(outputs.takeRadioRelease()) {
    inputs.cancelAll(true); invalidateReplies(); drainInputs();
    replyText(0,"OFF: Bluetooth disconnected; use cal J explicitly");
  }
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
    replyText(1,"Spyder-Leg-Test connected; outputs never arm automatically. Send show/help.");
  }
}
void processNotices() {
  for(uint8_t source=0;source<2;++source) {
    uint8_t notice=inputs.notice(source);
    if(!notice) continue;
    invalidateReplies(); drainInputs();
    if(notice&legtest::CommandInputs::Released) replyText(source,"OFF: all leg signals disabled. Use cal J to reattach.");
    else if(notice&legtest::CommandInputs::Held) replyText(source,"STOP: holding current commands; sequence cancelled.");
    else replyText(source,"ERR input integrity/timeout; held. Send blank newline then help.");
  }
}
void processCommand(uint8_t source,char *line) {
  if(!strcmp(line,"help")) {
    replyText(source,"show/help | cal J | move J DEG | zero J | test J OFFSET\r\n"
      "flip J | check J | limits J MIN MAX | speed 1..20\r\n"
      "stand | straight | step [1..3] | stop | off/!\r\n"
      "save/load: outputs OFF; J=0 hip,1 knee,2 foot. One command per line."); return;
  }
  if(!strcmp(line,"show")) { show(source); return; }
  if(hasFault()) { replyText(source,"ERR setup/I2C fault: outputs OFF, reset required."); return; }
  if(!strcmp(line,"save") || !strcmp(line,"load")) {
    if(leg.anyAttached()) { replyText(source,"ERR send off before save/load."); return; }
    if(!strcmp(line,"save")) replyText(source,legtest::saveCalibration(leg.settings())?"OK calibration saved; outputs OFF.":"ERR calibration not saved.");
    else {
      legtest::Settings settings;
      replyText(source,legtest::loadCalibration(settings) && leg.applySettings(settings)?"OK calibration loaded; outputs OFF.":"ERR no valid saved calibration; settings unchanged.");
    }
    return;
  }
  const uint16_t generation=inputs.generation();
  const char *result=leg.command(line);
  serviceSafetyEvents();
  if(hasFault()) { replyText(source,"ERR output fault: signals disabled, reset required."); return; }
  if(generation!=inputs.generation()) return; // Safety event superseded the result.
  if(!strcmp(result,"OK") && leg.busy()) motionSource=source;
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
  txQueue=xQueueCreate(12,sizeof(TxMessage));
  if(!bluetoothRx || !txQueue || xTaskCreate(transmitReplies,"servo-replies",4096,nullptr,1,nullptr)!=pdPASS) {
    systemFault=true; safety.release(); return;
  }
  if(!outputs.begin()) systemFault=true;
  leg.begin(millis());
  legtest::Settings stored;
  if(!systemFault && legtest::loadCalibration(stored)) leg.applySettings(stored);
  SerialBT.onData(bluetoothData);
  SerialBT.register_callback(bluetoothEvent);
  bluetoothReady=SerialBT.begin("Spyder-Leg-Test");
  if(!bluetoothReady) { systemFault=true; safety.release(); }
  replyText(0,"Spyder ESP32 leg test v1; USB 115200/Newline; Bluetooth Spyder-Leg-Test");
  replyText(0,bluetoothReady?"All outputs OFF. One leg: hip=0 knee=1 foot=2. Send help/show.":"FAULT: Bluetooth failed to start; reset required.");
  show(0);
}
void loop() {
  if(!txQueue || !bluetoothRx) { outputs.inhibit(); delay(1); return; }
  serviceSafetyEvents();
  readPort(0); readPort(1);
  inputs.expire(millis()); processNotices();
  const uint32_t now=millis();
  if(!hasFault() && uint32_t(now-lastHealth)>=250) {
    lastHealth=now;
    if(!outputs.healthy()) serviceSafetyEvents();
  }
  serviceSafetyEvents();
  if(!hasFault()) {
    const bool wasBusy=leg.busy();
    leg.tick(millis());
    serviceSafetyEvents();
    if(wasBusy && !leg.busy() && leg.anyAttached() && !hasFault()) replyText(motionSource,"Ready: target/sequence completed (commanded positions).");
  }
  delay(1);
}

// Original ESP32 + PCA9685, ONE UNLOADED SERVO on channel 0.
// Full wiring and test procedure are in README.md in this sketch folder.
#include <Arduino.h>
#include <BluetoothSerial.h>
#include <SPI.h> // Adafruit BusIO also includes its SPI helper.
#include <atomic>
#include "PcaOutput.h"
#include "ServoBench.h"

#if !defined(CONFIG_IDF_TARGET_ESP32) || !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED) || !defined(CONFIG_BT_SPP_ENABLED)
#error "Select an original ESP32 board with Bluetooth Classic SPP (e.g. ESP32 Dev Module). C3/S3/S2 are unsupported."
#endif

using servobench::Result;
BluetoothSerial SerialBT;
PcaOutput motor;
servobench::Bench bench(motor);
servobench::Inputs inputs(bench);

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
    bench.hold(); inputs.clear(); invalidateReplies(); drainInputs();
  }
}
void replyResult(uint8_t source,Result result) {
  switch(result) {
    case Result::None: return;
    case Result::Help:
      replyText(source,"arm | center | angle 0..180 | pulse 1000..2000\r\n"
        "1/2/3: 2s at 1300/1500/1700us then off\r\n"
        "sweep: one centre/low/high/centre pass\r\n"
        "stop: hold | off/!: release | show | help\r\n"
        "One command per line. ! needs no Enter."); return;
    case Result::Show: {
      char text[200];
      snprintf(text,sizeof(text),"Spyder ESP32 servo bench v1 | ch=0 | %s | pulse=%uus target=%uus | %s\r\n"
        "GPIO21=SDA GPIO22=SCL GPIO23=OE | nominal PWM ~50Hz | no position feedback",
        bench.faulted()?"FAULT: reset required":bench.armed()?"ARMED":"OFF",
        unsigned(bench.pulse()),unsigned(bench.target()),bench.busy()?"TEST/SWEEP":"manual/hold");
      replyText(source,text); return;
    }
    case Result::Ok: replyText(source,"OK; show reports current command/target"); return;
    case Result::Invalid: replyText(source,"ERR command/range; send help"); return;
    case Result::Busy: replyText(source,"ERR busy/already armed; stop or off first"); return;
    case Result::Unarmed: replyText(source,"ERR output off; send arm explicitly"); return;
    case Result::Fault: replyText(source,"ERR I2C/setup fault; power off servo, fix wiring, reset ESP32"); return;
    case Result::Held: replyText(source,"STOP: holding current command; sequence cancelled"); return;
    case Result::Released: replyText(source,"OFF: signal disabled; fresh arm required"); return;
    case Result::InputError: replyText(source,"ERR input integrity/timeout; motion held; send newline then help"); return;
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
    motor.radioClosed(); // Immediate OE inhibition, serialized with enable.
    radioOpened.store(false);
  } else if(event==ESP_SPP_SRV_OPEN_EVT) radioOpened.store(true);
}
void serviceSafetyEvents() {
  if(motor.takeRadioRelease()) {
    inputs.disconnect(); invalidateReplies(); drainInputs();
    replyText(0,"OFF: Bluetooth disconnected; fresh arm required");
  }
  if(inputCorrupt.exchange(false)) {
    bench.hold(); inputs.clear(); invalidateReplies(); drainInputs();
    replyResult(0,Result::InputError); replyResult(1,Result::InputError);
  }
  if(radioOpened.exchange(false)) {
    inputs.resetSource(1);
    replyText(1,"Spyder-Servo-Test connected; outputs never arm automatically. Send show/help.");
  }
}
void readPort(uint8_t source) {
  // Bounded per-port work prevents a noisy port starving the other or motion.
  for(uint8_t n=0;n<24;++n) {
    serviceSafetyEvents();
    uint8_t byte=0;
    int value=source==0 ? Serial.read() :
      (bluetoothRx && xQueueReceive(bluetoothRx,&byte,0)==pdTRUE ? int(byte) : -1);
    if(value<0) break;
    Result result=inputs.feed(source,char(value),millis());
    if(result==Result::Held || result==Result::Released || result==Result::InputError) {
      invalidateReplies(); drainInputs();
    }
    serviceSafetyEvents();
    replyResult(source,result);
  }
}
void setup() {
  // External 10k OE pull-up covers time before setup, including bootloader.
  digitalWrite(23,HIGH); pinMode(23,OUTPUT);
  Serial.begin(115200);
  Serial.onReceiveError(uartReceiveError);
  bluetoothRx=xQueueCreate(512,sizeof(uint8_t));
  txQueue=xQueueCreate(6,sizeof(TxMessage));
  if(!bluetoothRx || !txQueue || xTaskCreate(transmitReplies,"servo-replies",4096,nullptr,1,nullptr)!=pdPASS) {
    bench.fault(); return;
  }
  if(!motor.begin()) bench.fault();
  SerialBT.onData(bluetoothData);
  SerialBT.register_callback(bluetoothEvent);
  bluetoothReady=SerialBT.begin("Spyder-Servo-Test");
  if(!bluetoothReady) bench.fault();
  replyText(0,"Spyder ESP32 servo bench v1; USB 115200/Newline; Bluetooth Spyder-Servo-Test");
  replyText(0,bluetoothReady?"All outputs OFF. One unloaded servo on channel 0. Send help/show.":"FAULT: Bluetooth failed to start; reset required.");
  replyResult(0,Result::Show);
}
void loop() {
  if(!txQueue) { motor.disable(); delay(1); return; }
  serviceSafetyEvents();
  readPort(0); readPort(1);
  Result expired=inputs.expire(millis());
  if(expired!=Result::None) {
    invalidateReplies(); drainInputs(); replyResult(0,expired); replyResult(1,expired);
  }
  const uint32_t now=millis();
  if(!bench.faulted() && uint32_t(now-lastHealth)>=250) {
    lastHealth=now;
    if(!motor.healthy()) {
      bench.fault(); inputs.clear(); invalidateReplies(); drainInputs();
      replyResult(0,Result::Fault); replyResult(1,Result::Fault);
    }
  }
  serviceSafetyEvents();
  bench.tick(millis());
  delay(1); // Yield to Bluetooth/FreeRTOS; motion never waits on a sweep delay.
}

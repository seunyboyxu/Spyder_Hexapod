#include "Control.h"
#include "Protocol.h"
#include "Storage.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <Servo.h>
#include <avr/interrupt.h>
using namespace spyder;
class ServoOutputs : public Outputs {
  Servo servos[JointCount];

public:
  void begin() {
    for (uint8_t i = 0; i < JointCount; ++i) {
      digitalWrite(Controller::pin(i), LOW);
      pinMode(Controller::pin(i), OUTPUT);
    }
  }
  void attach(uint8_t i, uint8_t pin, uint16_t pulse) override {
    servos[i].writeMicroseconds(pulse);
    servos[i].attach(pin, 1000, 2000);
  }
  void write(uint8_t i, uint16_t pulse) override {
    servos[i].writeMicroseconds(pulse);
  }
  void detach(uint8_t i) override {
    // Detach alone can leave an in-progress pulse HIGH on AVR Servo.
    // Preserve the caller's interrupt state while terminating the output.
    const uint8_t saved = SREG;
    cli();
    servos[i].detach();
    digitalWrite(Controller::pin(i), LOW);
    SREG = saved;
  }
} outputs;
Controller controller(outputs);
class EepromStore : public ByteStore {
public:
  uint8_t read(uint16_t a) override { return EEPROM.read(a); }
  void update(uint16_t a, uint8_t v) override { EEPROM.update(a, v); }
} store;
class Settings : public SettingsService {
public:
  Result save() override {
    return saveConfig(store, controller.config()) ? Result::Ok
                                                  : Result::Storage;
  }
  Result load() override {
    Config config;
    return loadConfig(store, config) ? controller.applyConfig(config)
                                     : Result::Storage;
  }
} settings;
class SerialSink : public TextSink {
public:
  void write(const char *text) override {
    // A full STATUS exceeds the UART buffer. Keep motion/watchdog serviced
    // while bytes drain rather than blocking the scheduler for the snapshot.
    while (*text) {
      controller.tick(millis());
      Serial.write(static_cast<uint8_t>(*text++));
    }
  }
} sink;
Protocol protocol(controller, sink, settings);
void setup() {
  outputs.begin();
  Serial.begin(115200);
  controller.begin(millis());
  settings.load();
  Serial.println(F("BOOT SPYDER 2"));
}
void loop() {
  controller.tick(millis());
  for (uint8_t count = 0; count < 32 && Serial.available(); ++count)
    protocol.feed(char(Serial.read()), millis());
  controller.tick(millis());
}

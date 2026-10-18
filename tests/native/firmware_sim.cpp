// Host-only serial simulator: the real core/protocol/storage, no Arduino or
// motors.
#include "Control.h"
#include "Protocol.h"
#include "Storage.h"
#include <chrono>
#include <cstring>
#include <iostream>
using namespace spyder;
struct Output : Outputs {
  void attach(uint8_t, uint8_t, uint16_t) override {}
  void write(uint8_t, uint16_t) override {}
  void detach(uint8_t) override {}
};
struct Memory : ByteStore {
  uint8_t data[1024];
  Memory() { memset(data, 255, sizeof data); }
  uint8_t read(uint16_t a) override { return data[a]; }
  void update(uint16_t a, uint8_t v) override { data[a] = v; }
};
struct Sink : TextSink {
  void write(const char *text) override { std::cout << text << std::flush; }
};
struct Settings : SettingsService {
  Controller &c;
  Memory memory;
  explicit Settings(Controller &controller) : c(controller) {}
  Result save() override {
    return saveConfig(memory, c.config()) ? Result::Ok : Result::Storage;
  }
  Result load() override {
    Config config;
    return loadConfig(memory, config) ? c.applyConfig(config) : Result::Storage;
  }
};
int main() {
  Output output;
  Controller controller(output);
  controller.begin(0);
  Sink sink;
  Settings settings(controller);
  Protocol protocol(controller, sink, settings);
  const auto start = std::chrono::steady_clock::now();
  char ch;
  while (std::cin.get(ch)) {
    uint32_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - start)
                       .count();
    controller.tick(now);
    protocol.feed(ch, now);
  }
}

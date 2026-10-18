#include "Storage.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
using namespace spyder;
struct O : Outputs {
  void attach(uint8_t, uint8_t, uint16_t) override {}
  void write(uint8_t, uint16_t) override {}
  void detach(uint8_t) override {}
};
struct Memory : ByteStore {
  uint8_t bytes[1024];
  int writes = 0, fail = -1;
  Memory() { memset(bytes, 255, sizeof bytes); }
  uint8_t read(uint16_t a) override {
    assert(a < 1024);
    return bytes[a];
  }
  void update(uint16_t a, uint8_t v) override {
    assert(a < 1024);
    if (writes++ == fail)
      throw std::runtime_error("power loss");
    bytes[a] = v;
  }
};
int main() {
  O o;
  Controller c(o);
  c.begin(0);
  Config cfg = c.config(), loaded = {};
  Memory m;
  assert(!loadConfig(m, loaded));
  for (int i = 0; i < 12; ++i) {
    cfg.joints[i].calibrated = true;
    cfg.joints[i].direction = i % 2 ? -1 : 1;
    cfg.standing[i] = 1510;
  }
  cfg.standingValid = true;
  assert(saveConfig(m, cfg));
  assert(loadConfig(m, loaded));
  assert(loaded.standingValid && loaded.standing[11] == 1510 &&
         loaded.joints[1].direction == -1);
  assert(c.applyConfig(loaded) == Result::Ok && c.allDetached());
  const int count = m.writes;
  for (int stop = 0; stop < count; ++stop) {
    Memory interrupted;
    interrupted.fail = stop;
    try {
      saveConfig(interrupted, cfg);
    } catch (const std::runtime_error &) {
    }
    assert(!loadConfig(interrupted, loaded));
  }
  for (int index = 0; index < StorageBytes; ++index) {
    Memory corrupt = m;
    corrupt.bytes[index] ^= 1;
    assert(!loadConfig(corrupt, loaded));
  }
  // Independent fixture: Python binascii.crc_hqx over version/flags/payload.
  // Valid CRC must not bypass semantic validation of a 2100-us maximum.
  Memory semantic = m;
  semantic.bytes[93] = 0x34;
  semantic.bytes[94] = 0x08;
  semantic.bytes[123] = 0x48;
  semantic.bytes[124] = 0x43;
  assert(!loadConfig(semantic, loaded));
  Config next = cfg;
  next.joints[0].direction = -1;
  for (int stop = 0; stop < count; ++stop) {
    Memory overwrite = m;
    overwrite.writes = 0;
    overwrite.fail = stop;
    try {
      saveConfig(overwrite, next);
    } catch (const std::runtime_error &) {
    }
    if (loadConfig(overwrite, loaded)) {
      assert(stop == 0 && loaded.joints[0].direction == 1);
    }
  }
  Config bad = cfg;
  bad.joints[11].maximum = 2100;
  assert(!saveConfig(m, bad));
  assert(loadConfig(m, loaded) && loaded.joints[11].maximum == 1600);
  std::printf("storage: roundtrip, erased/corrupt/schema, bounds, %d "
              "interrupted boundaries on erased and existing records PASS\n",
              count);
}

#pragma once
#include "Control.h"
namespace spyder {
const uint16_t StorageBytes = 126;
class ByteStore {
public:
  virtual uint8_t read(uint16_t address) = 0;
  virtual void update(uint16_t address, uint8_t value) = 0;
};
bool saveConfig(ByteStore &store, const Config &config);
bool loadConfig(ByteStore &store, Config &config);
} // namespace spyder

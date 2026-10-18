#include "Storage.h"
namespace spyder {
// marker(1), version(1), flags(1), joints(12*8), pose(24), checksum(2),
// reserved(1).
static uint16_t checksum(const uint8_t *data) {
  uint16_t crc = 0xffff;
  for (uint16_t i = 1; i < 123; ++i) {
    crc ^= uint16_t(data[i]) << 8;
    for (uint8_t b = 0; b < 8; ++b)
      crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
  }
  return crc;
}
static void put(uint8_t *b, uint16_t &n, uint16_t v) {
  b[n++] = v & 255;
  b[n++] = v >> 8;
}
static uint16_t get(const uint8_t *b, uint16_t &n) {
  uint16_t v = b[n++];
  return v | (uint16_t(b[n++]) << 8);
}
bool saveConfig(ByteStore &s, const Config &c) {
  if (!validConfig(c))
    return false;
  uint8_t b[StorageBytes] = {};
  b[0] = 0xa5;
  b[1] = 1;
  b[2] = c.standingValid;
  uint16_t n = 3;
  for (uint8_t i = 0; i < 12; ++i) {
    const auto &j = c.joints[i];
    put(b, n, j.minimum);
    put(b, n, j.maximum);
    put(b, n, j.initial);
    b[n++] = j.direction == 1 ? 1 : 0;
    b[n++] = j.calibrated;
  }
  for (uint8_t i = 0; i < 12; ++i)
    put(b, n, c.standing[i]);
  const uint16_t crc = checksum(b);
  b[123] = crc & 255;
  b[124] = crc >> 8;
  b[125] = 0x5a;
  s.update(0, 0);
  for (uint16_t i = 1; i < StorageBytes; ++i)
    s.update(i, b[i]);
  s.update(0, b[0]);
  return true;
}
bool loadConfig(ByteStore &s, Config &c) {
  uint8_t b[StorageBytes];
  for (uint16_t i = 0; i < StorageBytes; ++i)
    b[i] = s.read(i);
  if (b[0] != 0xa5 || b[1] != 1 || b[2] > 1 || b[125] != 0x5a ||
      checksum(b) != (uint16_t(b[123]) | (uint16_t(b[124]) << 8)))
    return false;
  Config candidate;
  candidate.standingValid = b[2];
  uint16_t n = 3;
  for (uint8_t i = 0; i < 12; ++i) {
    auto &j = candidate.joints[i];
    j.minimum = get(b, n);
    j.maximum = get(b, n);
    j.initial = get(b, n);
    if (b[n] > 1 || b[n + 1] > 1)
      return false;
    j.direction = b[n++] ? 1 : -1;
    j.calibrated = b[n++];
  }
  for (uint8_t i = 0; i < 12; ++i)
    candidate.standing[i] = get(b, n);
  if (!validConfig(candidate))
    return false;
  c = candidate;
  return true;
}
} // namespace spyder

#pragma once
#include <vector>
#include <cstring>
extern std::vector<uint8_t> savedNvs;
extern bool failNvs;
class Preferences {
public:
 bool begin(const char *,bool) { return !failNvs; }
 void end() {}
 size_t getBytesLength(const char *) { return savedNvs.size(); }
 size_t getBytes(const char *,void *data,size_t size) {
  if(size<savedNvs.size()) return 0;
  memcpy(data,savedNvs.data(),savedNvs.size()); return savedNvs.size();
 }
 size_t putBytes(const char *,const void *data,size_t size) {
  if(failNvs) return 0;
  auto first=static_cast<const uint8_t *>(data); savedNvs.assign(first,first+size); return size;
 }
};

#include "NvSettings.h"
#include <Preferences.h>
namespace legtest {
bool loadCalibration(Settings &settings) {
  Preferences preferences;
  if(!preferences.begin("spyder-leg",true)) return false;
  uint8_t bytes[SavedSize];
  bool valid=preferences.getBytesLength("cal")==SavedSize &&
    preferences.getBytes("cal",bytes,sizeof(bytes))==sizeof(bytes);
  preferences.end();
  return valid && decodeSettings(bytes,settings);
}
bool saveCalibration(const Settings &settings) {
  uint8_t bytes[SavedSize];
  if(!encodeSettings(settings,bytes)) return false;
  Preferences preferences;
  if(!preferences.begin("spyder-leg",false)) return false;
  bool saved=preferences.putBytes("cal",bytes,sizeof(bytes))==sizeof(bytes);
  preferences.end(); return saved;
}
}

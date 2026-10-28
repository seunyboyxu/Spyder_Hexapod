#pragma once
#include "Arduino.h"
enum esp_spp_cb_event_t { ESP_SPP_CLOSE_EVT, ESP_SPP_SRV_OPEN_EVT };
struct esp_spp_cb_param_t {};
class BluetoothSerial : public FakeSerial {
public:
 bool connected=false;
 std::function<void(esp_spp_cb_event_t,esp_spp_cb_param_t *)> eventCb;
 std::function<void(const uint8_t *,size_t)> dataCb;
 bool begin(const char *) { return true; }
 bool hasClient() { return connected; }
 void register_callback(std::function<void(esp_spp_cb_event_t,esp_spp_cb_param_t *)> cb) { eventCb=cb; }
 void onData(std::function<void(const uint8_t *,size_t)> cb) { dataCb=cb; }
 void receivePacket(const std::string &s) {
  if(dataCb) { dataCb(reinterpret_cast<const uint8_t *>(s.data()),s.size()); return; }
  // Characterize the pinned core's default RX queue: excess is silently lost.
  for(uint8_t c:s) { if(rx.size()==512) break; rx.push_back(c); }
 }
};

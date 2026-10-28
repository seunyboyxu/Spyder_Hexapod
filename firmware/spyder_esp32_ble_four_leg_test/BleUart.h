#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLE2902.h>
#include <atomic>
#include <functional>
#include <freertos/semphr.h>

// Nordic UART Service. Commands are ASCII lines; notifications are <=20 bytes.
class BleUart : private BLEServerCallbacks, private BLECharacteristicCallbacks, private BLEDescriptorCallbacks {
public:
  using DataCallback=std::function<void(const uint8_t *,size_t)>;
  using EventCallback=std::function<void()>;
  ~BleUart() override;
  bool begin(const char *name,DataCallback data,EventCallback up,EventCallback down);
  bool hasClient() const { return connected_.load(); }
  bool ready() const { return connected_.load() && subscribed_.load(); }
  uint32_t epoch() const { return epoch_.load(); }
  bool takeError() { return txError_.exchange(false); }
  size_t write(const uint8_t *data,size_t size,uint32_t expectedEpoch);
  void poll();
private:
  SemaphoreHandle_t txMutex_=nullptr;
  BLEServer *server_=nullptr;
  BLECharacteristic *tx_=nullptr,*rx_=nullptr;
  BLE2902 *cccd_=nullptr;
  DataCallback data_;
  EventCallback up_,down_;
  std::atomic<bool> connected_{false},subscribed_{false},restart_{false},notifyOk_{false},txError_{false};
  std::atomic<uint32_t> epoch_{0};
  void onConnect(BLEServer *server) override;
  void onDisconnect(BLEServer *server) override;
  void onWrite(BLECharacteristic *characteristic) override;
  void onWrite(BLEDescriptor *descriptor) override;
  void onStatus(BLECharacteristic *characteristic,Status status,uint32_t code) override;
};

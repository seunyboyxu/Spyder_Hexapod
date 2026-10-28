#include "BleUart.h"
static const char *ServiceUuid="6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *RxUuid="6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *TxUuid="6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

namespace {
class TxLock {
  SemaphoreHandle_t mutex_;
  bool locked_;
public:
  explicit TxLock(SemaphoreHandle_t mutex,uint32_t timeout=portMAX_DELAY):mutex_(mutex),locked_(mutex && xSemaphoreTake(mutex,timeout)==pdTRUE) {}
  ~TxLock() { if(locked_) xSemaphoreGive(mutex_); }
  bool acquired() const { return locked_; }
};
}
BleUart::~BleUart() { if(txMutex_) vSemaphoreDelete(txMutex_); }
bool BleUart::begin(const char *name,DataCallback data,EventCallback up,EventCallback down) {
  if(txMutex_) return false;
  txMutex_=xSemaphoreCreateMutex();
  if(!txMutex_) return false;
  data_=data; up_=up; down_=down;
  BLEDevice::init(name);
  if(!BLEDevice::getInitialized()) return false;
  server_=BLEDevice::createServer();
  if(!server_) return false;
  server_->setCallbacks(this);
  BLEService *service=server_->createService(ServiceUuid);
  if(!service) return false;
  tx_=service->createCharacteristic(TxUuid,BLECharacteristic::PROPERTY_NOTIFY|BLECharacteristic::PROPERTY_READ);
  rx_=service->createCharacteristic(RxUuid,BLECharacteristic::PROPERTY_WRITE|BLECharacteristic::PROPERTY_WRITE_NR);
  if(!tx_ || !rx_) return false;
  cccd_=new BLE2902();
  cccd_->setCallbacks(this); tx_->addDescriptor(cccd_);
  tx_->setCallbacks(this); rx_->setCallbacks(this);
  service->start();
  server_->getAdvertising()->addServiceUUID(ServiceUuid);
  server_->getAdvertising()->setScanResponse(true);
  server_->getAdvertising()->start();
  return true;
}
void BleUart::onConnect(BLEServer *server) {
  (void)server;
  // No client can receive replies until its later subscription commits.
  cccd_->setNotifications(false);
  subscribed_.store(false); epoch_.fetch_add(1); connected_.store(true);
  if(up_) up_();
}
void BleUart::onDisconnect(BLEServer *server) {
  (void)server;
  connected_.store(false); subscribed_.store(false); epoch_.fetch_add(1);
  if(down_) down_(); // Inhibit OE before any advertising/descriptor work.
  // Wait for any in-progress dispatch before allowing a new client session.
  cccd_->setNotifications(false);
  TxLock lock(txMutex_,pdMS_TO_TICKS(5));
  restart_.store(true);
}
void BleUart::onWrite(BLECharacteristic *characteristic) {
  if(characteristic!=rx_ || !hasClient()) return;
  const auto value=characteristic->getValue();
  if(data_ && value.length()) data_(reinterpret_cast<const uint8_t *>(value.c_str()),value.length());
}
void BleUart::onWrite(BLEDescriptor *descriptor) {
  if(descriptor!=cccd_ || !hasClient()) return;
  bool enabled=descriptor->getLength()==2 && (descriptor->getValue()[0]&1)!=0;
  if(!enabled) {
    if(subscribed_.exchange(false)) {
      epoch_.fetch_add(1);
      if(down_) down_(); // Inhibit OE before waiting on reply dispatch.
    }
    TxLock barrier(txMutex_,pdMS_TO_TICKS(5)); // Never stall the BLE event task indefinitely.
    return;
  }
  bool announce=false;
  {
    TxLock lock(txMutex_,pdMS_TO_TICKS(5));
    if(!lock.acquired()) {
      // A stuck SDK send may need this callback task to drain its own queue.
      // Fail closed rather than deadlocking; the app can retry subscription.
      subscribed_.store(false); epoch_.fetch_add(1); cccd_->setNotifications(false);
      if(down_) down_();
      txError_.store(true); return;
    }
    if(!hasClient()) return;
    if(!subscribed_.load()) {
      epoch_.fetch_add(1); subscribed_.store(true); announce=true;
    }
  }
  if(announce && up_) up_();
}
void BleUart::onStatus(BLECharacteristic *characteristic,Status status,uint32_t code) {
  (void)code;
  if(characteristic==tx_) notifyOk_.store(status==BLECharacteristicCallbacks::SUCCESS_NOTIFY);
}
size_t BleUart::write(const uint8_t *data,size_t size,uint32_t expectedEpoch) {
  const size_t count=size>20?20:size;
  {
    TxLock lock(txMutex_);
    if(!lock.acquired() || !ready() || epoch()!=expectedEpoch || !count) return 0;
    tx_->setValue(const_cast<uint8_t *>(data),count);
    notifyOk_.store(false); tx_->notify();
    if(!notifyOk_.load()) { if(ready()) txError_.store(true); return 0; }
  }
  // Pace accepted notifications after releasing the session guard.
  vTaskDelay(pdMS_TO_TICKS(10));
  return count;
}

void BleUart::poll() {
  if(restart_.exchange(false) && !hasClient()) server_->getAdvertising()->start();
}

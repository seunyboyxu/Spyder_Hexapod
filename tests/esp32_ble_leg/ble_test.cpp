#include "BleUart.h"
#include <cassert>
#include <string>
#include <cstdio>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <freertos/semphr.h>
int main() {
 BleUart uart; std::string received; std::atomic<int> up{0},down{0};
 assert(uart.begin("Spyder-Leg-BLE",[&](const uint8_t *data,size_t n){received.append(reinterpret_cast<const char *>(data),n);},[&]{++up;},[&]{++down;}));
 auto &model=blefake::model();
 assert(model.name=="Spyder-Leg-BLE" && model.advertising.active);
 auto service=model.server.getService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E"); assert(service);
 auto rx=service->getCharacteristic("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
 auto tx=service->getCharacteristic("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
 assert(rx && tx && (rx->properties&BLECharacteristic::PROPERTY_WRITE));
 assert(rx->properties&BLECharacteristic::PROPERTY_WRITE_NR);
 assert(tx->properties&BLECharacteristic::PROPERTY_NOTIFY);
 model.server.clientConnect(); assert(uart.hasClient() && !uart.ready());
 const uint8_t message[]="abcdefghijklmnopqrstuvwxyz";
 assert(uart.write(message,26,uart.epoch())==0 && model.notifications.empty());
 tx->clientSubscribe(true); assert(uart.ready());
 rx->clientWrite("cal "); rx->clientWrite("0\n"); assert(received=="cal 0\n");
 auto epoch=uart.epoch(); assert(uart.write(message,26,epoch)==20);
 assert(model.notifications.size()==1 && model.notifications[0]=="abcdefghijklmnopqrst");
 tx->clientSubscribe(false); assert(!uart.ready() && down==1);
 tx->clientSubscribe(true); assert(uart.ready());
 model.server.clientDisconnect(); assert(!uart.hasClient() && down==2);
 uart.poll(); assert(model.advertising.active);
 model.server.clientConnect(); tx->clientSubscribe(true);
 assert(uart.write(message,26,epoch)==0); // No stale replies in a new session.
 assert(uart.write(message,26,uart.epoch())==20);
 model.notifyFail=true;
 assert(uart.write(message,26,uart.epoch())==0 && uart.takeError());
 assert(!uart.takeError());
 assert(up>=2);
 // Pause dispatch after its guard, then switch notification subscriptions.
 // With synchronization, unsubscribe must inhibit first and wait for dispatch;
 // without it the switch completes and the old chunk reaches a new epoch.
 model.notifyFail=false; model.notifications.clear();
 std::mutex mutex; std::condition_variable condition;
 bool paused=false,resume=false,switched=false,waiting=false;
 tx->afterSetValue=[&] {
  std::unique_lock<std::mutex> lock(mutex); paused=true; condition.notify_all();
  condition.wait(lock,[&]{return resume;});
 };
 semfake::onWait()=[&] { std::lock_guard<std::mutex> lock(mutex); waiting=true; condition.notify_all(); };
 auto oldEpoch=uart.epoch();
 std::thread sender([&] { uart.write(message,26,oldEpoch); });
 { std::unique_lock<std::mutex> lock(mutex); condition.wait(lock,[&]{return paused;}); }
 std::thread switcher([&] {
  tx->clientSubscribe(false); tx->clientSubscribe(true);
  std::lock_guard<std::mutex> lock(mutex); switched=true; condition.notify_all();
 });
 { std::unique_lock<std::mutex> lock(mutex);
   condition.wait(lock,[&]{return switched || waiting;}); resume=true; condition.notify_all(); }
 sender.join(); switcher.join(); tx->afterSetValue=nullptr; semfake::onWait()=nullptr;
 assert(model.notifications.empty());
 if(!uart.ready()) tx->clientSubscribe(true);
 assert(uart.ready());
 puts("PASS concurrent reply dispatch cannot cross a notification subscription epoch");
 // A congested SDK send must not deadlock the Bluetooth callback task.
 paused=false; resume=false; switched=false;
 tx->afterSetValue=[&] {
  std::unique_lock<std::mutex> lock(mutex); paused=true; condition.notify_all();
  condition.wait(lock,[&]{return resume;});
 };
 std::thread blockedSender([&] { uart.write(message,26,uart.epoch()); });
 { std::unique_lock<std::mutex> lock(mutex); condition.wait(lock,[&]{return paused;}); }
 std::thread boundedSwitcher([&] {
  tx->clientSubscribe(false); tx->clientSubscribe(true);
  std::lock_guard<std::mutex> lock(mutex); switched=true; condition.notify_all();
 });
 { std::unique_lock<std::mutex> lock(mutex);
   assert(condition.wait_for(lock,std::chrono::seconds(1),[&]{return switched;}));
   resume=true; condition.notify_all(); }
 blockedSender.join(); boundedSwitcher.join(); tx->afterSetValue=nullptr;
 assert(!uart.ready() && model.notifications.empty());
 tx->clientSubscribe(true); assert(uart.ready());
 puts("PASS bounded session callbacks release safely while notification dispatch is blocked");
 puts("PASS BLE NUS contract, subscription, RX fragments, notification chunk size, sessions and advertising");
}

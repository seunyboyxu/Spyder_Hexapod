#pragma once
#include "Arduino.h"
#include <string>
#include <map>
#include <memory>
class BLEServer; class BLECharacteristic; class BLEDescriptor;
class BLEServerCallbacks { public: virtual ~BLEServerCallbacks(){}; virtual void onConnect(BLEServer*){} virtual void onDisconnect(BLEServer*){} };
class BLECharacteristicCallbacks {
public:
 enum Status { SUCCESS_NOTIFY, ERROR_GATT };
 virtual ~BLECharacteristicCallbacks(){};
 virtual void onWrite(BLECharacteristic*){}
 virtual void onStatus(BLECharacteristic*,Status,uint32_t){}
};
class BLEDescriptorCallbacks { public: virtual ~BLEDescriptorCallbacks(){}; virtual void onWrite(BLEDescriptor*){} };
class BLEDescriptor {
public:
 uint8_t value[2]={0,0}; BLEDescriptorCallbacks *callback=nullptr;
 virtual ~BLEDescriptor(){}
 void setCallbacks(BLEDescriptorCallbacks *cb) { callback=cb; }
 size_t getLength() { return 2; }
 uint8_t *getValue() { return value; }
};
class BLE2902 : public BLEDescriptor {
public:
 void setNotifications(bool enabled) { value[0]=enabled?1:0; }
 bool getNotifications() { return (value[0]&1)!=0; }
};
class BLECharacteristic {
public:
 enum { PROPERTY_NOTIFY=1, PROPERTY_WRITE=2, PROPERTY_WRITE_NR=4, PROPERTY_READ=8 };
 unsigned properties; std::string value; std::function<void()> afterSetValue; BLECharacteristicCallbacks *callback=nullptr;
 std::unique_ptr<BLEDescriptor> descriptor;
 explicit BLECharacteristic(unsigned p):properties(p){}
 void setCallbacks(BLECharacteristicCallbacks *cb) { callback=cb; }
 void addDescriptor(BLEDescriptor *p) { descriptor.reset(p); }
 std::string getValue() { return value; }
 void setValue(uint8_t *p,size_t n) { value.assign(reinterpret_cast<char *>(p),n); if(afterSetValue) afterSetValue(); }
 void notify();
 void clientWrite(const std::string &s) { value=s; if(callback) callback->onWrite(this); }
 void clientSubscribe(bool enabled) { descriptor->value[0]=enabled?1:0; if(descriptor->callback) descriptor->callback->onWrite(descriptor.get()); }
};
class BLEService {
public:
 std::map<std::string,std::unique_ptr<BLECharacteristic>> chars;
 BLECharacteristic *createCharacteristic(const char *id,unsigned p) { chars[id].reset(new BLECharacteristic(p)); return chars[id].get(); }
 BLECharacteristic *getCharacteristic(const char *id) { return chars.count(id)?chars[id].get():nullptr; }
 void start(){}
};
class BLEAdvertising {
public:
 bool active=false, scan=false; std::string uuid;
 void addServiceUUID(const char *s) { uuid=s; }
 void setScanResponse(bool value) { scan=value; }
 void start() { active=true; }
};
namespace blefake { struct Model; Model &model(); }
class BLEServer {
public:
 BLEServerCallbacks *callback=nullptr; std::map<std::string,std::unique_ptr<BLEService>> services;
 void setCallbacks(BLEServerCallbacks *cb) { callback=cb; }
 BLEService *createService(const char *id) { services[id].reset(new BLEService); return services[id].get(); }
 BLEService *getService(const char *id) { return services.count(id)?services[id].get():nullptr; }
 BLEAdvertising *getAdvertising();
 void clientConnect(); void clientDisconnect();
};
namespace blefake {
struct Model { std::string name; bool initialized=false, notifyFail=false; BLEServer server; BLEAdvertising advertising; std::vector<std::string> notifications; };
inline Model &model() { static Model state; return state; }
}
inline BLEAdvertising *BLEServer::getAdvertising() { return &blefake::model().advertising; }
inline void BLEServer::clientConnect() { blefake::model().advertising.active=false; callback->onConnect(this); }
inline void BLEServer::clientDisconnect() { callback->onDisconnect(this); }
inline void BLECharacteristic::notify() {
 bool fail=blefake::model().notifyFail || !descriptor || !(descriptor->value[0]&1);
 if(!fail) blefake::model().notifications.push_back(value);
 if(callback) callback->onStatus(this,fail?BLECharacteristicCallbacks::ERROR_GATT:BLECharacteristicCallbacks::SUCCESS_NOTIFY,0);
}
class BLEDevice {
public:
 static void init(const char *name) { blefake::model().name=name; blefake::model().initialized=true; }
 static bool getInitialized() { return blefake::model().initialized; }
 static BLEServer *createServer() { return &blefake::model().server; }
};

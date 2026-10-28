#pragma once
#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <deque>
#include <functional>
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define CONFIG_IDF_TARGET_ESP32 1
#define CONFIG_BT_ENABLED 1
#define CONFIG_BLUEDROID_ENABLED 1
#define CONFIG_BT_SPP_ENABLED 1
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
extern int oe;
extern std::vector<int> levels;
extern uint32_t fakeNow;
inline void digitalWrite(int pin,int level) { if(pin==23) { oe=level; levels.push_back(level); } }
inline void pinMode(int,int) {}
inline void delay(int ms) { fakeNow+=ms; }
inline uint32_t millis() { return fakeNow; }
using hardwareSerial_error_t=int;
class FakeSerial {
public:
 std::deque<uint8_t> rx;
 unsigned written=0;
 std::function<void(hardwareSerial_error_t)> errorCb;
 void begin(int) {}
 void onReceiveError(std::function<void(hardwareSerial_error_t)> cb) { errorCb=cb; }
 int available() { return int(rx.size()); }
 int read() { if(rx.empty()) return -1; int c=rx.front(); rx.pop_front(); return c; }
 int availableForWrite() { return 128; }
 size_t write(const uint8_t *,size_t n) { written+=n; return n; }
 void receive(const char *s) { for(;*s;++s) rx.push_back(uint8_t(*s)); }
};
extern FakeSerial Serial;
struct FakeQueue { size_t capacity, width; std::deque<std::vector<uint8_t>> messages; };
using QueueHandle_t=FakeQueue *;
#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
#define portMAX_DELAY 0xffffffffU
#define pdMS_TO_TICKS(x) (x)
inline QueueHandle_t xQueueCreate(unsigned count,unsigned width) { return new FakeQueue{count,width,{}}; }
inline int xQueueSend(QueueHandle_t q,const void *data,unsigned) {
 if(q->messages.size()==q->capacity) return pdFALSE;
 const auto p=static_cast<const uint8_t *>(data); q->messages.emplace_back(p,p+q->width); return pdTRUE;
}
inline int xQueueReceive(QueueHandle_t q,void *data,unsigned) {
 if(q->messages.empty()) return pdFALSE;
 memcpy(data,q->messages.front().data(),q->width); q->messages.pop_front(); return pdTRUE;
}
inline unsigned uxQueueSpacesAvailable(QueueHandle_t q) { return q->capacity-q->messages.size(); }
inline unsigned uxQueueMessagesWaiting(QueueHandle_t q) { return q->messages.size(); }
inline int xTaskCreate(void (*)(void *),const char *,unsigned,void *,unsigned,void *) { return pdPASS; }
inline void vTaskDelay(unsigned) {}

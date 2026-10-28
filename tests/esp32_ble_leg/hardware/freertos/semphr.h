#pragma once
#include "Arduino.h"
#include <mutex>
#include <chrono>
namespace semfake { inline std::function<void()> &onWait() { static std::function<void()> hook; return hook; } }
struct FakeSemaphore { std::timed_mutex mutex; };
using SemaphoreHandle_t=FakeSemaphore *;
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return new FakeSemaphore; }
inline int xSemaphoreTake(SemaphoreHandle_t semaphore,unsigned timeout) {
 if(!semaphore->mutex.try_lock()) {
  if(semfake::onWait()) semfake::onWait()();
  if(timeout==portMAX_DELAY) semaphore->mutex.lock();
  else if(!semaphore->mutex.try_lock_for(std::chrono::milliseconds(timeout))) return pdFALSE;
 }
 return pdTRUE;
}
inline int xSemaphoreGive(SemaphoreHandle_t semaphore) { semaphore->mutex.unlock(); return pdTRUE; }
inline void vSemaphoreDelete(SemaphoreHandle_t semaphore) { delete semaphore; }

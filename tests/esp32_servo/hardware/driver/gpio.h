#pragma once
#include "Arduino.h"
#define GPIO_NUM_23 23
inline void gpio_set_level(int pin,int level) { digitalWrite(pin,level); }

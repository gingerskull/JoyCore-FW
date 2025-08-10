#pragma once
#include <Arduino.h>

#ifndef CONFIG_DEBUG
#define CONFIG_DEBUG 0
#endif

#if CONFIG_DEBUG
  #define DEBUG_PRINT(...)    Serial.print(__VA_ARGS__)
  #define DEBUG_PRINTLN(...)  Serial.println(__VA_ARGS__)
  #define DEBUG_PRINTF(fmt, ...) do { char __buf[128]; snprintf(__buf,sizeof(__buf),fmt,__VA_ARGS__); Serial.print(__buf);} while(0)
#else
  #define DEBUG_PRINT(...)    do{}while(0)
  #define DEBUG_PRINTLN(...)  do{}while(0)
  #define DEBUG_PRINTF(...)   do{}while(0)
#endif

#ifndef DEBUG_SERIAL_H
#define DEBUG_SERIAL_H

#include <Arduino.h>

//Set this to 1 while debugging with Serial Monitor
//Set this to 0 for the final video - I actually don't know if campisi want us to use only C or not so this is able disable 
#define ENABLE_SERIAL_DEBUG 1

#if ENABLE_SERIAL_DEBUG

static inline void debugSerialInit() {
  Serial.begin(9600);
  delay(1000);
}

#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)

#else

static inline void debugSerialInit() {
  //Serial disabled
}

#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)

#endif

#endif

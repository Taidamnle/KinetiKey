#ifndef DELAY_UTILS_H
#define DELAY_UTILS_H

#include <stdint.h>
#include <util/delay.h>

//delay function 
static inline void delayMs(uint16_t ms) {
  while (ms > 0) {
    _delay_ms(1);
    ms--;
  }
}

#endif

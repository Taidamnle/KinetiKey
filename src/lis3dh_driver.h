#ifndef LIS3DH_DRIVER_H
#define LIS3DH_DRIVER_H

#include <stdint.h>
#include <avr/io.h>

//Set up AVR SPI and LIS3DH
void lis3dhInit();

//Read xyz acceleration from LIS3DH
void lis3dhReadAccel(int16_t *x, int16_t *y, int16_t *z);

//Read one LIS3DH register
uint8_t lis3dhReadRegister(uint8_t reg);

//Write one LIS3DH register
void lis3dhWriteRegister(uint8_t reg, uint8_t value);

#endif

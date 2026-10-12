#include "lis3dh_driver.h"

//SPI command bits for the LIS3DH
#define READ_REG 0x80
#define INC_REG  0x40

//LIS3DH register addresses
#define CFG1_REG 0x20
#define AX_REG   0x28

//D8 maps to PB4 on ATmega32U4
#define LIS3DH_CS_DDR  DDRB
#define LIS3DH_CS_PORT PORTB
#define LIS3DH_CS_PIN  PB4

//ATmega32U4 SPI pins
#define SPI_DDR  DDRB
#define SPI_PORT PORTB
#define SPI_SS   PB0
#define SPI_SCK  PB1
#define SPI_MOSI PB2
#define SPI_MISO PB3


//Pull CS low to start talking to the sensor
static void lis3dhSelect() {
  LIS3DH_CS_PORT &= ~(1 << LIS3DH_CS_PIN);
}


//Pull CS high when we are done
static void lis3dhDeselect() {
  LIS3DH_CS_PORT |= (1 << LIS3DH_CS_PIN);
}


//Set up AVR SPI by hand
static void spiInit() {
  //MOSI, SCK, SS, and CS are outputs
  SPI_DDR |= (1 << SPI_MOSI);
  SPI_DDR |= (1 << SPI_SCK);
  SPI_DDR |= (1 << SPI_SS);
  LIS3DH_CS_DDR |= (1 << LIS3DH_CS_PIN);

  //MISO is input
  SPI_DDR &= ~(1 << SPI_MISO);

  //Keep SS high so the ATmega32U4 stays in master mode
  SPI_PORT |= (1 << SPI_SS);

  //Keep sensor CS high when not talking to it
  lis3dhDeselect();

  //Enable SPI in master mode
  SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);

  //No double speed
  SPSR = 0;
}


//Send one byte over SPI and get one byte back
static uint8_t spiTransfer(uint8_t data) {
  SPDR = data;

  while ((SPSR & (1 << SPIF)) == 0) {
    //Wait
  }

  return SPDR;
}


//Write one byte to a LIS3DH register
void lis3dhWriteRegister(uint8_t reg, uint8_t value) {
  lis3dhSelect();

  spiTransfer(reg);
  spiTransfer(value);

  lis3dhDeselect();
}


//Read one byte from a LIS3DH register
uint8_t lis3dhReadRegister(uint8_t reg) {
  uint8_t value = 0;

  lis3dhSelect();

  spiTransfer(reg | READ_REG);
  value = spiTransfer(0xFF);

  lis3dhDeselect();

  return value;
}


//Read xyz acceleration
void lis3dhReadAccel(int16_t *x, int16_t *y, int16_t *z) {
  uint8_t xLow = 0;
  uint8_t xHigh = 0;
  uint8_t yLow = 0;
  uint8_t yHigh = 0;
  uint8_t zLow = 0;
  uint8_t zHigh = 0;

  lis3dhSelect();

  //Start at oputxl  and auto-increment through xyz
  spiTransfer(AX_REG | READ_REG | INC_REG);

  xLow  = spiTransfer(0xFF);
  xHigh = spiTransfer(0xFF);

  yLow  = spiTransfer(0xFF);
  yHigh = spiTransfer(0xFF);

  zLow  = spiTransfer(0xFF);
  zHigh = spiTransfer(0xFF);

  lis3dhDeselect();

  *x = (int16_t)((xHigh << 8) | xLow);
  *y = (int16_t)((yHigh << 8) | yLow);
  *z = (int16_t)((zHigh << 8) | zLow);
}


//Set up SPI and configure LIS3DH
void lis3dhInit() {
  spiInit();

  //CTRL_REG1 = 0x20
  //0b01000111 means
  //0100 50 Hz data rate
  //0 normal power mode
  //1 Z enabled
  //1 Y enabled
  //1 X enabled
  lis3dhWriteRegister(CFG1_REG, 0b01000111);
}

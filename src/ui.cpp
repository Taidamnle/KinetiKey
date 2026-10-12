#include "ui.h"

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <avr/io.h>

#include "delay_utils.h"

//Onboard Circuit Playground NeoPixels
#define NEOPIXEL_PIN 17
#define NUM_PIXELS 10

//Button A = D4  = PD4
//Button B = D19 = PF6
#define BUTTON_A_DDR     DDRD
#define BUTTON_A_PORT    PORTD
#define BUTTON_A_PINREG  PIND
#define BUTTON_A_BIT     PD4

#define BUTTON_B_DDR     DDRF
#define BUTTON_B_PORT    PORTF
#define BUTTON_B_PINREG  PINF
#define BUTTON_B_BIT     PF6

const uint16_t BUTTON_DEBOUNCE_MS = 50;
const uint8_t UI_KEY_LENGTH = 3;

//Color idea
//Record uses blue/cyan
//Unlock uses purple/green
//Errors use red
const uint8_t RECORD_COLOR_R = 0;
const uint8_t RECORD_COLOR_G = 0;
const uint8_t RECORD_COLOR_B = 35;

const uint8_t RECORD_DONE_R = 0;
const uint8_t RECORD_DONE_G = 25;
const uint8_t RECORD_DONE_B = 25;

const uint8_t UNLOCK_WAIT_R = 25;
const uint8_t UNLOCK_WAIT_G = 0;
const uint8_t UNLOCK_WAIT_B = 25;

const uint8_t GOOD_R = 0;
const uint8_t GOOD_G = 35;
const uint8_t GOOD_B = 0;

const uint8_t BAD_R = 40;
const uint8_t BAD_G = 0;
const uint8_t BAD_B = 0;

static Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);


//Set up Button A and Button B using direct registers
static void buttonsInit() {
  //Make them inputs
  BUTTON_A_DDR &= ~(1 << BUTTON_A_BIT);
  BUTTON_B_DDR &= ~(1 << BUTTON_B_BIT);

  //Buttons read HIGH when pressed
  //So do not turn on internal pullups
  BUTTON_A_PORT &= ~(1 << BUTTON_A_BIT);
  BUTTON_B_PORT &= ~(1 << BUTTON_B_BIT);
}


//Read Button A
static bool readButtonA() {
  return ((BUTTON_A_PINREG & (1 << BUTTON_A_BIT)) != 0);
}

//Read Button B
static bool readButtonB() {
  return ((BUTTON_B_PINREG & (1 << BUTTON_B_BIT)) != 0);
}

//Set up NeoPixels
static void neoInit() {
  pixels.begin();
  pixels.setBrightness(30);
  pixels.clear();
  pixels.show();
}

//Set up UI hardware
void uiInit() {
  buttonsInit();
  neoInit();
}

//Button A press with simple debounce
bool buttonAPressedEvent() {
  if (!readButtonA()) {
    return false;
  }

  delayMs(BUTTON_DEBOUNCE_MS);

  if (!readButtonA()) {
    return false;
  }

  while (readButtonA()) {
    //Wait for release
  }

  delayMs(BUTTON_DEBOUNCE_MS);

  return true;
}

//Button B press with simple debounce
bool buttonBPressedEvent() {
  if (!readButtonB()) {
    return false;
  }

  delayMs(BUTTON_DEBOUNCE_MS);

  if (!readButtonB()) {
    return false;
  }

  while (readButtonB()) {
    //Wait for release
  }

  delayMs(BUTTON_DEBOUNCE_MS);

  return true;
}

//Turn off all pixels
void neoClear() {
  pixels.clear();
  pixels.show();
}

//Set all pixels to one color
static void neoSetAll(uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < NUM_PIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }

  pixels.show();
}

//Light only one pixel
static void neoSetOne(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  pixels.clear();

  if (index < NUM_PIXELS) {
    pixels.setPixelColor(index, pixels.Color(r, g, b));
  }

  pixels.show();
}

//Blink the full ring
static void neoBlink(uint8_t r, uint8_t g, uint8_t b, uint8_t times, uint16_t onMs, uint16_t offMs) {
  for (uint8_t i = 0; i < times; i++) {
    neoSetAll(r, g, b);
    delayMs(onMs);

    neoClear();
    delayMs(offMs);
  }
}

//Startup animation
void showStartupAnimation() {
  neoSetOne(9, 0, 0, 30);
  delayMs(300);
  neoClear();
  delayMs(100);
  neoSetOne(9, 0, 0, 30);
  delayMs(300);
  neoClear();
}

//Record progress
//Done gestures stay cyan
//Current one is blue
void showRecordSlot(uint8_t currentIndex) {
  pixels.clear();

  for (uint8_t i = 0; i < UI_KEY_LENGTH; i++) {
    if (i < currentIndex) {
      pixels.setPixelColor(i, pixels.Color(RECORD_DONE_R, RECORD_DONE_G, RECORD_DONE_B));
    }
    else if (i == currentIndex) {
      pixels.setPixelColor(i, pixels.Color(RECORD_COLOR_R, RECORD_COLOR_G, RECORD_COLOR_B));
    }
  }

  pixels.show();
}

//Unlock progress
//Correct gestures stay green
//Current one is purple
void showUnlockSlot(uint8_t currentIndex) {
  pixels.clear();

  for (uint8_t i = 0; i < UI_KEY_LENGTH; i++) {
    if (i < currentIndex) {
      pixels.setPixelColor(i, pixels.Color(GOOD_R, GOOD_G, GOOD_B));
    }
    else if (i == currentIndex) {
      pixels.setPixelColor(i, pixels.Color(UNLOCK_WAIT_R, UNLOCK_WAIT_G, UNLOCK_WAIT_B));
    }
  }

  pixels.show();
}

//All record slots completed
void showRecordCompleteProgress() {
  pixels.clear();

  for (uint8_t i = 0; i < UI_KEY_LENGTH; i++) {
    pixels.setPixelColor(i, pixels.Color(RECORD_DONE_R, RECORD_DONE_G, RECORD_DONE_B));
  }

  pixels.show();
}

//All unlock slots completed
void showUnlockCompleteProgress() {
  pixels.clear();

  for (uint8_t i = 0; i < UI_KEY_LENGTH; i++) {
    pixels.setPixelColor(i, pixels.Color(GOOD_R, GOOD_G, GOOD_B));
  }

  pixels.show();
}

//Idle indicator
void showIdle(bool keyRecorded) {
  pixels.clear();

  if (keyRecorded) {
    //dim green means key saved
    pixels.setPixelColor(9, pixels.Color(0, 12, 0));
  } else {
    //dim blue means no key yet
    pixels.setPixelColor(9, pixels.Color(0, 0, 12));
  }

  pixels.show();
}


//Bad gesture red blink
void showGestureBad() {
  neoBlink(BAD_R, BAD_G, BAD_B, 3, 80, 80);
}


//Unlock failure animation
void showUnlockFail() {
  neoBlink(40, 0, 0, 6, 80, 60);
}


//Unlock success animation
void showUnlockSuccess() {
  for (uint8_t round = 0; round < 3; round++) {
    for (uint8_t i = 0; i < NUM_PIXELS; i++) {
      pixels.clear();
      pixels.setPixelColor(i, pixels.Color(0, 40, 0));
      pixels.show();
      delayMs(60);
    }
  }

  neoSetAll(0, 40, 0);
  delayMs(500);
  neoClear();
}


//Record complete animation
void showRecordComplete() {
  neoBlink(0, 0, 40, 4, 120, 80);
}

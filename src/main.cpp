#include <Arduino.h>

#include "debug_serial.h"
#include "gesture_lock.h"
#include "ui.h"

void setup() {
  //Debug only
  //Turn this on when using Serial Monitor
  //For final demo, open debug_serial.h and set: the line to 0
  debugSerialInit();

  uiInit();
  gestureLockInit();

  DEBUG_PRINTLN(F("KinetiKey started"));
  DEBUG_PRINTLN(F("Button B = record"));
  DEBUG_PRINTLN(F("Button A = unlock"));

  showStartupAnimation();
  showIdle(keyIsRecorded());
}

void loop() {
  //Button B starts recording
  if (buttonBPressedEvent()) {
    DEBUG_PRINTLN(F("Button B pressed: record mode"));
    recordKey();
    showIdle(keyIsRecorded());
  }

  //Button A starts unlock
  if (buttonAPressedEvent()) {
    DEBUG_PRINTLN(F("Button A pressed: unlock mode"));
    attemptUnlock();
    showIdle(keyIsRecorded());
  }
}

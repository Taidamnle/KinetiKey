#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>

void uiInit();

bool buttonAPressedEvent();
bool buttonBPressedEvent();

void showStartupAnimation();
void showIdle(bool keyRecorded);

void showRecordSlot(uint8_t currentIndex);
void showUnlockSlot(uint8_t currentIndex);

void showRecordCompleteProgress();
void showUnlockCompleteProgress();

void showGestureBad();
void showUnlockFail();
void showUnlockSuccess();
void showRecordComplete();

void neoClear();

#endif

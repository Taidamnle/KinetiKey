#ifndef GESTURE_LOCK_H
#define GESTURE_LOCK_H

#include <stdbool.h>

void gestureLockInit();

bool keyIsRecorded();

void recordKey();
void attemptUnlock();

#endif

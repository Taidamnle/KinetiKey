#include "gesture_lock.h"

#include <stdint.h>
#include <math.h>

#include "debug_serial.h"
#include "delay_utils.h"
#include "lis3dh_driver.h"
#include "ui.h"

//Final demo can run without Serial - So it doesn't Really Matter the serial is only used for debugging

//How many gestures are in the password
const uint8_t KEY_LENGTH = 3;

//Gesture starts when total motion is above this value
const uint32_t START_THRESHOLD = 8000;

//Strongest axis needs to beat this to count as real movement
const int32_t AXIS_THRESHOLD = 6000;

//Keep smaller axes if they are strong enough compared to the main axis
//Lower value keeps more diagonal detail
const uint8_t KEEP_AXIS_PERCENT = 25;

//Use the first few samples to decide direction
const uint8_t SIGN_SAMPLES = 4;
const int32_t SIGN_THRESHOLD = 3000;

//How long we capture after the gesture starts
//14 samples x 20ms is about 280ms
const uint8_t CAPTURE_SAMPLES = 14;

//Similarity cutoff - if you want a stircter gesture u can change this i find 72 is a good number, so the range goes from around 60+, lower than 60 is very hard
const int16_t SIMILARITY_THRESHOLD = 72;

//Extra check so totally different vectors do not pass
const int16_t MAX_VECTOR_DISTANCE = 135;

//Raw acceleration values
static int16_t ax = 0;
static int16_t ay = 0;
static int16_t az = 0;

//Resting acceleration values
static int16_t restX = 0;
static int16_t restY = 0;
static int16_t restZ = 0;


//The gesture fingerprint
//Each axis is scaled from -100 to 100
struct GestureVector {
  int8_t x;
  int8_t y;
  int8_t z;
};


//The recorded password
static GestureVector recordedKey[KEY_LENGTH];

//True after key is recorded
static bool keyRecorded = false;


//Tell main whether a key exists
bool keyIsRecorded() {
  return keyRecorded;
}


//Absolute value
static int32_t abs32(int32_t value) {
  if (value < 0) {
    return -value;
  }

  return value;
}


//Sign function
static int8_t signOf(int32_t value) {
  if (value >= 0) {
    return 1;
  } else {
    return -1;
  }
}


//Zero vector means the gesture was too weak or unclear
static bool isZeroVector(GestureVector v) {
  return (v.x == 0 && v.y == 0 && v.z == 0);
}


//Debug print one gesture vector
static void debugPrintVector(GestureVector v) {
#if ENABLE_SERIAL_DEBUG
  Serial.print(F("("));
  Serial.print(v.x);
  Serial.print(F(", "));
  Serial.print(v.y);
  Serial.print(F(", "));
  Serial.print(v.z);
  Serial.print(F(")"));
#endif
}


//Save the current hand position as rest
static void calibrateRest() {
  DEBUG_PRINTLN(F("Calibrating rest position..."));

  int32_t sumX = 0;
  int32_t sumY = 0;
  int32_t sumZ = 0;

  for (uint8_t i = 0; i < 80; i++) {
    lis3dhReadAccel(&ax, &ay, &az);

    sumX += ax;
    sumY += ay;
    sumZ += az;

    delayMs(20);
  }

  restX = sumX / 80;
  restY = sumY / 80;
  restZ = sumZ / 80;

  DEBUG_PRINTLN(F("Rest calibration done"));
}


//Read motion after subtracting rest
static uint32_t readMotion(int32_t *motionX, int32_t *motionY, int32_t *motionZ) {
  lis3dhReadAccel(&ax, &ay, &az);

  *motionX = (int32_t)ax - restX;
  *motionY = (int32_t)ay - restY;
  *motionZ = (int32_t)az - restZ;

  return abs32(*motionX) + abs32(*motionY) + abs32(*motionZ);
}


//Get how strong one axis moved
static int32_t axisStrength(int32_t maxValue, int32_t minValue) {
  int32_t positiveStrength = abs32(maxValue);
  int32_t negativeStrength = abs32(minValue);

  if (positiveStrength >= negativeStrength) {
    return positiveStrength;
  } else {
    return negativeStrength;
  }
}


//Backup direction check using the biggest peak
static int8_t peakSign(int32_t maxValue, int32_t minValue) {
  if (abs32(maxValue) >= abs32(minValue)) {
    return 1;
  } else {
    return -1;
  }
}


//Use first motion for direction when possible
//If it is too weak, use the biggest peak instead
static int8_t chooseSign(int32_t firstPeak, int32_t maxValue, int32_t minValue) {
  if (abs32(firstPeak) >= SIGN_THRESHOLD) {
    return signOf(firstPeak);
  }

  return peakSign(maxValue, minValue);
}


//Scale one axis compared to the strongest axis
//Output range is -100 to 100
static int8_t normalizedComponent(int32_t strength, int32_t strongest, int8_t sign) {
  if (strongest <= 0) {
    return 0;
  }

  if (strength < AXIS_THRESHOLD) {
    return 0;
  }

  if (strength < ((strongest * KEEP_AXIS_PERCENT) / 100)) {
    return 0;
  }

  int32_t value = (strength * 100) / strongest;

  if (value > 100) {
    value = 100;
  }

  if (sign < 0) {
    value = -value;
  }

  return (int8_t)value;
}


//Capture one gesture and turn it into a vector
static GestureVector captureGestureVector(uint8_t mode, uint8_t gestureIndex) {
  GestureVector v;
  v.x = 0;
  v.y = 0;
  v.z = 0;

  int32_t motionX = 0;
  int32_t motionY = 0;
  int32_t motionZ = 0;

  uint32_t motion = 0;

  int32_t maxX = -2147483647;
  int32_t minX = 2147483647;

  int32_t maxY = -2147483647;
  int32_t minY = 2147483647;

  int32_t maxZ = -2147483647;
  int32_t minZ = 2147483647;

  int32_t firstPeakX = 0;
  int32_t firstPeakY = 0;
  int32_t firstPeakZ = 0;

#if ENABLE_SERIAL_DEBUG
  Serial.println();
  if (mode == 1) {
    Serial.print(F("[RECORD] Get ready for gesture "));
    Serial.print(gestureIndex + 1);
    Serial.println(F(" of 3"));
  }
  else if (mode == 2) {
    Serial.print(F("[UNLOCK] Get ready for gesture "));
    Serial.print(gestureIndex + 1);
    Serial.println(F(" of 3"));
  }
  else {
    Serial.println(F("[GESTURE] Get ready"));
  }

  Serial.println(F("Step 1: Return board to neutral/rest position."));
#endif

  delayMs(1200);

  if (mode == 1) {
    showRecordSlot(gestureIndex);
  }
  else if (mode == 2) {
    showUnlockSlot(gestureIndex);
  }

  DEBUG_PRINTLN(F("Step 2: Calibrating. Keep board still."));
  calibrateRest();

  DEBUG_PRINTLN(F("Step 3: Ready. Perform gesture now."));

  //Wait until the hand actually moves
  DEBUG_PRINTLN(F("Waiting for motion..."));

  while (1) {
    motion = readMotion(&motionX, &motionY, &motionZ);

    if (motion > START_THRESHOLD) {
      DEBUG_PRINTLN(F("Motion detected. Capturing gesture..."));
      break;
    }

    delayMs(20);
  }

  //Capture fixed window
  for (uint8_t i = 0; i < CAPTURE_SAMPLES; i++) {
    if (i != 0) {
      motion = readMotion(&motionX, &motionY, &motionZ);
    }

    //First few samples decide direction
    if (i < SIGN_SAMPLES) {
      if (abs32(motionX) > abs32(firstPeakX)) {
        firstPeakX = motionX;
      }

      if (abs32(motionY) > abs32(firstPeakY)) {
        firstPeakY = motionY;
      }

      if (abs32(motionZ) > abs32(firstPeakZ)) {
        firstPeakZ = motionZ;
      }
    }

    //Whole capture window decides strength
    if (motionX > maxX) {
      maxX = motionX;
    }

    if (motionX < minX) {
      minX = motionX;
    }

    if (motionY > maxY) {
      maxY = motionY;
    }

    if (motionY < minY) {
      minY = motionY;
    }

    if (motionZ > maxZ) {
      maxZ = motionZ;
    }

    if (motionZ < minZ) {
      minZ = motionZ;
    }

    delayMs(20);
  }

  neoClear();

  DEBUG_PRINTLN(F("Capture done. Processing gesture vector..."));

  int32_t strengthX = axisStrength(maxX, minX);
  int32_t strengthY = axisStrength(maxY, minY);
  int32_t strengthZ = axisStrength(maxZ, minZ);

  int32_t strongest = strengthX;

  if (strengthY > strongest) {
    strongest = strengthY;
  }

  if (strengthZ > strongest) {
    strongest = strengthZ;
  }

  if (strongest < AXIS_THRESHOLD) {
    DEBUG_PRINTLN(F("Gesture too weak. Retry."));
    return v;
  }

  int8_t signX = chooseSign(firstPeakX, maxX, minX);
  int8_t signY = chooseSign(firstPeakY, maxY, minY);
  int8_t signZ = chooseSign(firstPeakZ, maxZ, minZ);

  v.x = normalizedComponent(strengthX, strongest, signX);
  v.y = normalizedComponent(strengthY, strongest, signY);
  v.z = normalizedComponent(strengthZ, strongest, signZ);

#if ENABLE_SERIAL_DEBUG
  Serial.print(F("Gesture vector: "));
  debugPrintVector(v);
  Serial.println();
  Serial.println(F("Gesture accepted."));
#endif

  return v;
}


//Simple component-by-component difference
static int16_t vectorDistance(GestureVector a, GestureVector b) {
  int16_t dx = (int16_t)abs32((int32_t)a.x - (int32_t)b.x);
  int16_t dy = (int16_t)abs32((int32_t)a.y - (int32_t)b.y);
  int16_t dz = (int16_t)abs32((int32_t)a.z - (int32_t)b.z);

  return dx + dy + dz;
}


//Cosine similarity between two vectors
static int16_t vectorSimilarity(GestureVector a, GestureVector b) {
  int32_t dot = ((int16_t)a.x * (int16_t)b.x)
              + ((int16_t)a.y * (int16_t)b.y)
              + ((int16_t)a.z * (int16_t)b.z);

  int32_t magA2 = ((int16_t)a.x * (int16_t)a.x)
                + ((int16_t)a.y * (int16_t)a.y)
                + ((int16_t)a.z * (int16_t)a.z);

  int32_t magB2 = ((int16_t)b.x * (int16_t)b.x)
                + ((int16_t)b.y * (int16_t)b.y)
                + ((int16_t)b.z * (int16_t)b.z);

  if (magA2 == 0 || magB2 == 0) {
    return -100;
  }

  float magA = sqrt((float)magA2);
  float magB = sqrt((float)magB2);

  float similarity = ((float)dot / (magA * magB)) * 100.0;

  return (int16_t)similarity;
}


//Check if the attempt is close enough
static bool vectorsMatch(GestureVector recorded, GestureVector attempt) {
  int16_t similarity = vectorSimilarity(recorded, attempt);
  int16_t distance = vectorDistance(recorded, attempt);

#if ENABLE_SERIAL_DEBUG
  Serial.print(F("Similarity: "));
  Serial.print(similarity);
  Serial.print(F("% | Distance: "));
  Serial.println(distance);
#endif

  return (similarity >= SIMILARITY_THRESHOLD && distance <= MAX_VECTOR_DISTANCE);
}


//Set up gesture lock sensor
void gestureLockInit() {
  lis3dhInit();
  delayMs(500);

#if ENABLE_SERIAL_DEBUG
  uint8_t id = lis3dhReadRegister(0x0F);
  Serial.print(F("WHO_AM_I: "));
  Serial.println(id, HEX);
#endif
}


//Record the 3 gesture password
void recordKey() {
  DEBUG_PRINTLN(F("===== RECORD MODE ====="));
  DEBUG_PRINTLN(F("You will record a 3-gesture key."));
  DEBUG_PRINTLN(F("For each gesture: hold still, wait for ready, then move."));

  showRecordSlot(0);

  for (uint8_t i = 0; i < KEY_LENGTH; i++) {
#if ENABLE_SERIAL_DEBUG
    Serial.print(F("Record gesture "));
    Serial.print(i + 1);
    Serial.println(F(" of 3"));
#endif

    while (1) {
      GestureVector v = captureGestureVector(1, i);

      if (isZeroVector(v)) {
        showGestureBad();
        DEBUG_PRINTLN(F("Bad gesture: too weak or unclear."));
        DEBUG_PRINTLN(F("Redo this same gesture."));
      } else {
        recordedKey[i] = v;

#if ENABLE_SERIAL_DEBUG
        Serial.print(F("Saved gesture "));
        Serial.print(i + 1);
        Serial.print(F(": "));
        debugPrintVector(recordedKey[i]);
        Serial.println();
#endif

        //Keep saved gesture slot lit
        showRecordSlot(i + 1);
        delayMs(500);

        break;
      }
    }

    delayMs(1000);
  }

  keyRecorded = true;

  DEBUG_PRINTLN(F("KEY RECORDED"));
  DEBUG_PRINTLN(F("Press Button A to try unlocking."));

  showRecordCompleteProgress();
  delayMs(800);
  showRecordComplete();
}


//Try to unlock with 3 gestures
void attemptUnlock() {
  DEBUG_PRINTLN(F("===== UNLOCK MODE ====="));
  DEBUG_PRINTLN(F("Repeat the same 3 gestures in order."));

  if (!keyRecorded) {
    DEBUG_PRINTLN(F("No key recorded yet."));
    showGestureBad();
    return;
  }

  showUnlockSlot(0);

  for (uint8_t i = 0; i < KEY_LENGTH; i++) {
#if ENABLE_SERIAL_DEBUG
    Serial.print(F("Attempt gesture "));
    Serial.print(i + 1);
    Serial.println(F(" of 3"));
#endif

    GestureVector attempt;

    while (1) {
      attempt = captureGestureVector(2, i);

      if (isZeroVector(attempt)) {
        showGestureBad();
        DEBUG_PRINTLN(F("Bad gesture. Try this gesture again."));
      } else {
        break;
      }
    }

#if ENABLE_SERIAL_DEBUG
    Serial.print(F("Expected: "));
    debugPrintVector(recordedKey[i]);
    Serial.print(F(" | Got: "));
    debugPrintVector(attempt);
    Serial.println();
#endif

    if (!vectorsMatch(recordedKey[i], attempt)) {
      DEBUG_PRINTLN(F("UNLOCK FAILED"));
      showUnlockFail();
      return;
    }

#if ENABLE_SERIAL_DEBUG
    Serial.print(F("Gesture "));
    Serial.print(i + 1);
    Serial.println(F(" correct."));
#endif

    //Keep correct attempt slot lit green
    showUnlockSlot(i + 1);
    delayMs(800);
  }

  DEBUG_PRINTLN(F("UNLOCK SUCCESS"));

  showUnlockCompleteProgress();
  delayMs(600);
  showUnlockSuccess();
}

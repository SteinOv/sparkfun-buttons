#include <stdlib.h>

#include <SPI.h>
#include <stdint.h>
#include <SoftwareSerial.h> 
#include <bluefruit.h>
#include "motor_buttons.h"

const int PRESS_INTERVAL_MS = 10;
const int LONG_PRESS_MIN_INTERVALS = 60;
const int DOUBLE_PRESS_MAX_GAP_INTERVALS = 60;
const float MIN_LOW_FRACTION_FOR_PRESS = 0.75;

INIT_BUTTON(BUTTON_1, 29);
INIT_BUTTON(BUTTON_2, 21);

const int BUTTONS_AMOUNT = 2;
button BUTTONS[BUTTONS_AMOUNT] = {BUTTON_1, BUTTON_2};

BLEUart BLE;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting...");

    for (int i=0; i < BUTTONS_AMOUNT; i++) {
        pinMode(BUTTONS[i].pin, INPUT_PULLUP);
    }

  // Initialize Bluetooth:
  Bluefruit.begin();
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  Bluefruit.setName("motor_buttons");
  BLE.begin();

  // Start advertising device and bleuart services
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(BLE);
  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  // Set advertising interval (in unit of 0.625ms):
  Bluefruit.Advertising.setInterval(32, 244);
  // number of seconds in fast mode:
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);  


  Serial.println("--Setup Done--");
}

void loop() {
  if (BLE.available()) {
    char buffer[64];
    BLE.readBytes(buffer, 64);
    Serial.println(buffer);
  }
  // Check for type of button press
  for (int i=0; i < BUTTONS_AMOUNT; i++) {
    button *button = &BUTTONS[i];
    button->pinState = digitalRead(button->pin);
    PressType pressType = determinePressType(button); 

    if (pressType != TYPE_NONE) {
      char bleString[20];

      if (pressType == SINGLE_PRESS) {
        snprintf(bleString, 20, "%d,single", button->pin);
      } else if (pressType == DOUBLE_PRESS) {
        snprintf(bleString, 20, "%d,double", button->pin);
      } else if (pressType == LONG_PRESS) {
        snprintf(bleString, 20, "%d,long", button->pin);
      }

      BLE.write(bleString);
    }
 
    }
}


PressType determinePressType(button *button) {
  // Low pin state detected, start counting
  if (button->pressState == STATE_NONE && button->pinState == LOW) {
    button->pressState = POTENTIAL_PRESS;
    button->lowCount++;
    button->startTime = millis();
    return TYPE_NONE;
  // No press initiated
  } else if (button->pressState == STATE_NONE) {
    return TYPE_NONE;
  }

  // Count pin state
  if (button->pinState == LOW) {
    button->lowCount++;
  } else {
    button->highCount++;
  }

  // Still within press interval
  if (millis() - button->startTime < PRESS_INTERVAL_MS) {
    return TYPE_NONE;
  }

  // Determine whether press or no press was detected for this interval
  bool isPressThisInterval = button->lowCount > (button->lowCount + button->highCount) * MIN_LOW_FRACTION_FOR_PRESS;
  
  // Reset interval specific values
  button->lowCount = 0;
  button->highCount = 0;
  button->startTime = millis();

  // Potential press
  if (button->pressState == POTENTIAL_PRESS) {
    if (isPressThisInterval) {
      button->pressIntervalCount++;

      // Long press
      if (button->pressIntervalCount >= LONG_PRESS_MIN_INTERVALS) {
        resetButton(button);
        button->pressState = WAITING_FOR_RELEASE;
        return LONG_PRESS;
      }

      return TYPE_NONE;
    // No press
    } else if (button->pressIntervalCount == 0){
        resetButton(button);
        button->pressState = STATE_NONE;
        return TYPE_NONE;
    // Potential double press, start counting no presses
    } else {
      button->noPressIntervalCount = 1;
      button->pressState = POTENTIAL_DOUBLE_PRESS;
      return TYPE_NONE;
    }
  }

  // Double or single press
  if (button->pressState == POTENTIAL_DOUBLE_PRESS) {
    if (!isPressThisInterval) {
      button->noPressIntervalCount++;
      if (button->noPressIntervalCount > DOUBLE_PRESS_MAX_GAP_INTERVALS) {
        resetButton(button);
        button->pressState = STATE_NONE;
        return SINGLE_PRESS;
      }
      return TYPE_NONE;
    } else {
      resetButton(button);
      button->pressState = WAITING_FOR_RELEASE;
      return DOUBLE_PRESS;
    }
  }

  // Waiting for release
  if (button->pressState == WAITING_FOR_RELEASE && !isPressThisInterval) {
    button->pressState = STATE_NONE;
    return TYPE_NONE;
  } else if (button->pressState == WAITING_FOR_RELEASE) {
    return TYPE_NONE;
  }

  Serial.println("Unexpected button state");
  button->pressState = STATE_NONE;
  resetButton(button);
  return TYPE_NONE;
}

void resetButton(button *button) {
  button->lowCount = 0;
  button->highCount = 0;
  button->pressIntervalCount = 0;
  button->noPressIntervalCount = 0;
}

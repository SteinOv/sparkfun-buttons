#ifndef MOTOR_BUTTONS_H
#define MOTOR_BUTTONS_H

// Different states while determining press type
typedef enum {
  STATE_NONE, // Not pressed
  POTENTIAL_PRESS, // Start of press, no press still possible
  POTENTIAL_DOUBLE_PRESS,
  WAITING_FOR_RELEASE // Press already registered, but waiting for release before starting next press
} PressState;

// Possible press types
typedef enum {
    TYPE_NONE,
    SINGLE_PRESS,
    DOUBLE_PRESS,
    LONG_PRESS
} PressType;

typedef struct button {
    const int pin;
    byte pinState;
    PressState pressState;
    int highCount;
    int lowCount;
    long startTime;
    int pressIntervalCount;
    int noPressIntervalCount;
    byte pinmode;
} button;


#define INIT_BUTTON(button_name, pin_number) struct button button_name = {\
    .pin = pin_number, .pinState = HIGH, .pressState = STATE_NONE, \
    .highCount = 0, .lowCount = 0, .startTime = 0, .pressIntervalCount = 0,\
    .noPressIntervalCount = 0, .pinmode = INPUT_PULLUP};



#endif // MOTOR_BUTTONS_H
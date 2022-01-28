#pragma once
//-----------------------------
// gglobal headers
#include <Arduino.h>
#include <math.h>

// ----------------------------
// CONSTANTS
const unsigned long MILLIS_MAX_TIME=0xffffffff;

// ----------------------------
// ENUM DEFINITIONS
//-----------------------------
enum ESC_MOTORS //motors controlled by ESC
{
  MOT_FRONT_RIGHT,
  MOT_FRONT_LEFT,
  MOT_BACK_RIGHT,
  MOT_BACK_LEFT,
  NUMBER_OF_MOTORS
};

enum PROXIMITY_SENSORS 
{
  PS_FRONT,
  PS_BACK,
  PS_RIGHT,
  PS_LEFT,
  NUMBER_OF_PS
};

enum PIN_TYPES
{
  PIN_AUTO_MANUAL_SWITCH,
  PIN_AUTONOMY_LED,
  PIN_MOTOR_ENA,
  PIN_SERVO,
  PIN_PROXIMITY
};


// ----------------------------
// Typedef structures
//----------------------------
typedef struct
{
  //PWM pins
  int ENA[ESC_MOTORS::NUMBER_OF_MOTORS];
} ESC;

typedef struct
{
  //PWM pins
  int ENA;
  bool latch;
} LatchMechanism;

typedef struct
{
  //PWM pins
  int ENA;
} ProximitySensor;
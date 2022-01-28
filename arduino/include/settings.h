#pragma once
#include "mc_node.h"


// ----------------------------
// Constants
//----------------------------
//use time to open/close to detect anomalies signaling that the actuators are damaged
const unsigned long time_deviation_threshold = 30E+3;  //(ms) max time deviation allowed from expected opening/closing duration


// ----------------------------
// MCU pins assignment
//----------------------------
/* Table below summarize the Pin arrangement for boat & ground
PIN     BOAT                  GROUND
------  --------------------  ------
00 TX
01 RX
02      doorA.motorLHS.IN1    door.motor.IN1
03 PWM  doorA.motorLHS.IN2    door.motor.IN2
04      doorA.motorRHS.IN1
05 PWM  doorA.motorRHS.IN2
06 PWM  doorB.motorLHS.IN1
07      doorB.motorLHS.IN2
08      doorB.motorRHS.IN1
09 PWM  doorB.motorRHS.IN2
10 PWM  
11      
12      
13 LED  
14 A0   doorA.motorLHS.SW[0]  door.motor.SW[0]
15 A1   doorA.motorLHS.SW[1]  door.motor.SW[1]
16 A2   doorA.motorRHS.SW[0]
17 A3   doorA.motorRHS.SW[1]
18 A4   doorB.motorLHS.SW[0]
19 A5   doorB.motorLHS.SW[1]
20 A6   doorB.motorRHS.SW[0]
21 A7   doorB.motorRHS.SW[1] 
*/
/* The pin arrangement is loaded in the code with a bi-dimensional array:
- # of pins
- Pin definition: FIVE (5) integers:
  - Pin #
  - Pin type
  - Hardware indices (up to 3 enough to identify all, values not needed just filled with zeros)
*/
const int n_pins = 11;
const int PINS[n_pins][3] = 
{
  { 2, PIN_TYPES::PIN_MOTOR_ENA, ESC_MOTORS::MOT_FRONT_RIGHT},
  { 3, PIN_TYPES::PIN_MOTOR_ENA, ESC_MOTORS::MOT_FRONT_RIGHT},
  { 4, PIN_TYPES::PIN_MOTOR_ENA, ESC_MOTORS::MOT_FRONT_RIGHT},
  { 5, PIN_TYPES::PIN_MOTOR_ENA, ESC_MOTORS::MOT_FRONT_RIGHT},
  {10, PIN_TYPES::PIN_AUTO_MANUAL_SWITCH, 0},
  {11, PIN_TYPES::PIN_SERVO, 0},
  {13, PIN_TYPES::PIN_AUTONOMY_LED, 0},
  {A2, PIN_TYPES::PIN_PROXIMITY, PROXIMITY_SENSORS::PS_FRONT},
  {A3, PIN_TYPES::PIN_PROXIMITY, PROXIMITY_SENSORS::PS_BACK},
  {A0, PIN_TYPES::PIN_PROXIMITY, PROXIMITY_SENSORS::PS_RIGHT},
  {A1, PIN_TYPES::PIN_PROXIMITY, PROXIMITY_SENSORS::PS_LEFT}
};
//

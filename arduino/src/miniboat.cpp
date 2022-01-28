#include "miniboat.h"

/* ------------------------------------------------------------------------- */
/* CONSTRUCTOR / DESTRUCTOR */
/* ------------------------------------------------------------------------- */
Miniboat::Miniboat()
{  
} 
// ----------------------------
// INITIALIZE
void Miniboat::initialize()
{
  //Assign pin values 
  for (int i=0; i<n_pins; i++)
  {
    switch (PINS[i][1])
    {
      case PIN_TYPES::PIN_AUTO_MANUAL_SWITCH : SWITCH_AUTO_MANUAL = PINS[i][0]; break;
      case PIN_TYPES::PIN_AUTONOMY_LED : AUTONOMY_LED = PINS[i][0]; break;
      case PIN_TYPES::PIN_MOTOR_ENA : esc.ENA[PINS[i][2]] = PINS[i][0]; break;
      case PIN_TYPES::PIN_SERVO : lm.ENA = PINS[i][0]; break;
      case PIN_TYPES::PIN_PROXIMITY : ps[PINS[i][2]].ENA = PINS[i][0]; break;
    }
  }
  /*NOTE
  It is assumed here that settings are well defined, so all pins controlling the motors and switches 
  in all doors are properly assigned above */

  //-----------------------------
  //Initialize motor pins:
  

  // initializes autonomy mode
  autonomy_mode = true; 
  last_debounce_time = 0;
  pinMode(SWITCH_AUTO_MANUAL, INPUT_PULLUP);
  pinMode(AUTONOMY_LED, OUTPUT);
  digitalWrite(AUTONOMY_LED, autonomy_mode ? HIGH : LOW);

  // Initializes local gate structure variables 
  max_calibrated = false;
  min_calibrated = false;
  err_status = 0;

  return;
}


// ----------------------------
// LATCH FUNCTIONS
bool Miniboat::is_open()
{
  return lm.latch;
}
void Miniboat::servo_open()
{
  //here goes the functionality to deploy latching mechanism
  return;
}
void Miniboat::servo_close()
{
  //here goes the functionality to close the servo
  return;
}


// ----------------------------
// MOTOR FUNCTIONS
void Miniboat::calibrate()
{
  //here should start the calibration sequence
  return;
}
void Miniboat::motor_speed(ESC_MOTORS motor_id, int frc_cmd) {
  //here should send the pwm command to each of the motors
  return;
}

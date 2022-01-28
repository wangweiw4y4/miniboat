#pragma once

#include "mc_node.h"
#include "settings.h"

class Miniboat
{
  private:
    int bin_id_;

  public:
    Miniboat();
    //gate loaded in a vector, so initializes it after as a separate function
    void initialize();
    
    //manual control switches & leds
    bool autonomy_mode;
    unsigned long last_debounce_time;
    int SWITCH_AUTO_MANUAL, LATCH, DELATCH, AUTONOMY_LED;

    int frc_cmd[ESC_MOTORS::NUMBER_OF_MOTORS];
    bool latch_cmd;
    
    //hardware components
    ESC esc;
    LatchMechanism lm;
    ProximitySensor ps[PROXIMITY_SENSORS::NUMBER_OF_PS];

    //open-close calibration
    bool max_calibrated, min_calibrated;
    bool max_calibrating, min_calibrating;
    unsigned long max_time, min_time;
    unsigned long max_start_time, min_start_time;
    unsigned long delta_time_;

    //error status: 0 no error (initial value), 1 blocked in wrong state
    int err_status;

    //servo functions
    bool is_open();
    void servo_open();
    void servo_close();
    //motor functions
    void calibrate();
    void motor_speed(ESC_MOTORS motor_id, int frc_cmd);

};

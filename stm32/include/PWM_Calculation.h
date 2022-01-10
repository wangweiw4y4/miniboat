#ifndef __CPG_H
#define __CPG_H

#define PWM_Period_ms 20
#define PWM_Duty_Middle_us 1500
#define PWM_Duty_LeftLimit_us 1100
#define PWM_Duty_RightLimit_us 1900

#define Trans_factor             3               //60000/20000
#define MiddlePosition_value     4500            //PWM_Duty_Middle_us * Trans_factor; 
#define LeftLimit_value          3300            //PWM_Duty_LeftLimit_us  * Trans_factor; 
#define RightLimit_value         5700           //PWM_Duty_RightLimit_us * Trans_factor; 
#define min(x,y) ((x) < (y) ? x : y)
#define PI 3.14159265358

void PWM_Calculation(float LEFT, float RIGHT, float FRONT, float REAR);
void ServoMotor_PWM_Calculation(float LATCHING);
float TaylorSin(float x);
float TaylorCos(float x);



#endif


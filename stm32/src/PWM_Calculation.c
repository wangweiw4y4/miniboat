#include "stm32f10x.h"
#include "math.h"
#include "PWM_Calculation.h"

int PWM_Value[4];
int SERVOMotor_PWM_Value;
float T = 0.02;

void PWM_Calculation(float LEFT, float RIGHT, float FRONT, float REAR)
{

    LEFT = LEFT + 1500;
    RIGHT = RIGHT + 1500;
    FRONT = FRONT + 1500;
    REAR = REAR + 1500;

    PWM_Value[0] = LEFT * Trans_factor;
    PWM_Value[0] = PWM_Value[0] > RightLimit_value ? RightLimit_value   :  PWM_Value[0];
    PWM_Value[0] = PWM_Value[0] < LeftLimit_value   ? LeftLimit_value   :  PWM_Value[0];

    PWM_Value[1] = RIGHT * Trans_factor;
    PWM_Value[1] = PWM_Value[1] > RightLimit_value ? RightLimit_value   :  PWM_Value[1];
    PWM_Value[1] = PWM_Value[1] < LeftLimit_value   ? LeftLimit_value   :  PWM_Value[1];

    PWM_Value[2] = FRONT * Trans_factor;
    PWM_Value[2] = PWM_Value[2] > RightLimit_value ? RightLimit_value   :  PWM_Value[2];
    PWM_Value[2] = PWM_Value[2] < LeftLimit_value   ? LeftLimit_value   :  PWM_Value[2];

    PWM_Value[3] = REAR * Trans_factor;
    PWM_Value[3] = PWM_Value[3] > RightLimit_value ? RightLimit_value   :  PWM_Value[3];
    PWM_Value[3] = PWM_Value[3] < LeftLimit_value   ? LeftLimit_value   :  PWM_Value[3];
}


void ServoMotor_PWM_Calculation(float LATCHING)
{
    LATCHING = LATCHING + 1500;
    SERVOMotor_PWM_Value = LATCHING * Trans_factor;
    SERVOMotor_PWM_Value = SERVOMotor_PWM_Value > RightLimit_value ? RightLimit_value   :  SERVOMotor_PWM_Value;
    SERVOMotor_PWM_Value = SERVOMotor_PWM_Value < LeftLimit_value   ? LeftLimit_value   :  SERVOMotor_PWM_Value;
}


float TaylorSin(float x)
{
    float y = 0.0;
    float temp;

    while(x > (2 * PI))
        x = x - 2 * PI;

    while(x < 0)
        x = x + 2 * PI;

    if(x <= (PI / 2))
        y = x - x * x * x / 6;

    if(x > (PI / 2) && x <= PI)
    {
        temp = PI - x;
        y = temp - temp * temp * temp / 6;
    }

    if(x > PI && x <= (PI * 3 / 2))
    {
        temp = x - PI;
        y = temp * temp * temp / 6 - temp;
    }

    if(x > (PI * 3 / 2))
    {
        temp = 2 * PI - x;
        y = temp * temp * temp / 6 - temp;
    }

    return y;
}


float TaylorCos(float x)
{
    float y = 0.0;
    float temp;

    while(x > (PI))
        x = x - 2 * PI;

    while(x < (-PI))
        x = x + 2 * PI;

    if(x > 0 && x <= (PI / 2))
        y = 1 - x * x / 2 + x * x * x * x / 24;

    if(x > (-PI / 2) && x <= 0)
    {
        temp = PI - x;
        y = 1 - temp * temp / 2 + temp * temp * temp * temp / 24;
    }

    if(x > (PI / 2) && x <= PI)
    {
        temp = x - PI;
        y = temp * temp / 2 - temp * temp * temp * temp / 24 - 1;
    }

    if(x >= (-PI) && x <= (-PI / 2))
    {
        temp = 2 * PI - x;
        y = temp * temp / 2 - temp * temp * temp * temp / 24 - 1;
    }

    return y;
}



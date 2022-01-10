#include "stm32f10x.h"
#include "PWM_Update.h"



void PWM_LEFT(u16 value)
{
    TIM_SetCompare1(TIM3, value);
}

void PWM_RIGHT(u16 value)
{
    TIM_SetCompare2(TIM3, value);
}


void PWM_FRONT(u16 value)
{
    TIM_SetCompare3(TIM3, value);
}

void PWM_REAR(u16 value)
{
    TIM_SetCompare4(TIM3, value);
}

void PWM_ServoMotor(u16 value)
{
    TIM_SetCompare1(TIM1, value);
}

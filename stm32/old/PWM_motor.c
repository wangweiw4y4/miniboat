#include "stm32f10x.h"
#include "PWM_Update.h"



void PWM_LEFT(u16 value)	//×óÂÝÐý½°
{
    TIM_SetCompare1(TIM3, value);
}

void PWM_RIGHT(u16 value)	//ÓÒÂÝÐý½°
{
    TIM_SetCompare2(TIM3, value);
}


void PWM_FRONT(u16 value)	//Ç°ÂÝÐý½°
{
    TIM_SetCompare3(TIM3, value);
}

void PWM_REAR(u16 value)	 //ºóÂÝÐý½°
{
    TIM_SetCompare4(TIM3, value);
}

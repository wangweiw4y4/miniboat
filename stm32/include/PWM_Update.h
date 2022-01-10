#ifndef __PWM_MOTOR__
#define __PWM_MOTOR__
#include "stm32f10x.h"

void PWM_LEFT(u16 value);
void PWM_RIGHT(u16 value);
void PWM_FRONT(u16 value);
void PWM_REAR(u16 value);
void PWM_ServoMotor(u16 value);
#endif

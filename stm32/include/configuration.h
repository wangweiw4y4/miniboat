#ifndef __CONFIGURATION_H
#define	__CONFIGURATION_H
#include "stm32f10x.h"

void RCC_Configuration(void);
void GPIO_Configuration(void);
void USART_Configuration(void);
void TIM1_Configuration(void);
void TIM3_Configuration(void);
void I2C_Configuration(void);
void NVIC_Configuration(void);
void EXTI_Configuration(void);
void Systick_Configuration(void);
void delay(u32 count);
void USART2_IRQHandler(void);
void USART3_IRQHandler(void);



#endif

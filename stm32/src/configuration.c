#include "stm32f10x.h"
#include "configuration.h"
#include <stdarg.h>
#include "CRC.h"
/*
********************************************************************************
RCC_Configuration(void)
********************************************************************************
*/
void RCC_Configuration(void)
{
    /* RCC system reset(for debug purpose) */
    ErrorStatus HSEStartUpStatus;
    RCC_DeInit();			//

    /* Enable HSE */
    RCC_HSEConfig(RCC_HSE_ON);	  //
    /* Wait till HSE is ready */
    HSEStartUpStatus = RCC_WaitForHSEStartUp();  //

    if (HSEStartUpStatus == SUCCESS)		//
    {
        /* HCLK = SYSCLK */
        RCC_HCLKConfig(RCC_SYSCLK_Div1);

        /* PCLK2 = HCLK */
        RCC_PCLK2Config(RCC_HCLK_Div1);

        /* PCLK1 = HCLK/2 */
        RCC_PCLK1Config(RCC_HCLK_Div2);

        /* Flash 2 wait state */
        FLASH_SetLatency(FLASH_Latency_2);
        /* Enable Prefetch Buffer */
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

        /* PLLCLK = 8MHz * 9 = 72 MHz */
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
        /* Enable PLL */
        RCC_PLLCmd(ENABLE);

        /* Wait till PLL is ready */
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
        {
        }

        /* Select PLL as system clock source */
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

        /* Wait till PLL is used as system clock source */
        while (RCC_GetSYSCLKSource() != 0x08)
        {
        }
    }

    /* Enable GPIOA, GPIOB and AFIO clocks */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);		// Enable USART1 Clock
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);		//Enable USART2 Clock
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);		//Enable USART3 Clock

//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);//ENABLE Time1 clock for rotating servomotor control


//	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
//

//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
}



/*
********************************************************************************
********************************************************************************
*/
void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;


    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /*We use one PWM for latching, it is TIM1[CH1], corresponding to PA8*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);


    /*LED Configuration*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;			 //LED-->PA.1
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_SetBits(GPIOA, GPIO_Pin_1);


    //	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    //	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    //	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    //	GPIO_Init(GPIOB, &GPIO_InitStructure);


    //	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
    //	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    //	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    //	GPIO_Init(GPIOB, &GPIO_InitStructure);
    //

    //	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10;
    //	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    //	GPIO_InitStructure.GPIO_Mode =  GPIO_Mode_IPU;
    //	GPIO_Init(GPIOA, &GPIO_InitStructure);
    //

    //	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_3 | GPIO_Pin_5;
    //	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    //	GPIO_Init(GPIOA, &GPIO_InitStructure);
    //

    //	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 ;
    //	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    //	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    //	GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);


    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);


    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);


    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);


    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);


    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/*
********************************************************************************
********************************************************************************
*/
void TIM3_Configuration(void)
{

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    TIM_TimeBaseStructure.TIM_Period = 60000;
    TIM_TimeBaseStructure.TIM_Prescaler = 23;		 //Fpwm=50Hz;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);


    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    TIM_OCInitStructure.TIM_Pulse = 4500;
    TIM_OC4Init(TIM3, &TIM_OCInitStructure);	  //TIM3->CH4

    TIM_OCInitStructure.TIM_Pulse = 4500;
    TIM_OC3Init(TIM3, &TIM_OCInitStructure);	  //TIM3->CH3

    TIM_OCInitStructure.TIM_Pulse = 4500;
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);	  //TIM3->CH2

    TIM_OCInitStructure.TIM_Pulse = 4500;
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);	  //TIM3->CH1


    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM3, ENABLE);


    TIM_Cmd(TIM3, ENABLE);
}

/*
********************************************************************************
TIM1_Configuration(void)
********************************************************************************
*/
void TIM1_Configuration(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    TIM_OCInitTypeDef TIM_OCInitStructure;

    // Fpwm=72M/[(Prescaler+1)*Period];
    TIM_TimeBaseStructure.TIM_Period = 60000;
    TIM_TimeBaseStructure.TIM_Prescaler = 23;		 //Fpwm=50Hz;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);


    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;


    TIM_OCInitStructure.TIM_Pulse = 4500;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);	   //TIM1->CH1

    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM1, ENABLE);

    /* TIM1 Main Output Enable */
    TIM_CtrlPWMOutputs(TIM1, ENABLE);/*Perticular configuration for timer1 and timer8, timer1 will have no output without this configuration*/

    TIM_Cmd(TIM1, ENABLE);
}
/*
********************************************************************************
USART_Configuration(void)
********************************************************************************
*/
void USART_Configuration(void)
{
    USART_InitTypeDef USART_InitStructure;
    /*UART1 For Indoor GPS Data Reading*/
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    //UART2 For Wireless communication
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    //UART3 For IMU Data Reading
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);


    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);


    USART_Cmd(USART1, ENABLE);
    USART_Cmd(USART2, ENABLE);
    USART_Cmd(USART3, ENABLE);


//   /* Enable USART1 global interrupt */
//  NVIC_EnableIRQ(USART1_IRQn);


    USART_GetFlagStatus(USART1, USART_FLAG_TC);
    USART_GetFlagStatus(USART2, USART_FLAG_TC);
    USART_GetFlagStatus(USART3, USART_FLAG_TC);
}


/*
********************************************************************************I2C_Configuration(void)
********************************************************************************
*/
//void I2C_Configuration(void)
//{
//	I2C_InitTypeDef I2C_InitStructure;
//
//	// I2CÕ‚…Ë∏¥Œª.
//	RCC_APB1PeriphResetCmd( RCC_APB1Periph_I2C2, ENABLE );
//  RCC_APB1PeriphResetCmd( RCC_APB1Periph_I2C2, DISABLE );
//  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);

//	// I2C≈‰÷√.
//	I2C_DeInit( I2C2 );
//	// I2C2 configuration ---------------------------------------------
//	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
//	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
//	I2C_InitStructure.I2C_OwnAddress1 = 0x70;      //
//	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
//	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
//	I2C_InitStructure.I2C_ClockSpeed = 200000;//400000;

//	I2C_Cmd(I2C2, ENABLE );

//	I2C_Init(I2C2, &I2C_InitStructure);
//	I2C_GeneralCallCmd(I2C2,ENABLE);

//	I2C_ITConfig( I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, ENABLE );
//}



/*
********************************************************************************
NVIC_Configuration
********************************************************************************
*/
void NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    #ifdef  VECT_TAB_RAM
    /* Set the Vector Table base location at 0x20000000 */
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
    #else  /* VECT_TAB_FLASH  */
    /* Set the Vector Table base location at 0x08000000 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
    #endif

    //Usart1 NVIC
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);	//

    //Usart2 NVIC
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//
    NVIC_Init(&NVIC_InitStructure);	//

    //Usart3 NVIC
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//
    NVIC_Init(&NVIC_InitStructure);	//




//	/* 4 bit for pre-emption priority, 0 bits for subpriority */
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
//
//	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
//
//

//	/* Configure and enable I2C1 interrupts -------------------------------------*/
//	/* 0 bit for pre-emption priority, 4 bits for subpriority */
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
//
//	NVIC_InitStructure.NVIC_IRQChannel = I2C2_EV_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);

//	/* 0 bit for pre-emption priority, 4 bits for subpriority */
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
//
//	NVIC_InitStructure.NVIC_IRQChannel = I2C2_ER_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
}




/*
********************************************************************************
EXTI_Configuration(void)
********************************************************************************
*/
//void EXTI_Configuration(void)
//{
//	EXTI_InitTypeDef EXTI_InitStructure;

//	EXTI_ClearITPendingBit(EXTI_Line10);

//	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource10);
//	/* Configure Button EXTI line */
//	EXTI_InitStructure.EXTI_Line = EXTI_Line10;
//	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
//	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
//	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
//	EXTI_Init(&EXTI_InitStructure);
//}

/*
********************************************************************************
 Systick_Configuration(void)
********************************************************************************
*/
void Systick_Configuration(void)
{
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

    while (SysTick_Config(1440000));		//1440000;
}

/*
********************************************************************************
 delay(u32 count)
********************************************************************************
*/
void delay(u32 count)
{
    while (count--);
}





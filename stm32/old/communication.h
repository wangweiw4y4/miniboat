#ifndef __COMMUNICATION__
#define __COMMUNICATION__
#include "stm32f10x.h"


#include "stm32f10x.h"
#define Data_buffer 0x36
#define BufferSize 40

/****接收数据类型定义*****/
#define CPG_Command 0X00
#define Static_Offset_1_Sum 0X01
#define Static_Offset_1_Sub 0X02
#define Static_Offset_2_Sum 0X03
#define Static_Offset_2_Sub 0X04
#define Static_Offset_3_Sum 0X05
#define Static_Offset_3_Sub 0X06
#define Clear_data 0X07

/****发送相关类型定义*****/
/* 一些标志的宏定义 */
#define FLAG_RECV_REG_ADDR 0
#define FLAG_RECV_DATA	1
#define FLAG_SEND_DATA	2


/*****       从机内部虚拟寄存器地址说明     *****/
/************************************************/
/*0xA1 - 传感器编号寄存器*/
/*0xA2 - 数据长度寄存器*/
/*0xA3 - 读取数据寄存器*/
/* 寄存器地址宏定义 */
#define REG_SENSOR 0xB1
#define REG_LENGTH 0xB2
#define REG_READ   0xB3


void I2C_Error(void);
void I2C2_EV_IRQHandler(void);
void I2C2_ER_IRQHandler(void);
void EXTI15_10_IRQHandler(void) ;
unsigned char* Send_float_to_byte(float temp);
float Receive_byte_to_float(char* temp);


#endif


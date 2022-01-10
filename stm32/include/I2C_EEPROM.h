#ifndef __I2C_EEPROM__
#define __I2C_EEPROM__

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Private define ------------------------------------------------------------*/
#define SCL_H         GPIOB->BSRR = GPIO_Pin_6	 /* GPIO_SetBits(GPIOB , GPIO_Pin_6)   */
#define SCL_L         GPIOB->BRR  = GPIO_Pin_6   /* GPIO_ResetBits(GPIOB , GPIO_Pin_6) */

#define SDA_H         GPIOB->BSRR = GPIO_Pin_7	 /* GPIO_SetBits(GPIOB , GPIO_Pin_7)   */
#define SDA_L         GPIOB->BRR  = GPIO_Pin_7	 /* GPIO_ResetBits(GPIOB , GPIO_Pin_7) */

#define SCL_read      GPIOB->IDR  & GPIO_Pin_6   /* GPIO_ReadInputDataBit(GPIOB , GPIO_Pin_6) */
#define SDA_read      GPIOB->IDR  & GPIO_Pin_7	 /* GPIO_ReadInputDataBit(GPIOB , GPIO_Pin_7) */

#define I2C_PageSize  8  // 24C02每页8字节,总共256个字节;

#define ADDR_24LC02		0xA0   //EEPROM地址;

/* CPG参数存储地址定义 */
#define Speed 0X00
#define Log_degree_12 0X08
#define Log_degree_13 0X10
#define Amplitude_degree_1 0X18
#define Amplitude_degree_2 0X20
#define Amplitude_degree_3 0X28
#define Dynamic_Offset_degree_1 0X30
#define Dynamic_Offset_degree_2 0X38
#define Dynamic_Offset_degree_3 0X40
#define Static_Offset_degree_1 0X48
#define Static_Offset_degree_2 0X50
#define Static_Offset_degree_3 0X58


/* Private function prototypes -----------------------------------------------*/
FunctionalState I2C_WriteByte(unsigned char SendByte, unsigned int WriteAddress, unsigned char DeviceAddress);
FunctionalState I2C_ReadByte(char* pBuffer,   unsigned int length,   unsigned int ReadAddress,  unsigned char DeviceAddress);
void Clear_EEPROM(void);


#endif

#include "stm32f10x.h"
#include "I2C_EEPROM.h"
extern void delay(u32 count);

/*****************************************************************************/
//函数名称：I2C_delay
//函数功能：延时
/*****************************************************************************/
static void I2C_delay(void)
{
    unsigned char i = 50; /* 这里可以优化速度,经测试最低到5还能写入 */

    while(i)
    {
        i--;
    }
}

/*****************************************************************************/
//函数名称：I2C_Start
//函数功能：I2C起始信号
/*****************************************************************************/
static FunctionalState I2C_Start(void)
{
    SDA_H;
    SCL_H;
    I2C_delay();

    if(!SDA_read)return DISABLE;	/* SDA线为低电平则总线忙,退出 */

    SDA_L;
    I2C_delay();

    if(SDA_read) return DISABLE;	/* SDA线为高电平则总线出错,退出 */

    SDA_L;
    I2C_delay();
    return ENABLE;
}


/*****************************************************************************/
//函数名称：I2C_Stop
//函数功能：I2C停止信号
/*****************************************************************************/
static void I2C_Stop(void)
{
    SCL_L;
    I2C_delay();
    SDA_L;
    I2C_delay();
    SCL_H;
    I2C_delay();
    SDA_H;
    I2C_delay();
}


/*****************************************************************************/
//函数名称：I2C_Ack
//函数功能：I2C应答
/*****************************************************************************/
static void I2C_Ack(void)
{
    SCL_L;
    I2C_delay();
    SDA_L;
    I2C_delay();
    SCL_H;
    I2C_delay();
    SCL_L;
    I2C_delay();
}


/*****************************************************************************/
//函数名称：I2C_NoAck
//函数功能：I2C无应答
/*****************************************************************************/
static void I2C_NoAck(void)
{
    SCL_L;
    I2C_delay();
    SDA_H;
    I2C_delay();
    SCL_H;
    I2C_delay();
    SCL_L;
    I2C_delay();
}


/*****************************************************************************/
//函数名称：I2C_WaitAck
//函数功能：I2C等待应答
/*****************************************************************************/
static FunctionalState I2C_WaitAck(void)
{
    SCL_L;
    I2C_delay();
    SDA_H;
    I2C_delay();
    SCL_H;
    I2C_delay();

    if(SDA_read)
    {
        SCL_L;
        return DISABLE;
    }

    SCL_L;
    return ENABLE;
}


/*****************************************************************************/
//函数名称：I2C_SendByte
//函数功能：数据从高位到低位
/*****************************************************************************/
static void I2C_SendByte(uint8_t SendByte)
{
    unsigned char i = 8;

    while(i--)
    {
        SCL_L;
        I2C_delay();

        if(SendByte & 0x80)
            SDA_H;
        else
            SDA_L;

        SendByte <<= 1;
        I2C_delay();
        SCL_H;
        I2C_delay();
    }

    SCL_L;
}



/*****************************************************************************/
//函数名称：I2C_ReceiveByte
//函数功能：数据从高位到低位
/*****************************************************************************/
static uint8_t I2C_ReceiveByte(void)
{
    unsigned char i = 8;
    unsigned char ReceiveByte = 0;

    SDA_H;

    while(i--)
    {
        ReceiveByte <<= 1;
        SCL_L;
        I2C_delay();
        SCL_H;
        I2C_delay();

        if(SDA_read)
        {
            ReceiveByte |= 0x01;
        }
    }

    SCL_L;
    return ReceiveByte;
}



/*****************************************************************************/
//函数名称：I2C_WriteByte
//函数功能：写一字节数据
//SendByte: 待写入数据
//WriteAddress: 待写入地址
//DeviceAddress: 器件类型
//返回为:=1成功写入,=0失败
/*****************************************************************************/
FunctionalState I2C_WriteByte(unsigned char SendByte, unsigned int WriteAddress, unsigned char DeviceAddress)
{
    if(!I2C_Start())return DISABLE;

    I2C_SendByte(((WriteAddress & 0x0700) >> 7) | (DeviceAddress & 0xFFFE)); /*设置高起始地址+器件地址 */

    if(!I2C_WaitAck())
    {
        I2C_Stop();
        return DISABLE;
    }

    I2C_SendByte((unsigned char)(WriteAddress & 0x00FF));   /* 设置低起始地址 */
    I2C_WaitAck();
    I2C_SendByte(SendByte);
    I2C_WaitAck();
    I2C_Stop();
    /* 注意：因为这里要等待EEPROM写完，可以采用查询或延时方式(10ms)	*/
    /* Systick_Delay_1ms(10); */
    return ENABLE;
}


/*****************************************************************************/
//函数名称：I2C_ReadByte
//函数功能：读取一串数据
//pBuffer: 存放读出数据
//length: 待读出长度
//ReadAddress: 待读出地址
//DeviceAddress: 器件类型
//返回为:=1成功读入,=0失败
/*****************************************************************************/
FunctionalState I2C_ReadByte(char* pBuffer,   unsigned int length,   unsigned int ReadAddress,  unsigned char DeviceAddress)
{
    if(!I2C_Start())return DISABLE;

    I2C_SendByte(((ReadAddress & 0x0700) >> 7) | (DeviceAddress & 0xFFFE)); /* 设置高起始地址+器件地址 */

    if(!I2C_WaitAck())
    {
        I2C_Stop();
        return DISABLE;
    }

    I2C_SendByte((uint8_t)(ReadAddress & 0x00FF));   /* 设置低起始地址 */
    I2C_WaitAck();
    I2C_Start();
    I2C_SendByte(((ReadAddress & 0x0700) >> 7) | DeviceAddress | 0x0001);
    I2C_WaitAck();

    while(length)
    {
        *pBuffer = I2C_ReceiveByte();

        if(length == 1)I2C_NoAck();
        else I2C_Ack();

        pBuffer++;
        length--;
    }

    I2C_Stop();
    return ENABLE;
}

/*
********************************************************************************
** 函数名称 ： void Clear_EEPROM(void)
** 函数功能 ： 设置CPG运行初始化参数,从EEPROM读入初始化参数;
********************************************************************************
*/
void Clear_EEPROM(void)
{
    unsigned char i;

    for(i = 0; i < 255; i++)
    {
        I2C_WriteByte(0X00, i, ADDR_24LC02);
        delay(20000);
    }
}

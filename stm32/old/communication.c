#include "configuration.h"
#include "PWM_Calculation.h"
#include "delay.h"

char Buffer[BufferSize];  //数据缓冲区;
char *p = (&Buffer[0] + 2); //根据数据数据包格式，使指针P指向数组第三个地址，也就是传输的数据;
unsigned char *q;

u8 receive; //i2c接受数据临时保存
u8 reg;//标记要读写的是哪个寄存器
int check = 0xaa;//校验位
u8 count;//标记多字节发送中还剩多少字节没有发送
u8 send;//要发送的数据
u8 flag = 0; //标注是接受的是寄存器地址还是值
u8 j = 0;
unsigned char kk = 0;
unsigned char i;
unsigned char Send[4];
unsigned char sensor, length; //传感器编号，数据长度
unsigned char flag_iic_read = 0;	//IIC用到的标志位

extern unsigned char Bottom_Board_Data[24];



/*****************************************************************************/
//函数名称：I2C2_EV_IRQHandler
//函数功能：I2C2中断服务函数
/*****************************************************************************/
void I2C2_EV_IRQHandler(void)
{
    switch(I2C_GetLastEvent(I2C2))
    {
        /*从机接收事件*/
        //EV1：从机接收，地址匹配到
        case I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED:	//从机接收，地址匹配到;
            break;

        //EV2:从机开始接收数据
        case I2C_EVENT_SLAVE_BYTE_RECEIVED:	   //从机接收到数据字节;
            if(Data_buffer == I2C_ReceiveData(I2C2))
            {
                i = 0;
                flag_iic_read = 1;
                break;			//当接收到的是寄存器地址Data_buffer时，不存储;
            }
            else if(flag_iic_read == 1)
            {
                Buffer[i++] = I2C_ReceiveData(I2C2); //接收数据，暂存入数据缓冲区当中;

                if(i == BufferSize) //一包数据接收完毕;
                {
                    i = 0;
                    flag_iic_read = 0;
                }

                break;
            }

            else if(flag == FLAG_RECV_REG_ADDR) //先收到的是寄存器地址
            {
                switch(I2C_ReceiveData(I2C2)) // 判断选中的是哪个寄存器
                {
                    case REG_SENSOR:
                        reg = REG_SENSOR;
                        flag = FLAG_RECV_DATA; //下一个收到的字节是数据
                        break;

                    case REG_LENGTH:
                        reg = REG_LENGTH;
                        flag = FLAG_RECV_DATA; //下一个收到的字节是数据
                        break;

                    case REG_READ:
                        reg = REG_READ;
                        count = length + 2; //将length装载入count，以便进行发送计数。加3是因为要加上起始位、校验位、结束位。但是I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED会发掉起始位，所以count只需要加2就行了
                        flag = FLAG_SEND_DATA; //下一个字节开始向主机发送数据
                        break;

                    default:
                        I2C_Error();
                        break;
                }
            }
            else if(flag == FLAG_RECV_DATA) //根据选中的虚拟寄存器，向虚拟寄存器中写值
            {
                receive = I2C_ReceiveData(I2C2);

                switch(reg)
                {
                    case REG_SENSOR:
                        sensor = receive;
                        break;

                    case REG_LENGTH:
                        length = receive;
                        break;

                    default:
                        I2C_Error();
                        break;
                }

                flag = FLAG_RECV_REG_ADDR; //下一个收到的是寄存器地址
            }
            else
                I2C_Error();

            break;

        //EV4：从机检测到停止位
        case I2C_EVENT_SLAVE_STOP_DETECTED:    //检测到停止信号,打开I2C2中断;
            I2C_ClearFlag(I2C2, I2C_FLAG_STOPF);
            I2C_ITConfig( I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE );  //解包完成时，关闭I2C2中断;

            if((Buffer[0] == 0XFE) && Buffer[39] == 0XFA) //确认数据包的起始位和停止位;
            {
                I2C_ITConfig( I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE );  //在解包的时候，关闭I2C2中断;

                switch(Buffer[1])  //判断数据类型,采取不同的解包方法;
                {
                    case CPG_Command:

                        break;

                    case Static_Offset_1_Sum:

                        for(i = 0; i < 4; i++)
                        {
                            I2C_WriteByte(*(q + i), Static_Offset_degree_1 + i, ADDR_24LC02);
                            delay(20000);
                        }

                        break;

                    case Static_Offset_1_Sub:

                        for(i = 0; i < 4; i++)
                        {
                            I2C_WriteByte(*(q + i), Static_Offset_degree_1 + i, ADDR_24LC02);
                            delay(20000);
                        }

                        break;

                    case Static_Offset_2_Sum:

                        for(i = 0; i < 4; i++)
                        {
                            I2C_WriteByte(*(q + i), Static_Offset_degree_2 + i, ADDR_24LC02);
                            delay(20000);
                        }

                        break;

                    case Static_Offset_2_Sub:

                        for(i = 0; i < 4; i++)
                        {
                            I2C_WriteByte(*(q + i), Static_Offset_degree_2 + i, ADDR_24LC02);
                            delay(20000);
                        }

                        break;

                    case Static_Offset_3_Sum:

                        for(i = 0; i < 4; i++)
                        {
                            I2C_WriteByte(*(q + i), Static_Offset_degree_3 + i, ADDR_24LC02);
                            delay(20000);
                        }

                        break;

                    case Static_Offset_3_Sub:

                        for(i = 0; i < 4; i++)
                        {
                            I2C_WriteByte(*(q + i), Static_Offset_degree_3 + i, ADDR_24LC02);
                            delay(20000);
                        }

                        break;

                    case Clear_data:
                        Clear_EEPROM();
                        break;
                }

                for(kk = 0; kk < 40; kk++)	//主要是清除Buffer[]，以便于下次不再进入解包过程
                    Buffer[kk] = 0;
            }
            else
            {
                I2C_Error();
            }

            I2C_Configuration();  //每次数据传送完后,重新配置I2C,下次才能正确接收数据;
            break;

        /* 从机发送事件 */
        //EV1：从机发送地址匹配到
        case I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED:  /* EV1 */
            I2C_SendData(I2C2, 0xfe); //发送整个数据包的第一个字节，即起始位
            check = 0; //初始化
            break;

        //EV3：从机开始发送数据
        case I2C_EVENT_SLAVE_BYTE_TRANSMITTING:          /* EV3 */
            if(1 == count)
            {
                I2C_SendData(I2C2, 0xfa);
                count--;
            }

            if(2 == count) //发送校验位
            {
                I2C_SendData(I2C2, check);
                count--;
            }

            if(count > 2)	//发送正常数据
            {
                send = count - 2;
                I2C_SendData(I2C2, Bottom_Board_Data[j]);
                check = checkCRC(check, Bottom_Board_Data[j]);
                count--;
                j++;
            }

            if(0 == count)
            {
                flag = FLAG_RECV_REG_ADDR; //恢复到空闲状态，等待要操作的寄存器
                j = 0;
            }

            break;

        default:
            I2C_Configuration();
            break;

    }
}

/*****************************************************************************/
//函数名称：I2C2_ER_IRQHandler
//函数功能：I2C2错误中断
/*****************************************************************************/
void I2C2_ER_IRQHandler(void)
{
    switch (I2C_GetLastEvent(I2C2))
    {
        case I2C_EVENT_SLAVE_ACK_FAILURE:
            I2C_Configuration();   //错误重新配置;
            break;

        default:
            break;
    }
}



/*****************************************************************************/
//函数名称：EXTI15_10_IRQHandler(void)
//函数功能：外部中断线10的中断服务函数
/*****************************************************************************/
void EXTI15_10_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line10) != RESET)
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_8);
        EXTI_ClearITPendingBit(EXTI_Line10);	//清除中断线10
    }
}



/* 错误处理函数 */
void I2C_Error()
{
    ;
}


/*
********************************************************************************
** 函数名称 ： Send_float_to_byte   Receive_byte_to_float
** 函数功能 ： 数据转换
********************************************************************************
*/
unsigned char* Send_float_to_byte(float temp)
{
    union UFLOAT
    {
        float f;
        unsigned char byte[4];
    };
    union UFLOAT data;
    int i;
    data.f = temp;

    for(i = 0; i < 4; i++)
        Send[i] = data.byte[i];

    return Send;
}




float Receive_byte_to_float(char* temp)
{
    union UFLOAT
    {
        float f;
        char byte[4];
    };
    union UFLOAT data;
    char i;

    for(i = 0; i < 4; i++)
        data.byte[i] = *(temp + i);

    return data.f;
}


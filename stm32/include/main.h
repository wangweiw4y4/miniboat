#ifndef __MAIN_H
#define	__MAIN_H

#include "stm32f10x.h"
#include <stdarg.h>
#include "CRC.h"
#include "PWM_Calculation.h"

#define LatchActionMaxTimeSecond 1.40

float LatchingRotationalSPeed = 0;
unsigned char DeLatchFinishFlag = 0;
unsigned char LatchFinishFlag = 0;
int LatchActionTimeCount = 0;
char DeLatchFlag = 0;
char LatchFlag = 1;
unsigned char* Send_float_to_byte(float temp);
float Receive_byte_to_float(char* temp);
unsigned char Send[4];


void DeLatch(void)
{
    if(DeLatchFinishFlag == 0)
    {
        LatchActionTimeCount++;
        LatchingRotationalSPeed = -800;
        ServoMotor_PWM_Calculation(LatchingRotationalSPeed);

        if(LatchActionTimeCount == (int)(LatchActionMaxTimeSecond * 1000 / PWM_Period_ms))
        {
            DeLatchFinishFlag = 1;
            LatchActionTimeCount = 0;
        }
    }
    else
    {
        LatchingRotationalSPeed = -2;
        ServoMotor_PWM_Calculation(LatchingRotationalSPeed);
        DeLatchFlag = 0;
			  DeLatchFinishFlag = 0;
    }

}

void Latch(void)
{
    if(LatchFinishFlag == 0)
    {
        LatchActionTimeCount++;
        LatchingRotationalSPeed = 800;
        ServoMotor_PWM_Calculation(LatchingRotationalSPeed);

        if(LatchActionTimeCount == (int)(LatchActionMaxTimeSecond * 1000 / PWM_Period_ms))
        {
            LatchFinishFlag = 1;
            LatchActionTimeCount = 0;
        }
    }
    else
    {
        LatchingRotationalSPeed = -10;
        ServoMotor_PWM_Calculation(LatchingRotationalSPeed);
        LatchFlag = 0;
			  LatchFinishFlag = 0;
    }
}

/*
********************************************************************************
**Send_float_to_byte   Receive_byte_to_float
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


uint16_t Receive_byte_to_uint16(char* temp)
{
    union UFLOAT
    {
        uint16_t f;
        char byte[2];
    };
    union UFLOAT data;
    char i;

    for(i = 0; i < 2; i++)
        data.byte[i] = *(temp + i);

    return data.f;
}


int32_t Receive_byte_to_int32(char* temp)
{
    union UFLOAT
    {
        int32_t f;
        char byte[4];
    };
    union UFLOAT data;
    char i;

    for(i = 0; i < 4; i++)
        data.byte[i] = *(temp + i);

    return data.f;
}

uint32_t Receive_byte_to_uint32(char* temp)
{
    union UFLOAT
    {
        uint32_t f;
        char byte[4];
    };
    union UFLOAT data;
    char i;

    for(i = 0; i < 4; i++)
        data.byte[i] = *(temp + i);

    return data.f;
}


static char *itoa(int value, char *string, int radix)
{
    int     i, d;
    int     flag = 0;
    char    *ptr = string;

    /* This implementation only works for decimal numbers. */
    if (radix != 10)
    {
        *ptr = 0;
        return string;
    }

    if (!value)
    {
        *ptr++ = 0x30;
        *ptr = 0;
        return string;
    }

    /* if this is a negative value insert the minus sign. */
    if (value < 0)
    {
        *ptr++ = '-';

        /* Make the value positive. */
        value *= -1;
    }

    for (i = 10000; i > 0; i /= 10)
    {
        d = value / i;

        if (d || flag)
        {
            *ptr++ = (char)(d + 0x30);
            value -= (d * i);
            flag = 1;
        }
    }

    /* Null terminate the string. */
    *ptr = 0;

    return string;

} /* NCL_Itoa */


void USART_printf(USART_TypeDef* USARTx, uint8_t *Data, ...)
{
    const char *s;
    int d;
    char buf[16];

    va_list ap;
    va_start(ap, Data);

    while ( *Data != 0)     // 判断是否到达字符串结束符
    {
        if ( *Data == 0x5c )  //'\'
        {
            switch ( *++Data )
            {
                case 'r':							          //回车符
                    USART_SendData(USARTx, 0x0d);
                    Data ++;
                    break;

                case 'n':							          //换行符
                    USART_SendData(USARTx, 0x0a);
                    Data ++;
                    break;

                default:
                    Data ++;
                    break;
            }
        }
        else if ( *Data == '%')
        {
            //
            switch ( *++Data )
            {
                case 's':										  //字符串
                    s = va_arg(ap, const char *);

                    for ( ; *s; s++)
                    {
                        USART_SendData(USARTx, *s);

                        while( USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET );
                    }

                    Data++;
                    break;

                case 'd':										//十进制
                    d = va_arg(ap, int);
                    itoa(d, buf, 10);

                    for (s = buf; *s; s++)
                    {
                        USART_SendData(USARTx, *s);

                        while( USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET );
                    }

                    Data++;
                    break;

                default:
                    Data++;
                    break;
            }
        } /* end of else if */
        else USART_SendData(USARTx, *Data++);

        while( USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET );
    }
}












#endif

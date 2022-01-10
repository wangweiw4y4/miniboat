#include "infrared.h"
#include "stm32f10x.h"

#define ADC1_DR_Address    ((uint32_t)0x4001244C)  //ADC1 DR寄存器基地址
#define N 1000
#define M 3
vu16 ADCConvertedValue[N][M];	   //存放ADC转换结果  也是DMA的目标地址
vu16 After_filter[M];  		  //用于存放处理以后的结果
float vol[6];
float temp[3];
int   k;


/*
********************************************************************************
** 函数名称 ： Filter(void)
** 函数功能 ： 接口函数，读取AD采样的值
********************************************************************************
*/
void Filter(void)			//接口函数，读取AD值
{
    float sum = 0;
    u16 count;

    for(k = 0; k < 3; k++)
    {
        for(count = 0; count < N; count++)
        {
            sum += ADCConvertedValue[count][k];
        }

        After_filter[k] = sum / N;
        temp[k] = 3.3 * After_filter[k] / 0x0fff;
        vol[k] = temp[k] + 0.0026 * temp[k] + 0.0022; //拟合曲线

        sum = 0;
    }

    //电压电流的矫正函数，其中vol[0]存放的是红外数据
    vol[1] = (vol[1] - 2.5) / 0.18; //采集的舵机运行时的电流
    vol[2] = 4 * temp[2] + 0.26;	//电池的电压

}


/*
********************************************************************************
** 函数名称 ： IR_ADC_Config(void)
** 函数功能 ： AD采样和DMA初始化
********************************************************************************
*/
void IR_ADC_Config(void)
{
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;      //DMA初始化结构体声明

    /* DMA1 channel1 configuration ----------------------------------------------*/
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;    //DMA对应的外设基地址
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&ADCConvertedValue;   //内存存储基地址
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;	//DMA的转换模式为SRC模式，由外设搬移到内存
    DMA_InitStructure.DMA_BufferSize = N * M;		 //DMA缓存大小，1个,单位为DMA_MemoryDataSize
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	//接收一次数据后，设备地址禁止后移
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;	//关闭接收一次数据后，目标内存地址后移
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;  //定义外设数据宽度为16位
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;  //DMA搬数据尺寸，HalfWord就是为16位
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;   //转换模式，循环缓存模式。
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;	//DMA优先级高
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;		  //M2M模式禁用
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);      //使能通道11

    /* Enable DMA1 channel1 */
    DMA_Cmd(DMA1_Channel1, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    /* ADC1 configuration ------------------------------------------------------*/
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	   /* 独立模式 */
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;			   /* 连续多通道模式 */
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;	   /* 连续转换 */
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;  /* 转换不受外部触发*/
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;		       /* 右对齐 */
    ADC_InitStructure.ADC_NbrOfChannel = 3;					   /* 扫描通道数 */
    ADC_Init(ADC1, &ADC_InitStructure);

    /* ADC1 regular channel15 configuration */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);  /* 通道X,采用时间为55.5周期,1代表规则通道第1个 */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 2, ADC_SampleTime_55Cycles5);   /* 通道X,采用时间为55.5周期,1代表规则通道第1个 */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 3, ADC_SampleTime_55Cycles5);   /* 通道X,采用时间为55.5周期,1代表规则通道第1个 */

// ADC_SoftwareStartConvCmd(ADC1,ENABLE);    /* 使能转换开始 */
    /* Enable ADC1 DMA */
    ADC_DMACmd(ADC1, ENABLE);	  //ADC命令，使能
    /* Enable ADC1 */
    ADC_Cmd(ADC1, ENABLE);  //开启ADC1

    /* Enable ADC1 reset calibaration register */
    ADC_ResetCalibration(ADC1);	  //重新校准

    /* Check the end of ADC1 reset calibration register */
    while(ADC_GetResetCalibrationStatus(ADC1));  //等待重新校准完成

    /* Start ADC1 calibaration */
    ADC_StartCalibration(ADC1);		//开始校准

    /* Check the end of ADC1 calibration */
    while(ADC_GetCalibrationStatus(ADC1));	   //等待校准完成

    /* Start ADC1 Software Conversion */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);	//连续转换开始，ADC通过DMA方式不断的更新RAM区。

}


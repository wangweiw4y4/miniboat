#ifndef __INFRARED_H
#define __INFRARED_H

//红外传感器型号：GP2Y0A02YK0F 20-150cm 夏普红外线测距传感器

#define NUM 50 //红外传感器计算一次距离，读取原始数据的次数
#define DEG3			   //拟合一个三次的曲线

#ifdef DEG5
#define DEG  5
static float p[] = {-900.9, 8836.0, -34390.0, 66440.0, -64100.0, 25330.0};
#endif

#ifdef DEG7
#define DEG  7
static float p[] = {-11340.0, 165800.0, -1034000.0, 3566000.0, -7343000.0, 9024000.0, -6129000.0, 1775000.0};
#endif

#ifdef DEG3
#define DEG  3
static float p[] = {-193.2, 1214.0, -2788.0, 2630.0};
#endif

void IR_ADC_Config(void);
void Filter(void);

#endif

/**************************************************************************************************/
/******************************************Wei Wang************************************************/
/*******************************NEW IMU and New Protocal for ROS***********************************/
/*******************************SENSEable City Lab & CSAIL, MIT************************************/
/******************************************2017.07.30**********************************************/
/**************************************************************************************************/
#include "stm32f10x.h"
#include "configuration.h"
#include "PWM_Update.h"
#include "I2C_EEPROM.h"
#include "PWM_Calculation.h"
#include "delay.h"
#include "main.h"
#include "math.h"
#include "string.h"

#define STARTBYTE 0xAA
#define ENDBYTE 0xFC
#define ROBOT_ID 0x01
#define UARTHEARDER_NUMBER 5
#define GEARTOSPEEDRATE 4
#define BWFW_SCALE 0.6265     //0.00983/0.01569
#define A1 74.764
#define A2 -156.56
#define A3 159.09
#define A4 1522.3


//#define A1 55.535
//#define A2 180.89
//#define A3 320.34
//#define A4 1520

float LeftRotationalSPeed = 0;
float RightRotationalSPeed = 0;
float FrontRotationalSPeed = 0;
float RearRotationalSPeed = 0;

float RotationalSpeed[4] = { 0 };
int INTSPEED;
int UARTCOMMAND_TOTALNUMBER = UARTHEARDER_NUMBER;
char CommandDataLength;
char SensorDataLength;
struct
{
    char StartByte;
    char RobotID;
    char CommandType;
    char DataLength;
    /************Data Area**************/
    float ForceData[4];// The forces received from the higher-level controller
    char LatchingCommand[4];
    char SensorCommand[8];
    /************Data Area**************/
    char EndByte;
} ReceivingPacket;

struct
{
    char StartByte;
    char RobotID;
    char CommandType;
    char DataLength;
    /************Data Area**************/
    float GPSData[3];
    float IMUData[6];
    /************Data Area**************/
    char EndByte;
} SendingPacket;

extern int PWM_Value[4];
extern int SERVOMotor_PWM_Value;

unsigned char UpdateRateFlag;

char Thruster_Init_Flag = 0;  //
int Thruster_Init_TimeCount = 0;  //
char UARTSendCount = 0;

/*Parameters for Wireless Communication*/
unsigned char UART2CommandData;
unsigned char UART2CommandDataBuffer[100];
unsigned char UARTSendDataBuffer[200]; // The matrix to store the sending byte data
unsigned char UARTCommandCount = 0;	//
unsigned char UARTCommandFinishFlag = 0;	//
char temp4byte[4];
float FloatCommand;


/*Parameters for IMU BNO055*/
unsigned char IMUBYTENUMBER = 28;
char IMUBYTEDATA[28];
float RobotStateIMU[13];//Roll,pitch,yaw,wx,wy,wz,ax,ay,az

unsigned char IMUTestCount = 0;	//
unsigned char IMUCount = 0;	//
unsigned char IMUCommandFinishFlag;
unsigned char UART3CommandData;
unsigned char IMUResponceFlag;
unsigned char IMULengthFlag;
unsigned char IMUFloatCount = 0;	//


/*Parameters for Indoor GPS*/
unsigned char INDOORGPS_BYTENUMBER = 29;
char INDOORGPSBYTE[29];

unsigned char INDOORGPSBYTECount = 0;	//
unsigned char INDOORGPSCommandFinishFlag;
unsigned char UART1CommandData;
unsigned char INDOORGPSDestinationAddressFlag;
unsigned char INDOORGPSSecondByteFlag;
unsigned char INDOORGPSFrameTypeFlag;
int32_t RobotStateGPS[4];
uint16_t IndoorGPSFrameType;
char TempInt2byte[2];
char IndoorGPSIntDataCount = 0;

unsigned char* f2bdata;	  //
float ForceData[4];//The forces received from the higher-level controller
float StateData[9];//The state sent to the higher-level controller: Time, x, y, angular velocity, yaw, pitch, roll, linear ax, linear ay;
char StateDataSendingFreqFlag;
char State_Num = 9;
char ReceiveForceDataFlag = 0;
char StateCount = 0;
char FloatToByteCount = 0;
char FrameByteCount = 0;

char ReceiveForceCount = 0;
char ReceiveByteCount = 0;

int calibration_high_flag = 0;
int calibration_high_count = 0;
int calibration_low_flag = 0;
int calibration_low_count = 0;
int calibration_finish_flag = 0;

int maxspeed = 200;
int minspeed = 0;
int testspeed = 20;


void UARTCommandProcess(void)
{
    switch (ReceivingPacket.CommandType)
    {
        case 0x31:   // Receive the force of MPC control
            memcpy(&ReceivingPacket, UART2CommandDataBuffer, ReceivingPacket.DataLength + UARTHEARDER_NUMBER);

            //USART_printf(USART2, "Data Received %d, %d, %d, %d\r\n", (int)(ReceivingPacket.ForceData[0]),(int)(ReceivingPacket.ForceData[1]),(int)(ReceivingPacket.ForceData[2]), (int)(ReceivingPacket.ForceData[3]));
            for(ReceiveForceCount = 0; ReceiveForceCount < 4; ReceiveForceCount++)
            {
                /*********************************The force range is 0 to 1N**********************************************/
                if (ReceivingPacket.ForceData[ReceiveForceCount] > 0.0)
                {
                    if(ReceivingPacket.ForceData[ReceiveForceCount] > 1)
                        ReceivingPacket.ForceData[ReceiveForceCount] = 1;

                    RotationalSpeed[ReceiveForceCount] = A1 * pow(ReceivingPacket.ForceData[ReceiveForceCount], 3) + A2 * pow(ReceivingPacket.ForceData[ReceiveForceCount], 2) + A3 * ReceivingPacket.ForceData[ReceiveForceCount] + A4; // The force input is from 0 to 3N
                    RotationalSpeed[ReceiveForceCount] = RotationalSpeed[ReceiveForceCount] - 1500;
                }
                else if ( ReceivingPacket.ForceData[ReceiveForceCount] == 0.0)
                {
                    RotationalSpeed[ReceiveForceCount] = 0;//The last 5 reduces the errors when the force is small
                }
                else
                {
                    ReceivingPacket.ForceData[ReceiveForceCount] = 0;
                }

                /*********************************The force range is 0 to 1N**********************************************/
            }

            if(ReceivingPacket.LatchingCommand[0] == (char)(1))
            {
                LatchFlag = 1;
            }

            if(ReceivingPacket.LatchingCommand[0] == (char)(2))
            {
                DeLatchFlag = 1;
            }

            LeftRotationalSPeed = RotationalSpeed[0];
            RightRotationalSPeed = RotationalSpeed[1];
            FrontRotationalSPeed = RotationalSpeed[2];
            RearRotationalSPeed = RotationalSpeed[3];
            PWM_Calculation(LeftRotationalSPeed, RightRotationalSPeed, FrontRotationalSPeed, RearRotationalSPeed);


            break;

        default:
            break;
    }

}

int main(void)
{
    RCC_Configuration();
    GPIO_Configuration();
    TIM1_Configuration();
    TIM3_Configuration();
    USART_Configuration(); //UART initialize
    Systick_Configuration();//
    NVIC_Configuration();		//
    //BNO055_INIT();	//IMU Initialization
    //I2C_Configuration();	//
    //CPG_Configuration();   //
    //IR_ADC_Config();	//
    //EXTI_Configuration();	//
    PWM_Calculation(LeftRotationalSPeed, RightRotationalSPeed, FrontRotationalSPeed, RearRotationalSPeed);
    ServoMotor_PWM_Calculation(LatchingRotationalSPeed);

    while (1)
    {


        if (UpdateRateFlag == 1)	//Every 20ms UpdateRateFlag=1 to update the Motor signal.
        {
            if(LatchFlag == 1)
                Latch();

            if(DeLatchFlag == 1)
                DeLatch();

            if(calibration_finish_flag == 1)
            {

                /*
                ********************************************************************************
                ** SysTickHandler(void)
                **
                ********************************************************************************
                */


                /********************************UART2 Receiving command from or sending states to high-level controller  *************************************/
                /********************************UART2 Receiving command from or sending states to high-level controller  *************************************/
                if (UARTCommandFinishFlag == 1)
                {
                    UARTCommandProcess();
                    UARTCommandFinishFlag = 0;
                }

                /********************************UART2 Receiving command from or sending states to high-level controller  *************************************/
                /********************************UART2 Receiving command from or sending states to high-level controller  *************************************/



                /********************************* State data sending to the High-level controller***************************************/
                /********************************* State data sending to the High-level controller***************************************/
                /********************************* State data sending to the High-level controller***************************************/
                StateDataSendingFreqFlag++;

                if (StateDataSendingFreqFlag == 5) // Send the states to high level controller at 10 Hz
                {
                    SendingPacket.StartByte = STARTBYTE;
                    SendingPacket.RobotID = ROBOT_ID;
                    SendingPacket.CommandType = 0x32;// Send the robot state to the higher level controller
                    SendingPacket.EndByte = ENDBYTE;
                    SendingPacket.DataLength = sizeof(SendingPacket.IMUData) + sizeof(SendingPacket.GPSData);
                    memcpy(&UARTSendDataBuffer, &SendingPacket.StartByte, SendingPacket.DataLength + UARTHEARDER_NUMBER);

                    //Sending the Robot States to the high-level controller
                    for (FrameByteCount = 0; FrameByteCount < SendingPacket.DataLength + UARTHEARDER_NUMBER; FrameByteCount++)
                    {
                        USART_SendData(USART2, UARTSendDataBuffer[FrameByteCount]);

                        while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
                    }

                    //USART_printf(USART2,"Total length is %d\r\n",SendingPacket.DataLength+UARTHEARDER_NUMBER);
                    StateDataSendingFreqFlag = 0;
                }

                /********************************* State data sending to the High-level controller***************************************/
                /********************************* State data sending to the High-level controller***************************************/
                /********************************* State data sending to the High-level controller***************************************/
            }
            else
            {
                if(calibration_high_flag == 1)
                {
                    calibration_low_count = calibration_low_count + 1;
                    LeftRotationalSPeed = minspeed;
                    RightRotationalSPeed = minspeed;
                    FrontRotationalSPeed = minspeed;
                    RearRotationalSPeed = minspeed;
                    PWM_Calculation(LeftRotationalSPeed, RightRotationalSPeed, FrontRotationalSPeed, RearRotationalSPeed);

                    if (calibration_low_count == 500)
                        calibration_finish_flag = 1;

                    UpdateRateFlag = 0;
                }
                else
                {
                    calibration_high_count = calibration_high_count + 1;
                    LeftRotationalSPeed = maxspeed;
                    RightRotationalSPeed = maxspeed;
                    FrontRotationalSPeed = maxspeed;
                    RearRotationalSPeed = maxspeed;
                    PWM_Calculation(LeftRotationalSPeed, RightRotationalSPeed, FrontRotationalSPeed, RearRotationalSPeed);

                    if(calibration_high_count == 500)
                        calibration_high_flag = 1;

                    UpdateRateFlag = 0;
                }

            }

            UpdateRateFlag = 0;
        }//if (UpdateRateFlag == 1)
    }//while (1)
}

/*
********************************************************************************
** SysTickHandler(void)
**
********************************************************************************
*/
void SysTick_Handler(void)
{
    PWM_ServoMotor(SERVOMotor_PWM_Value);
    PWM_LEFT(PWM_Value[0]);
    PWM_RIGHT(PWM_Value[1]);
    PWM_FRONT(PWM_Value[2]);
    PWM_REAR(PWM_Value[3]);
    UpdateRateFlag = 1;
}


/*
********************************************************************************
Receive Data from Indoor GPS
********************************************************************************
*/
void USART1_IRQHandler(void)                	//
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //
    {
        USART_ClearFlag(USART1, USART_FLAG_RXNE);	  	  		//
        UART1CommandData = USART_ReceiveData(USART1); 	//

        if(INDOORGPSFrameTypeFlag == 1)
        {
            INDOORGPSBYTE[INDOORGPSBYTECount++] = UART1CommandData;

            if(INDOORGPSBYTECount == INDOORGPS_BYTENUMBER)
            {
                INDOORGPSFrameTypeFlag = 0;
                INDOORGPSBYTECount = 0;
                INDOORGPSCommandFinishFlag = 1;
            }
        }
        else if(INDOORGPSDestinationAddressFlag == 1 && INDOORGPSSecondByteFlag == 1)
        {
            INDOORGPSBYTE[INDOORGPSBYTECount++] = UART1CommandData;

            if(INDOORGPSBYTECount == 4)
            {
                TempInt2byte[0] = INDOORGPSBYTE[2];
                TempInt2byte[1] = INDOORGPSBYTE[3];
                IndoorGPSFrameType = Receive_byte_to_uint16(TempInt2byte);

                if(IndoorGPSFrameType == 0x0011)
                {
                    INDOORGPSDestinationAddressFlag = 0;
                    INDOORGPSSecondByteFlag = 0;
                    INDOORGPSFrameTypeFlag = 1;
                }
                else
                {
                    INDOORGPSDestinationAddressFlag = 0;
                    INDOORGPSSecondByteFlag = 0;
                    INDOORGPSFrameTypeFlag = 0;
                    INDOORGPSBYTECount = 0;
                }
            }
        }
        else if(INDOORGPSDestinationAddressFlag == 1)
        {
            if(UART1CommandData == 0x47) //The length of the IMU DATA
            {
                INDOORGPSSecondByteFlag = 1;
                INDOORGPSBYTE[INDOORGPSBYTECount++] = UART1CommandData;
            }
            else
            {
                INDOORGPSDestinationAddressFlag = 0;
                INDOORGPSBYTECount = 0;
            }
        }
        else if(UART1CommandData == 0xFF) //The Destination Address of the Indoor GPS DADA
        {
            INDOORGPSDestinationAddressFlag = 1;
            INDOORGPSBYTE[INDOORGPSBYTECount++] = UART1CommandData;
        }

    }
}

/*
********************************************************************************
Receive Wireless Command
********************************************************************************
*/
void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //
    {
        USART_ClearFlag(USART2, USART_FLAG_RXNE);	  	  		//
        UART2CommandData = USART_ReceiveData(USART2); 							  	 //
        UART2CommandDataBuffer[UARTCommandCount++] = UART2CommandData;	//

        if (UARTCommandCount == UARTCOMMAND_TOTALNUMBER) // If recive all the byte data in the packet
        {
            UARTCommandCount = 0; //Recieive all the data, reset UARTCommandCount flat to be ready for reciving next packet
            ReceivingPacket.StartByte = UART2CommandDataBuffer[0];
            ReceivingPacket.EndByte = UART2CommandDataBuffer[UARTCOMMAND_TOTALNUMBER - 1];
            ReceivingPacket.RobotID = UART2CommandDataBuffer[1];
            ReceivingPacket.CommandType = UART2CommandDataBuffer[2];
            ReceivingPacket.DataLength = UART2CommandDataBuffer[3];

            if ((ReceivingPacket.StartByte == STARTBYTE) && (ReceivingPacket.EndByte == ENDBYTE))
            {
                if (ReceivingPacket.RobotID == ROBOT_ID)
                    UARTCommandFinishFlag = 1;
            }

            UARTCOMMAND_TOTALNUMBER = UARTHEARDER_NUMBER; //Reset the UARTCOMMAND_TOTALNUMBER to UARTHEARDER_NUMBER.
        }
        else if (UARTCommandCount == (UARTHEARDER_NUMBER - 1))
        {
            CommandDataLength = UART2CommandData;
            UARTCOMMAND_TOTALNUMBER = UARTHEARDER_NUMBER + CommandDataLength;
        }
    }
}
/*
********************************************************************************
** Function: Reading Data from IMU INO055
********************************************************************************
*/
void USART3_IRQHandler(void)                	//
{
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  //
    {
        USART_ClearFlag(USART3, USART_FLAG_RXNE);	  	  		//
        UART3CommandData = USART_ReceiveData(USART3);

        if(IMUResponceFlag == 1 && IMULengthFlag == 1)
        {
            IMUBYTEDATA[IMUCount++] = UART3CommandData;

            if(IMUCount == IMUBYTENUMBER)
            {
                IMUCount = 0;
                IMUResponceFlag = 0;
                IMULengthFlag = 0;
                IMUCommandFinishFlag = 1;
            }
        }
        else
        {
            if(IMUResponceFlag == 1)
            {
                if(UART3CommandData == (IMUBYTENUMBER - 2)) //The length of the IMU DATA
                {
                    IMULengthFlag = 1;
                    IMUBYTEDATA[IMUCount++] = UART3CommandData;
                }
            }
            else if(UART3CommandData == 0xBB) //THe ResponseByte of the IMU DADA
            {
                IMUResponceFlag = 1;
                IMUBYTEDATA[IMUCount++] = UART3CommandData;
            }
        }
    }
}

#ifndef MY_SERIAL_NODE_H
#define MY_SERIAL_NODE_H

#include <string>
#include <cstring>

#define STARTBYTE 0xAA
#define ENDBYTE 0xFC
#define ROBOT_ID 0x01         // Robot ID Number
#define UARTHEARDER_NUMBER 5  // Header Length of the Pakage
#define MPCCOMMAND 0x31;

unsigned char* Send_float_to_byte(float temp);
float Receive_byte_to_float(char* temp);
uint16_t Receive_byte_to_uint16(char* temp);
int32_t Receive_byte_to_int32(char* temp);
uint32_t Receive_byte_to_uint32(char* temp);

unsigned char Send[4];

/*
********************************************************************************
** Function Name: Send_float_to_byte   Receive_byte_to_float ...
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
  for (i = 0; i < 4; i++)
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
  for (i = 0; i < 4; i++)
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
  for (i = 0; i < 2; i++)
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
  for (i = 0; i < 4; i++)
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
  for (i = 0; i < 4; i++)
    data.byte[i] = *(temp + i);
  return data.f;
}

static struct
{
  unsigned char StartByte = STARTBYTE;
  unsigned char RobotID = ROBOT_ID;
  unsigned char CommandType;
  unsigned char DataLength;
  /************Data Area**************/
  float ForceData[4];  // The forces received from the higher-level controller
  unsigned char LatchingCommand[4];
  unsigned char SensorCommand[8];
  /************Data Area**************/
  char EndByte = ENDBYTE;
} SendingPacket;

static struct
{
  unsigned char StartByte;
  unsigned char RobotID;
  unsigned char CommandType;
  unsigned char DataLength;
  /************Data Area**************/
  float GPSData[3];
  float IMUData[6];
  /************Data Area**************/
  unsigned char EndByte;
} ReceivingPacket;

#endif

// ROS
#include "ros/ros.h"
#include <ros/console.h>

// roboat
#include <roboat_core/serial_node.h>
#include <roboat_core/SensorFromMicroController.h>
#include <roboat_core/CommandToMicroController.h>

// other
#include <serial/serial.h>

size_t CommandFrameSize;
size_t SendFramesize = 33;

serial::Serial ros_ser;

std::string port;
double step;

// Callback function for command topics
void callback(roboat_core::CommandToMicroController Command)
{
  ROS_INFO("[SERIAL NODE]Write:%x,%x,%x,%x\n", Command.CommandtoLower[0], Command.CommandtoLower[1],
           Command.CommandtoLower[2], Command.CommandtoLower[32]);
  CommandFrameSize = ros_ser.write(&Command.CommandtoLower[0], SendFramesize);
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "serial_node");
  ros::NodeHandle n;
  // subcribe command topic
  ros::Subscriber command_sub = n.subscribe("command", 10, callback);
  // publish sensor topic
  ros::Publisher sensor_pub = n.advertise<roboat_core::SensorFromMicroController>("sensor", 10);

  n.param("port/serial", port, std::string("/dev/ttyAMA0"));
  n.param("system_dynamics/step", step, 0.1);

  try
  {
    ros_ser.setPort(port);
    ros_ser.setBaudrate(115200);
    serial::Timeout to = serial::Timeout::simpleTimeout(1000);
    ros_ser.setTimeout(to);
    ros_ser.open();
  }
  catch (serial::IOException& e)
  {
    ROS_ERROR_STREAM("[SERIAL NODE]Unable to open port");
    return -1;
  }

  if (ros_ser.isOpen())
  {
    ROS_INFO_STREAM("[SERIAL NODE]Serial Port opened");
  }
  else
  {
    return -1;
  }

  ros::Rate loop_rate(1/step);
  while (ros::ok())
  {
    if (ros_ser.available())
    {
      //ROS_INFO("[SERIAL NODE]loop - serial available");
      roboat_core::SensorFromMicroController SensorData;

      size_t SensorFrameSize;
      size_t ReceiveFrameSize = 41;
      uint8_t SensorFrame[ReceiveFrameSize];
      SensorFrameSize = ros_ser.read(SensorFrame, ReceiveFrameSize);
      ReceivingPacket.StartByte = SensorFrame[0];
      ReceivingPacket.EndByte = SensorFrame[ReceiveFrameSize - 1];
      ReceivingPacket.RobotID = SensorFrame[1];
      ReceivingPacket.CommandType = SensorFrame[2];
      ReceivingPacket.DataLength = SensorFrame[3];
      memcpy(&ReceivingPacket, SensorFrame, ReceivingPacket.DataLength + UARTHEARDER_NUMBER);

      if ((ReceivingPacket.StartByte == STARTBYTE) && (ReceivingPacket.EndByte == ENDBYTE) &&
          (ReceivingPacket.RobotID == ROBOT_ID))
      {
        //ROS_INFO("[SERIAL NODE]GPS and IMU data:%f,%f,%f,%f,%f,%f,%f,%f\n", ReceivingPacket.GPSData[1],
        //         ReceivingPacket.GPSData[2], ReceivingPacket.IMUData[0], ReceivingPacket.IMUData[1],
         //        ReceivingPacket.IMUData[2], ReceivingPacket.IMUData[3], ReceivingPacket.IMUData[4],
         //        ReceivingPacket.IMUData[5]);

        memcpy(&SensorData.SensorDataFromLower[0], &SensorFrame[UARTHEARDER_NUMBER - 1], ReceivingPacket.DataLength);
        sensor_pub.publish(SensorData);
      }
    }
    else
    {
      //ROS_ERROR_STREAM("[SERIAL NODE]loop - serial not available");
    }

    ros::spinOnce();
    loop_rate.sleep();
  }
}

#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "ros/ros.h"
#include <std_msgs/Float64MultiArray.h>
#include <sensor_msgs/Joy.h>

#include <roboat_core/CommandToMicroController.h>
#include <roboat_core/Force.h>
#include <roboat_core/model_identification.h>
#include <roboat_msgs/ThrustState.h>


class Command
{
private:
  roboat_core::CommandToMicroController commandMsg(double* force, unsigned char* latch);
  ros::Publisher command_pub;
  ros::Publisher command_force_pub;
  ros::Publisher thrust_state_pub;
  ros::Publisher latch_override_pub;
  ros::Publisher latch_pub;
  int command_priority, stop_force_priority = 3, calibrating_priority = 4;
  unsigned char latchingaction[1] = {0}; 
  double force[4]; 
  double joy_max_force, force_available;
  const int pid_priority = 1;
  const int mpc_priority = 2;
  double stop_force[4] = {0, 0, 0, 0};
  
  ModelIdentification model_;

  int missed_beats_, max_missed_beats_=4;

  void forceCallback(const roboat_core::Force::ConstPtr& msg, int priority);
  void joyCallback(const sensor_msgs::Joy msg);

public:
  Command(ros::NodeHandle n);
};

#endif

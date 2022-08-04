#include "ros/ros.h"
#include <ros/package.h>
#include <sensor_msgs/Joy.h>
#include <std_msgs/UInt16.h>

#include <boost/bind.hpp>
#include <boost/ref.hpp>

// roboat
#include <roboat_core/CommandToMicroController.h>
#include <roboat_core/Force.h>
#include <roboat_core/serial_node.h>
#include <roboat_core/Command.hpp>

roboat_core::CommandToMicroController Command::commandMsg(double *force, unsigned char *latch)
{
  roboat_core::CommandToMicroController command;

  // Create the serial SendingPacket to send to the lower level controller
  SendingPacket.CommandType = MPCCOMMAND;
  SendingPacket.DataLength =
      sizeof(SendingPacket.ForceData) + sizeof(SendingPacket.LatchingCommand) + sizeof(SendingPacket.SensorCommand);
  for (int i = 0; i < 4; i++)
    SendingPacket.ForceData[i] = (float)force[i];
  SendingPacket.LatchingCommand[0] = latch[0];

  // Copy the SendingPacket Data to the command data
  memcpy(&command.CommandtoLower[0], &SendingPacket, SendingPacket.DataLength + UARTHEARDER_NUMBER);

  return command;
}

void Command::forceCallback(const roboat_core::Force::ConstPtr &msg, int priority)
{

  if (priority < command_priority)
  {
    command_priority = priority;
    std::copy(std::begin(msg->data), std::end(msg->data), std::begin(force));
  }

  ROS_DEBUG("control force received with priority %d", priority);
}
// latch = 1, delatch = 2, otherwise 0 (no action).
void Command::joyCallback(sensor_msgs::Joy msg)
{
  if (msg.buttons[4] == 1)
  {
    if (msg.axes[1] > 0)
    {
      force[0] = Command::joy_max_force * msg.axes[1];
      force[1] = Command::joy_max_force * msg.axes[1];
      if (msg.axes[0] > 0)
      {
        force[2] = Command::joy_max_force * msg.axes[0] * 0.25;
        force[3] = 0;
      }
      else
      {
        force[2] = 0;
        force[3] = -Command::joy_max_force * msg.axes[0] * 0.25;
      }
    }

    else
    {
      force[2] = -Command::joy_max_force * msg.axes[1];
      force[3] = -Command::joy_max_force * msg.axes[1];
      if (msg.axes[0] > 0)
      {
        force[0] = 0;
        force[1] = Command::joy_max_force * msg.axes[0] * 0.25;
      }
      else
      {
        force[0] = -Command::joy_max_force * msg.axes[0] * 0.25;
        force[1] = 0;
      }
    }
    if (msg.buttons[3] == 1)
      latchingaction[0] = 1;
    else if (msg.buttons[0] == 1)
      latchingaction[0] = 2;
    else
      latchingaction[0] = 0;
    // start using the joypad command force
    command_priority = 0;
    ROS_DEBUG("[COMMAND_NODE] joypad force received start");
  }
  else
  {
    // stop using the joypad command force
    command_priority = stop_force_priority;
    ROS_DEBUG("[COMMAND_NODE] joypad force received end");
  }
}

void Command::latchCallback(std_msgs::UInt16 msg)
{
  latchingaction[0] = msg.data;
  if (latchingaction[0] == prev_latchingaction[0]){
    latchingaction[0] = 0;
  }
  prev_latchingaction[0] = msg.data;
}

Command::Command(ros::NodeHandle n)
{
  Command::joy_max_force = 1.0;

  // publisher for command topic
  command_pub = n.advertise<roboat_core::CommandToMicroController>("command", 1);
  command_force_pub = n.advertise<roboat_core::Force>("command_force", 1);
  thrust_state_pub = n.advertise<roboat_msgs::ThrustState>("thrust_state", 1);
  latch_pub = n.advertise<std_msgs::UInt16>("latching_open_close_int", 1);
  latch_override_pub = n.advertise<std_msgs::UInt16>("latching_override_int", 1);

  // force subscriber topics for PF, PID, MPC and joypad
  const int pf_priority = 1, pid_priority = 2, mpc_priority = 3;
  ros::Subscriber joy_sub = n.subscribe("joy", 10, &Command::joyCallback, this);
  ros::Subscriber pid_sub = n.subscribe<roboat_core::Force>("pid_force", 1, boost::bind(&Command::forceCallback, this, _1, pid_priority));
  ros::Subscriber mpc_sub = n.subscribe<roboat_core::Force>("mpc_force", 1, boost::bind(&Command::forceCallback, this, _1, mpc_priority));
  ros::Subscriber pf_sub = n.subscribe<roboat_core::Force>("pf_force", 1, boost::bind(&Command::forceCallback, this, _1, pf_priority));
  ros::Subscriber latching_sub = n.subscribe("latching", 10, &Command::latchCallback, this);
  ros::Rate loopRate(50);

  if (n.hasParam("joypad/max_force"))
    n.getParam("joypad/max_force", Command::joy_max_force);

  while (ros::ok())
  {
    ros::spinOnce();

    /** set command force in order of priority **/
    ROS_DEBUG("Command priority = %d", command_priority);

    roboat_core::Force command_force_msg;
    std::copy(std::begin(force), std::end(force), std::begin(command_force_msg.data));
    command_force_pub.publish(command_force_msg);
    command_pub.publish(commandMsg(force, latchingaction));

    // reset command to stop as default if joypad is not in use
    if (command_priority != 0)
    {
      command_priority = stop_force_priority;
      std::copy(std::begin(stop_force), std::end(stop_force), std::begin(force));
    }

    roboat_msgs::ThrustState thrust_state_msg;
    thrust_state_msg.priority = command_priority;
    thrust_state_pub.publish(thrust_state_msg);

    loopRate.sleep();
  }
}

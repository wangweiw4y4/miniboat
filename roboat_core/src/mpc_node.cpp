// ROS
#include "ros/ros.h"

// Roboat
#include <roboat_core/MPC.hpp>

int main(int argc, char** argv)
{
  ros::init(argc, argv, "mpc_node");

  ros::NodeHandle n;

  MPC node(n);
}

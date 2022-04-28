#include "ros/ros.h"

#include <roboat_core/Command.hpp>

int main(int argc, char **argv)
{
  ros::init(argc, argv, "command_node");
  ros::NodeHandle n;
  Command node(n);
  return 0;
}

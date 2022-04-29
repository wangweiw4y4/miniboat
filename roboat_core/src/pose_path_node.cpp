#include "ros/ros.h"

#include <roboat_core/PosePath.hpp>

int main(int argc, char **argv)
{
  ros::init(argc, argv, "pose_path_node");
  ros::NodeHandle n;
  ros::NodeHandle nh("~");
  PosePath node(n, nh);
  return 0;
}

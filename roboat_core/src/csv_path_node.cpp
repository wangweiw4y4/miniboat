#include "ros/ros.h"

#include <roboat_core/CsvPath.hpp>

int main(int argc, char **argv)
{
  ros::init(argc, argv, "csv_path_node");
  ros::NodeHandle n;
  CsvPath node(n);
  return 0;
}

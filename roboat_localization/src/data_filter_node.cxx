// Copyright (c) 2021. MIT & AMS institute - Roboat.
#include <ros/ros.h>



int main(int argc, char **argv) {
  ros::init(argc, argv, "roboat_filter");

  ROS_INFO("\033[1;32m----> roboat_filter: Data Filter.\033[0m");

  ros::MultiThreadedSpinner spinner(5);
  spinner.spin();

  return 0;
}

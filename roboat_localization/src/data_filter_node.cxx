// Copyright (c) 2021. MIT & AMS institute - Roboat.
#include "roboat_localization/data_filter.h"

int main(int argc, char **argv) {
  ros::init(argc, argv, "data_filter");

  DataFilter DF;

  ROS_INFO("\033[1;32m----> roboat_localization: data filter.\033[0m");

  ros::MultiThreadedSpinner spinner(4);
  spinner.spin();

  return 0;
}

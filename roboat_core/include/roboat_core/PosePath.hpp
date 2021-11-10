#ifndef POSEPATH_HPP
#define POSEPATH_HPP

#include "ros/ros.h"
#include <roboat_core/Path.hpp>

class PosePath : public Path
{
private:

  // ros params
  double  x, y, theta, sim_time;
  bool repeat;

public:
  PosePath(ros::NodeHandle n, ros::NodeHandle nh);
};

#endif

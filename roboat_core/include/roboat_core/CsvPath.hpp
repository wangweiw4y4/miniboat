#ifndef CSVPATH_HPP
#define CSVPATH_HPP

#include "ros/ros.h"
#include <roboat_core/Path.hpp>

class CsvPath : public Path
{
private:
  // ros params
  double sim_time;
  bool repeat;
  std::string relativeFileName;

public:
  CsvPath(ros::NodeHandle n);
};

#endif

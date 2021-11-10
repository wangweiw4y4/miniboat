#ifndef PATH_HPP
#define PATH_HPP

#include "ros/ros.h"
#include <nav_msgs/Path.h>

class Path
{
protected:
  int num_steps;
  double step;
  double angle_rotate=0, cos_angle_rotate=1, sin_angle_rotate=0;
  double init_x=0, init_y=0;
  bool yaw_tracking=true;

  ros::Publisher path_pub;
  ros::Publisher ref_pose_pub;

  nav_msgs::Path pathMsg(double* trajectory);
  void setRotation(double angle);
  double* trajectoryPointSet(double* trajectory, int refNum, double x, double y, double theta);
};

#endif

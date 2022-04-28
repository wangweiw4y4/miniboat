#include "ros/ros.h"
#include "ros/package.h"
#include <nav_msgs/Path.h>
#include <geometry_msgs/PoseStamped.h>
#include <tf/transform_datatypes.h>

#include "roboat_core/Path.hpp"

#include "math.h"
#include "csv.h"

void Path::setRotation(double angle)
{
  angle_rotate = angle;
  cos_angle_rotate = cos(angle_rotate);
  sin_angle_rotate = sin(angle_rotate);
}

double *Path::trajectoryPointSet(double *trajectory, int refNum, double x, double y, double theta)
{
  trajectory[3 * refNum] = init_x + x * cos_angle_rotate - y * sin_angle_rotate;
  trajectory[3 * refNum + 1] = init_y + x * sin_angle_rotate + y * cos_angle_rotate;
  if (yaw_tracking)
  {
    trajectory[3 * refNum + 2] = theta + angle_rotate;
  }
  return trajectory;
}

nav_msgs::Path Path::pathMsg(double *trajectory, std::string nmpc_frame)
{
  nav_msgs::Path msg;
  msg.header.frame_id = nmpc_frame;

  for (int i = 0; i < num_steps; i++)
  {
    geometry_msgs::PoseStamped pose;
    pose.header.frame_id = nmpc_frame;
    pose.pose.position.x = trajectory[3 * i];
    pose.pose.position.y = trajectory[3 * i + 1];
    pose.pose.orientation = tf::createQuaternionMsgFromYaw(trajectory[3 * i + 2]);
    msg.poses.push_back(pose);
  }

  return msg;
}

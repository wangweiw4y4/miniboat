#include "ros/ros.h"
#include "ros/package.h"
#include <nav_msgs/Path.h>
#include <geometry_msgs/PoseStamped.h>
#include <tf/transform_datatypes.h>

#include "roboat_core/PosePath.hpp"

#include "math.h"
#include "csv.h"

PosePath::PosePath(ros::NodeHandle n, ros::NodeHandle nh)
{
  path_pub = n.advertise<nav_msgs::Path>("roboat_path", 1);
  ref_pose_pub = n.advertise<geometry_msgs::PoseStamped>("ref_pose", 1);

  n.param("system_dynamics/step", step, 0.1);
  n.param("system_dynamics/num_steps", num_steps, 40);
  nh.getParam("x", x);
  nh.getParam("y", y);
  nh.getParam("theta", theta);
  nh.param("sim_time", sim_time, 5.);
  nh.param("repeat", repeat, true);

  double trajectory[3 * num_steps];

  for (int i = 0; i < num_steps; i++)
  {
    trajectoryPointSet(trajectory, i, x, y, theta);
  }

  nav_msgs::Path path_msg = pathMsg(trajectory);
  ros::Rate loopRate(1 / step);
  ros::Time begin = ros::Time::now();

  while (ros::ok())
  {
    path_pub.publish(path_msg);
    ref_pose_pub.publish(path_msg.poses[0]);
    loopRate.sleep();
    if (!repeat && sim_time < (ros::Time::now() - begin).toSec())
    {
      break;
    }
  }
}

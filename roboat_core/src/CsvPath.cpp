#include "ros/ros.h"
#include "ros/package.h"
#include <nav_msgs/Path.h>
#include <geometry_msgs/PoseStamped.h>
#include <tf/transform_datatypes.h>

#include "roboat_core/CsvPath.hpp"

#include "math.h"
#include "csv.h"

CsvPath::CsvPath(ros::NodeHandle n)
{
  path_pub = n.advertise<nav_msgs::Path>("roboat_path", 1);
  ref_pose_pub = n.advertise<geometry_msgs::PoseStamped>("ref_pose", 1);

  double angle;

  n.param("trajectory/yaw_tracking", yaw_tracking, true);
  n.param("trajectory/sim_time", sim_time, 30.);
  n.param("trajectory/init_x", init_x, 0.);
  n.param("trajectory/init_y", init_y, 0.);
  n.param("trajectory/angle_rotate", angle, 0.);
  n.param("trajectory/file", relativeFileName, std::string("/paths/miniboat_shapeshift_miniboat4.csv"));
  n.param("trajectory/repeat", repeat, false);
  n.param("system_dynamics/step", step, 0.1);
  n.param("system_dynamics/num_steps", num_steps, 20);

  setRotation(angle);
  
  ROS_DEBUG("[CSV_PATH_NODE] relative filename: %s", relativeFileName.c_str()); 
  
  std::string fileName = ros::package::getPath("roboat_core") + relativeFileName;
  
  ROS_DEBUG("[CSV_PATH_NODE] filename: %s", fileName.c_str()); 
  
  double trajectory[3 * num_steps], x, y, theta;
  bool started = false;

  while(ros::ok() && (repeat || !started)){

    started = true;

    io::CSVReader<3> in(fileName);
    in.read_header(io::ignore_extra_column, "x", "y", "theta");
    
    ROS_DEBUG("[CSV_PATH_NODE] start file read with header"); 
  
    for (int i = 0; i < num_steps; i++)
    {
      if (in.read_row(x, y, theta))
      {
        trajectoryPointSet(trajectory, i, x, y, theta);
      }
      else
      {
        break;
      }
    }
  
    ros::Rate loopRate(1/step);
    ros::Time begin = ros::Time::now();
  
    while (ros::ok() && in.read_row(x, y, theta))
    {
      ros::spinOnce();
  
      if ((ros::Time::now() - begin).toSec() > sim_time)
        break;
  
      for (int i = 0; i < 3 * num_steps; i++)
        trajectory[i] = trajectory[i + 3];
  
      trajectoryPointSet(trajectory, num_steps-1, x, y, theta);
  
      nav_msgs::Path path_msg = pathMsg(trajectory);
      path_pub.publish(path_msg);
  
      // publish pose representation of state
      ref_pose_pub.publish(path_msg.poses[0]);
  
      loopRate.sleep();
    }
  }
}


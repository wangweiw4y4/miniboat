// Copyright 2021 MIT, all rights reserved
#ifndef ROBOAT_VISUALIZATION_H
#define ROBOAT_VISUALIZATION_H

#include <ros/ros.h>

#include <tf/tf.h>
#include <tf/transform_listener.h>
#include <tf/transform_broadcaster.h>

#include <nav_msgs/Path.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/PoseStamped.h>
#include <roboat_msgs/ThrustState.h>
#include <roboat_core/Force.h>
#include <visualization_msgs/Marker.h>
#include <cmath>
#include <iomanip>

// These are the strings to display for different priorities. Based on roboat_utils/IThrustersManager.h TH_CMD_PRIORITY
static const std::string TH_CMD_PRIORITY_STRING[5] = { "Idle", "PID", "MPC", "Joy" };

class RoboatVisualization
{
private:
  ros::NodeHandle nh_;
  std::string id_; //roboat ID being visualized
  std::string base_link_frame_; //base_link frame where path transform is published

  ros::Publisher pub_path;
  ros::Publisher pub_marker;
  ros::Publisher pub_priority_marker;
  ros::Publisher pub_force_marker;
  ros::Subscriber sub_priority;
  ros::Subscriber sub_latching;
  ros::Subscriber sub_force;
  ros::Subscriber current_odometry;

  ros::Timer path_update_timer;
  double distance_till_update;
  double update_rate;
  double path_length;
  geometry_msgs::PoseStamped pose_stamped_;
  double dx, dy, dTheta;
  double f1, f2, f3, f4;
  roboat_msgs::ThrustState current_thrust_state;

public:
  RoboatVisualization();

  void initialize(ros::NodeHandle &nh, std::string id="");

  void thrustStateCallback(const roboat_msgs::ThrustState& msg);
  void odometryCallback(const nav_msgs::Odometry& msg);
  void forceCallback(const roboat_core::Force& msg);
  void pathHandler(const ros::TimerEvent& event);
  void visualizeRobot(geometry_msgs::PoseStamped pose_stamped);
};

#endif  // ROBOAT_VISUALIZATION_H
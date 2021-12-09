// Copyright 2020 AMS, all rights reserved
#ifndef SWARM_H
#define SWARM_H

// base dependencies
#include <math.h>
#include <iostream>
#include <algorithm>
#include <vector>

// ROS general packages
#include "ros/ros.h"
#include <ros/package.h>
#include <geometry_msgs/Pose.h>
#include <nav_msgs/Odometry.h>
#include <tf/transform_datatypes.h>

class SwarmState
{
private:
  ros::NodeHandle nh_;
  ros::Subscriber state_sub_;
  std::vector<double>* state_;
  int state_roll_over_count_;

  void stateCallback(const nav_msgs::Odometry msg);

public:
  SwarmState();
  void initialize(ros::NodeHandle &nh, std::string id, std::vector<double>* state);
};


class Swarm 
{
private:
  ros::NodeHandle nh_;
  int idx_;       //index within the swarm
  int n_boats_;   //total number of boats
  std::vector<std::string> boat_ids_;
  std::vector<SwarmState> swarm_state_;   //callbacks processing all messages

public:
  Swarm(ros::NodeHandle &nh);
  int getIdx();
  int getBoatN();
  std::vector<std::string> getBoatNames();
  
  /* state is the main variable that swarm provides access to the running node, since it 
  process the callbacks from each miniboat and fills the state */
  std::vector<std::vector<double>> state_;
};

#endif  // SWARM_H

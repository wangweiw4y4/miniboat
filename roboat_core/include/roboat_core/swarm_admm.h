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
#include <roboat_core/State.h>
#include <roboat_core/FloatArray.h>


class SwarmState
{
private:
  ros::NodeHandle nh_;
  ros::Subscriber state_sub_;
  std::vector<double>* state_;
  double* last_beat_; 

  void stateCallback(const roboat_core::State msg);

public:
  SwarmState();
  void initialize(ros::NodeHandle &nh, std::string id, std::vector<double>* state, double* last_beat);
};

class SwarmTrajectory
{
private:
  ros::NodeHandle nh_;
  ros::Subscriber trajectory_sub_;
  std::vector<double>* trajectory_;
  double* last_beat_; 

  void trajectoryCallback(const roboat_core::FloatArray msg);

public:
  SwarmTrajectory();
  void initialize(ros::NodeHandle &nh, std::string id, std::vector<double>* trajectory, double* last_beat);
};

class SwarmCopy
{
private:
  ros::NodeHandle nh_;
  ros::Subscriber copy_sub_;
  std::vector<double>* copy_;
  double* last_beat_; 

  void copyCallback(const roboat_core::FloatArray msg);

public:
  SwarmCopy();
  void initialize(ros::NodeHandle &nh, std::string id, std::vector<double>* copy, double* last_beat);
};

class SwarmMultiplier
{
private:
  ros::NodeHandle nh_;
  ros::Subscriber multiplier_sub_;
  std::vector<double>* multiplier_;
  double* last_beat_; 

  void multiplierCallback(const roboat_core::FloatArray msg);

public:
  SwarmMultiplier();
  void initialize(ros::NodeHandle &nh, std::string id, std::vector<double>* multiplier, double* last_beat);
};

class Swarm 
{
private:
  ros::NodeHandle nh_;
  int idx_;       //index within the swarm
  int n_boats_;   //total number of boats
  int N_; //ocp horizon
  int ocpX_states_; //ocpX states
  std::vector<std::string> boat_ids_;
  std::vector<SwarmState> swarm_state_;   //callbacks processing all messages
  std::vector<SwarmTrajectory> swarm_trajectory_;   //callbacks processing all messages
  std::vector<SwarmCopy> swarm_copy_;   //callbacks processing all messages
  std::vector<SwarmMultiplier> swarm_multiplier_;   //callbacks processing all messages

public:
  Swarm() {};
  Swarm(ros::NodeHandle &nh);
  void initialize(ros::NodeHandle &nh);
  int getIdx();
  int getBoatN();
  std::vector<std::string> getBoatNames();
  
  /* state is the main variable that swarm provides access to the running node, since it 
  process the callbacks from each miniboat and fills the state */
  std::vector<std::vector<double>> state_;
  std::vector<std::vector<double>> trajectory_;
  std::vector<std::vector<double>> copy_;
  std::vector<std::vector<double>> multiplier_;
  std::vector<double> last_beat_;
};

#endif  // SWARM_H
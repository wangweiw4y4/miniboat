#ifndef MPC_HPP
#define MPC_HPP

#include "ros/ros.h"
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <geometry_msgs/TwistWithCovarianceStamped.h>
#include <nav_msgs/Path.h>
#include <std_msgs/Float32MultiArray.h>
#include <std_msgs/Float64MultiArray.h>
#include <acado_common.h>

class MPC
{
private:
  double lastGoal[ACADO_NX], trajectory[ACADO_NX * ACADO_N];
  double u_autoware;
  double v_autoware;
  double step;
  int num_steps;
  bool path_received = false;
  bool pose_received = false;
  //double yaw_tailor_reference;
  double last_state[6];
  std::vector<double> state;
  ros::Publisher force_pub;
  ros::Subscriber path_sub;
  ros::Subscriber pose_sub;
  ros::Subscriber twist_sub;

  // functions related to processing subscriber msgs
  void pathCallback(const nav_msgs::Path::ConstPtr& msg);
  void poseCallback(const geometry_msgs::PoseWithCovarianceStamped msg);
  void stateCallback(const std_msgs::Float64MultiArray array);
  void twistCallback(const geometry_msgs::TwistWithCovarianceStamped msg);
  void trajectoryFill(double* trajectory, int pathSize);

  std::vector<double> acadoForce(std::vector<double> state, double* trajectory);

public:
  MPC(ros::NodeHandle n);
};
#endif

#ifndef SIM_HPP
#define SIM_HPP

// ROS
#include "ros/ros.h"
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <geometry_msgs/TwistStamped.h>

// roboat
#include <roboat_core/Force.h>

// C++
#include <boost/numeric/odeint.hpp>

using namespace boost::numeric::odeint;

typedef std::vector<double> state_type;

class Sim
{
private:
  std::vector<double> state;
  double simStep = 0.0001;

  /** default system dynamics parameters **/
  double d11 = 6;    // drag coff in the x direction
  double d22 = 8;    // drag coff in y direction
  double d33 = 0.6;  // drag torque coff
  double m11 = 12;   // mass plus added mass in the x direction
  double m22 = 16;   // mass plus added mass in the y direction
  double m33 = 1.5;  // moment of inertia plus added mass around the z axis
  double aa = 0.9;
  double bb = 0.45;
  double step = 0.1;

  ros::Publisher twist_pub;
  ros::Publisher pose_pub;
  ros::Publisher pub_VelocityRviz;
  ros::Subscriber force_sub;
  ros::Subscriber initialpose_sub;

  runge_kutta4<state_type> stepper;
  state_type integrate(state_type& x, double time);
  void forceCallback(const roboat_core::Force::ConstPtr& msg);
  void initialPoseCallback(const geometry_msgs::PoseWithCovarianceStamped::ConstPtr& msg);

public:
  Sim(ros::NodeHandle n);
  void operator()(const state_type& x, state_type& dxdt, const double /* t */);
};

#endif

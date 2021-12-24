// ROS
#include "ros/ros.h"
#include <ros/package.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <geometry_msgs/TwistWithCovarianceStamped.h>
#include <tf/transform_datatypes.h>
#include <tf/transform_broadcaster.h>
#include <std_msgs/Float32.h>

// C++
#include <boost/numeric/odeint.hpp>
#include <boost/ref.hpp>
#include "math.h"
#include <random>

// roboat
#include <roboat_core/Sim.hpp>
#include <roboat_core/Force.h>

using namespace boost::numeric::odeint;

std::default_random_engine generator;
std::normal_distribution<double> distx(0.0,0.0);
std::normal_distribution<double> disty(0.0,0.0);
std::normal_distribution<double> distt(0.0,0.0);
      

void Sim::operator()(const state_type& x, state_type& dxdt, const double /* t */)
{
  dxdt[0] = cos(x[2]) * x[3] - sin(x[2]) * x[4];
  dxdt[1] = sin(x[2]) * x[3] + cos(x[2]) * x[4];
  dxdt[2] = x[5];
  dxdt[3] = -d11 / m11 * x[3] + x[6] / m11 + x[7] / m11;
  dxdt[4] = -d22 / m22 * x[4] + x[8] / m22 + x[9] / m22;
  dxdt[5] = -d33 / m33 * x[5] + aa / (2 * m33) * x[6] - aa / (2 * m33) * x[7] + bb / (2 * m33) * x[8] - bb / (2 * m33) * x[9];
  dxdt[6] = 0;
  dxdt[7] = 0;
  dxdt[8] = 0;
  dxdt[9] = 0;
}

void Sim::forceCallback(const roboat_core::Force::ConstPtr& msg)
{
  // last indices of state represent force
  for (int i = 0; i < 4; i++)
    state[i + 6] = msg->data[i];
}

void Sim::initialPoseCallback(const geometry_msgs::PoseWithCovarianceStamped::ConstPtr& msg)
{
  // last indices of state represent force
  double roll, pitch, yaw;
  tf::Matrix3x3 m;

  m = tf::Matrix3x3(tf::Quaternion(msg->pose.pose.orientation.x, msg->pose.pose.orientation.y,
                                   msg->pose.pose.orientation.z, msg->pose.pose.orientation.w));
  m.getRPY(roll, pitch, yaw);

  state[0] = msg->pose.pose.position.x;
  state[1] = -msg->pose.pose.position.y;
  state[2] = -yaw;
}

state_type Sim::integrate(state_type& x, double time)
{
  integrate_const(stepper, boost::ref(*this), x, 0.0, time, simStep);
  return x;
}

Sim::Sim(ros::NodeHandle n)
{
  state = std::vector<double>(10);
  std::fill(state.begin(), state.end(), 0);

  n.param("init_x", state[0]);
  n.param("init_y", state[1]);
  n.param("system_dynamics/d11", d11);
  n.param("system_dynamics/d22", d22);
  n.param("system_dynamics/d33", d33);
  n.param("system_dynamics/m11", m11);
  n.param("system_dynamics/m22", m22);
  n.param("system_dynamics/m33", m33);
  n.param("system_dynamics/aa", aa);
  n.param("system_dynamics/bb", bb);
  n.param("system_dynamics/step", step);

  // publisher for x,y,theta + reference vector
  twist_pub = n.advertise<geometry_msgs::TwistWithCovarianceStamped>("filtered_twist", 10);
  pose_pub = n.advertise<geometry_msgs::PoseWithCovarianceStamped>("filtered_pose", 10);
  pub_VelocityRviz = n.advertise<std_msgs::Float32>("linear_velocity_viz", 10);

  // force from MPC, other controller, or manual rostopic
  force_sub = n.subscribe("command_force", 1, &Sim::forceCallback, this);

  // initial pose from rviz
  initialpose_sub = n.subscribe("initialpose", 1, &Sim::initialPoseCallback, this);

  ros::Rate loop_rate(1/step);

  ros::Time lastTime, currentTime = ros::Time::now();

  while (ros::ok())
  {
    ros::spinOnce();

    lastTime = currentTime;
    currentTime = ros::Time::now();

    // calculate new state as integration of state, over time using system model
    state = integrate(state, (currentTime - lastTime).toSec());

    // publish new twist (velocity)
    geometry_msgs::TwistWithCovarianceStamped twist_msg;
    
    //twist_msg.twist.linear.x = cos(state[2])*state[3]-sin(state[2])*state[4];
    //twist_msg.twist.linear.y = sin(state[2])*state[3]+cos(state[2])*state[4];
    twist_msg.twist.twist.linear.x = state[3];
    twist_msg.twist.twist.linear.y = state[4];
    twist_msg.twist.twist.linear.y*= -1;
    twist_msg.twist.twist.angular.z = -state[5]; 

    twist_msg.header.stamp = currentTime;
    twist_msg.header.frame_id = "base_link";
    twist_pub.publish(twist_msg);

    // publish new pose
    geometry_msgs::PoseWithCovarianceStamped pose_msg;
    pose_msg.pose.pose.position.x = state[0]+distx(generator);
    pose_msg.pose.pose.position.y = -state[1]+disty(generator);
    pose_msg.pose.pose.orientation = tf::createQuaternionMsgFromYaw(-state[2]+distt(generator));
    pose_msg.header.stamp = currentTime;
    pose_msg.header.frame_id = "odom";
    pose_pub.publish(pose_msg);
    
    static tf::TransformBroadcaster odom_broadcaster;
	geometry_msgs::TransformStamped odom_trans;
	odom_trans.header.stamp = ros::Time::now();
	odom_trans.header.frame_id = "odom";
	odom_trans.child_frame_id = "base_link";

	odom_trans.transform.translation.x = pose_msg.pose.pose.position.x;
	odom_trans.transform.translation.y = pose_msg.pose.pose.position.y;
	odom_trans.transform.translation.z = pose_msg.pose.pose.position.z;
	odom_trans.transform.rotation = pose_msg.pose.pose.orientation;

	// send the transform
	odom_broadcaster.sendTransform(odom_trans);
    
    std_msgs::Float32 vel_rviz;
	vel_rviz.data = twist_msg.twist.twist.linear.x;
	pub_VelocityRviz.publish(vel_rviz);
    

    ROS_DEBUG("[SIM_NODE] loop runtime: %fs, integration-time: %fs", ros::Time::now().toSec() - currentTime.toSec(),
              (currentTime - lastTime).toSec());

    loop_rate.sleep();
  }
}

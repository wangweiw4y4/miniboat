/*
Copyright (c) 2020, Marvelmind Robotics
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/

#include "ros/ros.h"
#include "std_msgs/String.h"

#include <ros/package.h>
#include "std_msgs/String.h"
#include "std_msgs/Int16.h"
#include <std_msgs/Float32MultiArray.h>
#include <std_msgs/Float64.h>
#include <std_msgs/Float64MultiArray.h>
#include <std_msgs/Int16MultiArray.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseArray.h>
#include <tf/transform_datatypes.h>
#include <tf/transform_broadcaster.h>
#include <tf/transform_listener.h>
#include <tf/transform_datatypes.h>
#include <nav_msgs/Odometry.h>
// other
#include <Eigen/Dense>
#include <math.h>
#include <iostream>
#include <algorithm>
#include <vector>

#include <roboat_core/Force.h>


#define ROS_NODE_NAME "pid_node"


#define DOUT_LPF_ORDER 4

double orientation_qx;
double orientation_qy;
double orientation_qz;
double orientation_qw;

double P = 0.5;
double I = 0;
double D = 0;
double ek;
double ek_old;
double e_total;

bool pid_flag = false;
int state_roll_over_count = 0;
/*External Loop PID Parameters*/
double external_K_yaw = 0.5;
double external_Td_yaw = 0.1;
double external_Ti_yaw = 50.0;
double external_e_yaw_differential = 0.0;
double external_e_yaw_integral = 0.0;
double external_e_yaw_last = 0.0;
double external_e_yaw = 0.0;
double external_out_yaw = 0.0; 
double external_dout_yaw = 0.0f;
double external_dout_pre_yaw = 0.0f;
double external_e_yaw_integral_a = 25.0;

/*Internal Loop PID Parameters*/
double internal_K_yaw = 0.6;
double internal_Td_yaw = 0.6;
double internal_Ti_yaw = 200.0;
double internal_e_yaw_differential = 0.0;
double internal_e_yaw_integral = 0.0;
double internal_e_yaw_last = 0.0;
double internal_e_yaw = 0.0; 
double internal_out_yaw = 0.0;
double internal_dout_yaw = 0.0f;
double internal_dout_pre_yaw = 0.0f;
double internal_e_yaw_integral_a = 16.5;


double step;
double control_effort;

double req_theta = M_PI/4;
double req_force = 0.25;

double desired_yaw = 0.0;
double desired_angular_velocity = 0;

std::vector<double> state(6);

void stateCallback(const nav_msgs::Odometry msg) {
    
    double yaw;

    state[0] = msg.pose.pose.position.x;
    state[1] = -msg.pose.pose.position.y; //
    yaw = -tf::getYaw(tf::Quaternion(msg.pose.pose.orientation.x, msg.pose.pose.orientation.y,
                                    msg.pose.pose.orientation.z, msg.pose.pose.orientation.w));

    // Unwrap the angle when roll over happens
    double yaw_diff = state[2] - yaw - state_roll_over_count * 2 * M_PI;
    if (yaw_diff > M_PI)
    {
      state_roll_over_count += 1;
    }
    else if (yaw_diff < -M_PI)
    {
      state_roll_over_count -= 1;
    }
    state[2] = yaw + state_roll_over_count * 2 * M_PI;
    
    state[3] = msg.twist.twist.linear.x;
    state[4] = -msg.twist.twist.linear.y;
    state[5] = -msg.twist.twist.angular.z;
    
    ROS_INFO("miniboat state is %f, %f, %f, %f, %f, %f", state[0], state[1],state[2],state[3],state[4],state[5]);
    
}



void controleffortcallback(const std_msgs::Float64 & control_msg)
{
   control_effort =control_msg.data;
}

Eigen::VectorXd pid_yaw_control(double yaw, double angular_velocity, double desired_yaw)
{ 
  
  /*Exteral PID, the output of external PID is the reference of internal PID*/
   external_e_yaw = desired_yaw - yaw;
  //if(external_e_yaw >= M_PI) external_e_yaw = external_e_yaw - 2*M_PI;
	//if(external_e_yaw <= -M_PI) external_e_yaw = external_e_yaw + 2*M_PI; 
  
  external_dout_yaw = external_K_yaw * external_Td_yaw * external_e_yaw_differential;
	external_dout_pre_yaw = (external_dout_yaw + (DOUT_LPF_ORDER - 1) * external_dout_pre_yaw) / DOUT_LPF_ORDER;
  external_out_yaw = (external_K_yaw * external_e_yaw + external_dout_pre_yaw);  
  
  external_e_yaw_differential = external_e_yaw - external_e_yaw_last;
	external_e_yaw_integral = external_e_yaw_integral + external_e_yaw;
	external_e_yaw_last = external_e_yaw;
  
  /*Internal PID*/
  internal_e_yaw = external_out_yaw - angular_velocity; 
  internal_dout_yaw = internal_K_yaw * internal_Td_yaw * internal_e_yaw_differential;
	internal_dout_pre_yaw = (internal_dout_yaw + (DOUT_LPF_ORDER - 1) * internal_dout_pre_yaw) / DOUT_LPF_ORDER;

  if ( (internal_e_yaw >= internal_e_yaw_integral_a) || (internal_e_yaw <= - internal_e_yaw_integral_a) ) 
	{
		internal_out_yaw =  (internal_K_yaw * internal_e_yaw + internal_dout_pre_yaw);  //内环PD控制
		internal_e_yaw_differential = internal_e_yaw - internal_e_yaw_last;
		internal_e_yaw_last = internal_e_yaw;
	}
	if ( (internal_e_yaw > - internal_e_yaw_integral_a) & (internal_e_yaw < internal_e_yaw_integral_a) ) 
	{
		internal_out_yaw = (internal_K_yaw * internal_e_yaw + internal_K_yaw / internal_Ti_yaw * internal_e_yaw_integral + internal_dout_pre_yaw);  //内环PID控制
		internal_e_yaw_differential = internal_e_yaw - internal_e_yaw_last;
		internal_e_yaw_integral = internal_e_yaw_integral + internal_e_yaw;
		internal_e_yaw_last = internal_e_yaw;
	}
  
  ROS_INFO("desired_yaw, external_pid_output, internal_pid_output: %f,%f,%f\n", desired_yaw, external_out_yaw, internal_out_yaw); 

  Eigen::VectorXd force(4);

  if ( internal_out_yaw > 0 || internal_out_yaw == 0)
 {   
 	  force(0) = std::min(0.5 * internal_out_yaw, 1.0);
  	force(1) = 0;
  	force(2) = 0;
  	force(3) = std::min(0.5 * internal_out_yaw, 1.0);
 } 
 else
 {
 	  force(0) = 0;
  	force(1) = std::min( - 0.5 * internal_out_yaw, 1.0);
  	force(2) = std::min( - 0.5 * internal_out_yaw, 1.0);
  	force(3) = 0;
 }
  return force;
}


Eigen::VectorXd pid_control2(double ek)
{
  double PID_output = P * ek + D * (ek - ek_old); 
  if (ek!=0)
  {
    ROS_INFO("PID input:  %f\n", ek);
    ROS_INFO("PID output: %f\n", PID_output);  
  } 
  Eigen::VectorXd force(4);
  
 // force = R_INV * torque;
 
 if ( PID_output > 0 || PID_output == 0)
 {   
 	force(0) = std::min(0.5 * PID_output, 1.0);
  	force(1) = 0;
  	force(2) = 0;
  	force(3) = std::min(0.5 * PID_output, 1.0);
 } 
 else
 {
 	force(0) = 0;
  	force(1) = std::min( - 0.5 * PID_output, 1.0);
  	force(2) = std::min( - 0.5 * PID_output, 1.0);
  	force(3) = 0;
 }

  ek_old = ek;
  return force;
}





Eigen::VectorXd pid_control()
{
  Eigen::VectorXd force(4);
  
 if ( control_effort > 0 || control_effort == 0)
 {   
 	  force(0) = std::min(control_effort, 1.0);
  	force(1) = 0;
  	force(2) = 0;
  	force(3) = std::min(control_effort, 1.0);
 } 
 else
 {
 	  force(0) = 0;
  	force(1) = std::min( - control_effort, 1.0);
  	force(2) = std::min( - control_effort, 1.0);
  	force(3) = 0;
 }
  return force;
}


Eigen::VectorXd velocity_control(double theta, double req_force)
{

  Eigen::VectorXd force(4);
  double theta1 = M_PI / 4;
  if ((theta > 0 && theta < M_PI / 2) || theta == 0)
 {
 	force(0) = req_force * cos(theta) / sin(theta1) + req_force * sin(theta) / cos(theta1);
  	force(1) = req_force * sin(theta) / cos(theta1);
  	force(2) = req_force * sin(theta) / cos(theta1);
  	force(3) = 0;
 }

  else if ((theta > M_PI / 2 && theta < M_PI) || theta == M_PI /2)
 {
 	force(0) = req_force * sin(theta) / cos(theta1);
  	force(1) = 0;
  	force(2) = req_force * sin(theta) / cos(theta1) + req_force * abs (cos(theta)) / sin(theta1);
  	force(3) = req_force * abs (cos(theta)) / sin(theta1);
 }

  else if ((theta > M_PI && theta < M_PI / 2 * 3) || theta == M_PI )
 {
 	force(0) = 0;
  	force(1) = req_force * abs (sin(theta)) / cos(theta1);
  	force(2) = req_force * abs (cos(theta)) / sin(theta1);
  	force(3) = req_force * abs (cos(theta)) / sin(theta1) + req_force * abs (sin(theta)) / cos(theta1);
 }

  else if ((theta > M_PI / 2 * 3 && theta < 2* M_PI) || theta == M_PI /2 * 3)
 {
 	force(0) = req_force * cos(theta) / sin(theta1);
  	force(1) = req_force * cos(theta) / sin(theta1) + req_force * abs (sin(theta)) / cos(theta1);
  	force(2) = 0;
  	force(3) = req_force * abs (sin(theta)) / cos(theta1);
 }
 else
 {
    force(0) = 0 ;
    force(1) = 0 ;
    force(2) = 0 ;
    force(3) = 0 ;
 }
 
//ROS_INFO("velocity force:  %f,%f,%f,%f\n", force(0), force(1), force(2),force(3)); 
  return force;
}

int main(int argc, char **argv)
{
	
  // initialize ROS node
  ros::init(argc, argv, ROS_NODE_NAME);
  
  // ROS node reference 
  ros::NodeHandle rosNode;
  rosNode.param("system_dynamics/step", step, 0.1);
  ros::Rate loop_rate(1/step);
  ros::Time begin = ros::Time::now();
  Eigen::VectorXd force = Eigen::VectorXd::Zero(4);


  if (rosNode.hasParam("PID/req_theta")) rosNode.getParam("PID/req_theta", req_theta);
  if (rosNode.hasParam("PID/req_force")) rosNode.getParam("PID/req_force", req_force);
  
  if (rosNode.hasParam("PID/P")) rosNode.getParam("PID/P", P);
  if (rosNode.hasParam("PID/I")) rosNode.getParam("PID/I", I);
  if (rosNode.hasParam("PID/D")) rosNode.getParam("PID/D", D);
  
  if (rosNode.hasParam("cascadepid/pid_flag")) rosNode.getParam("cascadepid/pid_flag", pid_flag);
  if (rosNode.hasParam("cascadepid/external_K_yaw")) rosNode.getParam("cascadepid/external_K_yaw", external_K_yaw);
  if (rosNode.hasParam("cascadepid/external_Td_yaw")) rosNode.getParam("cascadepid/external_Td_yaw", external_Td_yaw);
  if (rosNode.hasParam("cascadepid/external_Ti_yaw")) rosNode.getParam("cascadepid/external_Ti_yaw", external_Ti_yaw);
  if (rosNode.hasParam("cascadepid/external_e_yaw_differential")) rosNode.getParam("cascadepid/external_e_yaw_differential", external_e_yaw_differential);
  if (rosNode.hasParam("cascadepid/external_e_yaw_integral")) rosNode.getParam("cascadepid/external_e_yaw_integral", external_e_yaw_integral);
  if (rosNode.hasParam("cascadepid/external_e_yaw_last")) rosNode.getParam("cascadepid/external_e_yaw_last", external_e_yaw_last);
  if (rosNode.hasParam("cascadepid/external_e_yaw")) rosNode.getParam("cascadepid/external_e_yaw", external_e_yaw);
  if (rosNode.hasParam("cascadepid/external_out_yaw")) rosNode.getParam("cascadepid/external_out_yaw", external_out_yaw);
  if (rosNode.hasParam("cascadepid/external_dout_yaw")) rosNode.getParam("cascadepid/external_dout_yaw", external_dout_yaw);
  if (rosNode.hasParam("cascadepid/external_dout_pre_yaw")) rosNode.getParam("cascadepid/external_dout_pre_yaw", external_dout_pre_yaw);
  if (rosNode.hasParam("cascadepid/external_e_yaw_integral_a")) rosNode.getParam("cascadepid/external_e_yaw_integral_a", external_e_yaw_integral_a);

  if (rosNode.hasParam("cascadepid/internal_K_yaw")) rosNode.getParam("cascadepid/internal_K_yaw", internal_K_yaw);
  if (rosNode.hasParam("cascadepid/internal_Td_yaw")) rosNode.getParam("cascadepid/internal_Td_yaw", internal_Td_yaw);
  if (rosNode.hasParam("cascadepid/internal_Ti_yaw")) rosNode.getParam("cascadepid/internal_Ti_yaw", internal_Ti_yaw);
  if (rosNode.hasParam("cascadepid/internal_e_yaw_differential")) rosNode.getParam("cascadepid/internal_e_yaw_differential", internal_e_yaw_differential);
  if (rosNode.hasParam("cascadepid/internal_e_yaw_integral")) rosNode.getParam("cascadepid/internal_e_yaw_integral", internal_e_yaw_integral);
  if (rosNode.hasParam("cascadepid/internal_e_yaw_last")) rosNode.getParam("cascadepid/internal_e_yaw_last", internal_e_yaw_last);
  if (rosNode.hasParam("cascadepid/internal_e_yaw")) rosNode.getParam("cascadepid/internal_e_yaw", internal_e_yaw);
  if (rosNode.hasParam("cascadepid/internal_out_yaw")) rosNode.getParam("cascadepid/internal_out_yaw", internal_out_yaw);
  if (rosNode.hasParam("cascadepid/internal_dout_yaw")) rosNode.getParam("cascadepid/internal_dout_yaw", internal_dout_yaw);
  if (rosNode.hasParam("cascadepid/internal_dout_pre_yaw")) rosNode.getParam("cascadepid/internal_dout_pre_yaw", internal_dout_pre_yaw);
  if (rosNode.hasParam("cascadepid/internal_e_yaw_integral_a")) rosNode.getParam("cascadepid/internal_e_yaw_integral_a", internal_e_yaw_integral_a);

  if (rosNode.hasParam("cascadepid/desired_yaw")) rosNode.getParam("cascadepid/desired_yaw", desired_yaw);

  ros::Publisher setpoint_pub = rosNode.advertise<std_msgs::Float64>("setpoint", 1);

  ros::Publisher heading_pub = rosNode.advertise<std_msgs::Float64>("heading", 1);
    // publisher for force topic
  ros::Publisher force_pub = rosNode.advertise<roboat_core::Force>("pid_force", 1); 


  // Declare need to subscribe data from topic
 // ros::Subscriber substate = rosNode.subscribe("state", 1/step, statecallback);
  ros::Subscriber state_sub = rosNode.subscribe("odometry/filtered", 1/step, stateCallback);  // to get the angular velocity, odometry
 // ros::Subscriber subcontroleffort = rosNode.subscribe("control_effort", 1, controleffortcallback);




  while (ros::ok())
  {
      ros::spinOnce();
       
     ek = 0 - state[2];
        
        std_msgs::Float64 setpoint; //Heading angle reference
        setpoint.data = 0.0;
      setpoint_pub.publish(setpoint);  
 
      std_msgs::Float64 heading; //Heading angle reference
      heading.data = state[2];
      heading_pub.publish(heading);
 
    //  force =  velocity_control(req_theta, req_force)+ pid_control();
   //    force =  pid_control();
   //    force =  velocity_control(req_theta, req_force);
      if(pid_flag) {
        
        force = pid_yaw_control(state[2],state[5],desired_yaw);
     // ROS_INFO("velocity force:  %f,%f,%f,%f\n", force(0), force(1), force(2),force(3)); 
      
      roboat_core::Force forceMsg;
      Eigen::VectorXd::Map(&forceMsg.data[0], force.size()) = force;
      force_pub.publish(forceMsg);
      }

    loop_rate.sleep();
  }

  roboat_core::Force forceMsg;
  memset(&forceMsg.data[0], 0, sizeof(forceMsg.data));
  force_pub.publish(forceMsg);

  return 0;

}


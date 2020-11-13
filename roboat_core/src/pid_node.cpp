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

// other
#include <Eigen/Dense>
#include <math.h>
#include <iostream>
#include <algorithm>
#include <vector>

#include <roboat_core/Force.h>


#define ROS_NODE_NAME "pid_node"


#define PI 3.1415926


double orientation_qx;
double orientation_qy;
double orientation_qz;
double orientation_qw;


double step;
double control_effort;

double req_theta = PI/4;
double req_force = 0.25;
std::vector<double> state(6);

void statecallback(const std_msgs::Float64MultiArray & array_msg) {
    for(int i = 0; i < 6; i++) 
      state[i] = array_msg.data[i];
}

void controleffortcallback(const std_msgs::Float64 & control_msg)
{
   control_effort =control_msg.data;
}



Eigen::VectorXd pid_control()
{
  Eigen::VectorXd force(4);
  
 if ( control_effort > 0 || control_effort == 0)
 {   
 	  force(0) = std::min(0.5 * control_effort, 1.0);
  	force(1) = 0;
  	force(2) = 0;
  	force(3) = std::min(0.5 * control_effort, 1.0);
 } 
 else
 {
 	  force(0) = 0;
  	force(1) = std::min( - 0.5 * control_effort, 1.0);
  	force(2) = std::min( - 0.5 * control_effort, 1.0);
  	force(3) = 0;
 }
  return force;
}


Eigen::VectorXd velocity_control(double theta, double req_force)
{

  Eigen::VectorXd force(4);
  double theta1 = PI / 4;
  if ((theta > 0 && theta < PI / 2) || theta == 0)
 {
 	force(0) = req_force * cos(theta) / sin(theta1) + req_force * sin(theta) / cos(theta1);
  	force(1) = req_force * sin(theta) / cos(theta1);
  	force(2) = req_force * sin(theta) / cos(theta1);
  	force(3) = 0;
 }

  else if ((theta > PI / 2 && theta < PI) || theta == PI /2)
 {
 	force(0) = req_force * sin(theta) / cos(theta1);
  	force(1) = 0;
  	force(2) = req_force * sin(theta) / cos(theta1) + req_force * abs (cos(theta)) / sin(theta1);
  	force(3) = req_force * abs (cos(theta)) / sin(theta1);
 }

  else if ((theta > PI && theta < PI / 2 * 3) || theta == PI )
 {
 	force(0) = 0;
  	force(1) = req_force * abs (sin(theta)) / cos(theta1);
  	force(2) = req_force * abs (cos(theta)) / sin(theta1);
  	force(3) = req_force * abs (cos(theta)) / sin(theta1) + req_force * abs (sin(theta)) / cos(theta1);
 }

  else if ((theta > PI / 2 * 3 && theta < 2* PI) || theta == PI /2 * 3)
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
  if (rosNode.hasParam("PID/req_theta")) rosNode.getParam("PID/req_theta", req_theta);
  if (rosNode.hasParam("PID/req_force")) rosNode.getParam("PID/req_force", req_force);
  
  
 

  ros::Publisher setpoint_pub = rosNode.advertise<std_msgs::Float64>("setpoint", 1);

  ros::Publisher heading_pub = rosNode.advertise<std_msgs::Float64>("heading", 1);
    // publisher for force topic
  ros::Publisher force_pub = rosNode.advertise<roboat_core::Force>("pid_force", 1); 


  // Declare need to subscribe data from topic
  ros::Subscriber substate = rosNode.subscribe("state", 1, statecallback);
  ros::Subscriber subcontroleffort = rosNode.subscribe("control_effort", 1, controleffortcallback);


  rosNode.param("system_dynamics/step", step, 0.1);
  ros::Rate loop_rate(1/step);
  ros::Time begin = ros::Time::now();
  Eigen::VectorXd force = Eigen::VectorXd::Zero(4);

  while (ros::ok())
  {
      ros::spinOnce();
        
        std_msgs::Float64 setpoint; //Heading angle reference
        setpoint.data = 0.0;
      setpoint_pub.publish(setpoint);  
 
      std_msgs::Float64 heading; //Heading angle reference
      heading.data = state[2];
      heading_pub.publish(heading);
 
    //  force =  velocity_control(req_theta, req_force)+ pid_control();
       force =  pid_control();
     // ROS_INFO("velocity force:  %f,%f,%f,%f\n", force(0), force(1), force(2),force(3)); 
      
      roboat_core::Force forceMsg;
      Eigen::VectorXd::Map(&forceMsg.data[0], force.size()) = force;
      force_pub.publish(forceMsg);


    loop_rate.sleep();
  }

  roboat_core::Force forceMsg;
  memset(&forceMsg.data[0], 0, sizeof(forceMsg.data));
  force_pub.publish(forceMsg);

  return 0;

}


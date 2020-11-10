
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "marvelmind_nav/hedge_pos.h"
#include "marvelmind_nav/hedge_pos_a.h"
#include "marvelmind_nav/hedge_pos_ang.h"
#include "marvelmind_nav/beacon_pos_a.h"
#include "marvelmind_nav/hedge_imu_raw.h"
#include "marvelmind_nav/hedge_imu_fusion.h"
#include "marvelmind_nav/beacon_distance.h"
#include "marvelmind_nav/hedge_telemetry.h"
#include "marvelmind_nav/hedge_quality.h"
#include "marvelmind_nav/marvelmind_waypoint.h"
#include <visualization_msgs/Marker.h>

#include <ros/package.h>
#include "std_msgs/String.h"
#include "std_msgs/Int16.h"
#include <std_msgs/Float32MultiArray.h>
#include <std_msgs/Float64MultiArray.h>
#include <std_msgs/Int16MultiArray.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseArray.h>
#include <tf/transform_datatypes.h>
#include <tf/transform_broadcaster.h>
#include <tf/transform_listener.h>
#include <tf/transform_datatypes.h>

#include <sensor_msgs/Imu.h>

// other
#include <Eigen/Dense>
#include <math.h>
#include <iostream>
#include <algorithm>
#include <vector>

#include <roboat_core/Force.h>

#define ROS_NODE_NAME "sensor_node"
#define HEDGE_IMU_RAW_TOPIC_NAME "hedge_imu_raw"
#define HEDGE_IMU_FUSION_TOPIC_NAME "hedge_imu_fusion"

#define SCALE_HEDGE 3.0
#define ROBOT_LENGTH 0.20;


#define PI 3.1415926

double step;

double orientation_qx;
double orientation_qy;
double orientation_qz;
double orientation_qw;


int ANGLE_DRIFT = 0;
int USE_BNO055 = 1;


geometry_msgs::PoseStamped pose_bno055;
geometry_msgs::TwistStamped twist_bno055;

geometry_msgs::PoseStamped pose_beaconIMU;
geometry_msgs::TwistStamped twist_beaconIMU;

double roll_bno055;
double pitch_bno055;
double yaw_bno055; 

double roll_beaconIMU;
double pitch_beaconIMU;
double yaw_beaconIMU;


std::vector<double> state(6);


void bno055callback(const sensor_msgs::Imu::ConstPtr& imu_raw)
{
        tf::Matrix3x3 m;

        m = tf::Matrix3x3(
        tf::Quaternion(imu_raw->orientation.x, imu_raw->orientation.y, imu_raw->orientation.z, imu_raw->orientation.w));
        m.getRPY(roll_bno055, pitch_bno055, yaw_bno055);
        //ROS_ERROR("bno055: %f,%f,%f", roll_bno055, pitch_bno055, yaw_bno055);

        pose_bno055.pose.orientation = imu_raw->orientation;
        twist_bno055.twist.angular = imu_raw->angular_velocity;

}

void IMURawCallback(const marvelmind_nav::hedge_imu_raw& hedge_imu_raw_msg)
{
	// ROS_INFO("Raw IMU: Timestamp: %08d, aX=%05d aY=%05d aZ=%05d  gX=%05d gY=%05d gZ=%05d  cX=%05d cY=%05d cZ=%05d", 	
	// 			(int) hedge_imu_raw_msg.timestamp_ms,
	// 			(int) hedge_imu_raw_msg.acc_x, (int) hedge_imu_raw_msg.acc_y, (int) hedge_imu_raw_msg.acc_z,
	// 			(int) hedge_imu_raw_msg.gyro_x, (int) hedge_imu_raw_msg.gyro_y, (int) hedge_imu_raw_msg.gyro_z,
	// 			(int) hedge_imu_raw_msg.compass_x, (int) hedge_imu_raw_msg.compass_y, (int) hedge_imu_raw_msg.compass_z);


// 0.0175 dps/LSB
   twist_beaconIMU.twist.angular.x = hedge_imu_raw_msg.gyro_x * 0.0175 * PI / 180;
   twist_beaconIMU.twist.angular.y = -hedge_imu_raw_msg.gyro_y * 0.0175 * PI / 180;
   twist_beaconIMU.twist.angular.z = -hedge_imu_raw_msg.gyro_z * 0.0175 * PI / 180;


}

void IMUFusionCallback(const marvelmind_nav::hedge_imu_fusion& hedge_imu_fusion_msg)
{
	// ROS_INFO("IMU fusion: Timestamp: %08d, X=%.3f  Y= %.3f  Z=%.3f  q=%.3f,%.3f,%.3f,%.3f v=%.3f,%.3f,%.3f  a=%.3f,%.3f,%.3f", 	
	// 			(int) hedge_imu_fusion_msg.timestamp_ms,
	// 			(float) hedge_imu_fusion_msg.x_m, (float) hedge_imu_fusion_msg.y_m, (float) hedge_imu_fusion_msg.z_m,
	// 			(float) hedge_imu_fusion_msg.qw, (float) hedge_imu_fusion_msg.qx, (float) hedge_imu_fusion_msg.qy, (float) hedge_imu_fusion_msg.qz,
	// 			(float) hedge_imu_fusion_msg.vx, (float) hedge_imu_fusion_msg.vy, (float) hedge_imu_fusion_msg.vz,
	// 			(float) hedge_imu_fusion_msg.ax, (float) hedge_imu_fusion_msg.ay, (float) hedge_imu_fusion_msg.az);

  
    pose_beaconIMU.header.stamp  =ros::Time::now();

    pose_beaconIMU.pose.position.x = hedge_imu_fusion_msg.x_m;
    pose_beaconIMU.pose.position.y = hedge_imu_fusion_msg.y_m; 
    pose_beaconIMU.pose.position.z = hedge_imu_fusion_msg.z_m; 
  
	
   //pose.header.stamp.sec =  (int) (hedge_imu_fusion_msg.timestamp_ms/1000);
   //pose.header.stamp.nsec =  (int) ((hedge_imu_fusion_msg.timestamp_ms%1000)*(10^6));					
   
    pose_beaconIMU.pose.orientation.x= hedge_imu_fusion_msg.qx;
    pose_beaconIMU.pose.orientation.y= hedge_imu_fusion_msg.qy;
    pose_beaconIMU.pose.orientation.z= hedge_imu_fusion_msg.qz;
    pose_beaconIMU.pose.orientation.w= hedge_imu_fusion_msg.qw;
    
     
    orientation_qx= hedge_imu_fusion_msg.qx;
    orientation_qy= hedge_imu_fusion_msg.qy;
    orientation_qz= hedge_imu_fusion_msg.qz;
    orientation_qw= hedge_imu_fusion_msg.qw;
    tf::Matrix3x3 m;
    m = tf::Matrix3x3(tf::Quaternion(pose_beaconIMU.pose.orientation.x, pose_beaconIMU.pose.orientation.y, pose_beaconIMU.pose.orientation.z, pose_beaconIMU.pose.orientation.w));
    m.getRPY(roll_beaconIMU, pitch_beaconIMU, yaw_beaconIMU);
    yaw_beaconIMU = -yaw_beaconIMU;

    ROS_INFO("beaconIMU roll, pitch, yaw:  %f.%f,%f\n", roll_beaconIMU, pitch_beaconIMU, yaw_beaconIMU);


   twist_beaconIMU.header.stamp = ros::Time::now();
   //twist.header.stamp.sec =  (int) (hedge_imu_fusion_msg.timestamp_ms/1000);
   //twist.header.stamp.nsec =  (int) ((hedge_imu_fusion_msg.timestamp_ms%1000)*(10^6));	

   twist_beaconIMU.twist.linear.x = hedge_imu_fusion_msg.vx;
   twist_beaconIMU.twist.linear.y = -hedge_imu_fusion_msg.vy;
   twist_beaconIMU.twist.linear.z = -hedge_imu_fusion_msg.vz;
   


    pose_bno055.header.stamp  =ros::Time::now();

    pose_bno055.pose.position.x = hedge_imu_fusion_msg.x_m;
    pose_bno055.pose.position.y = hedge_imu_fusion_msg.y_m; 
    pose_bno055.pose.position.z = hedge_imu_fusion_msg.z_m; 
    pose_bno055.pose.orientation = pose_bno055.pose.orientation;

    twist_bno055.header.stamp = ros::Time::now();
    twist_bno055.twist.linear.x = hedge_imu_fusion_msg.vx;
    twist_bno055.twist.linear.y = -hedge_imu_fusion_msg.vy;
    twist_bno055.twist.linear.z = -hedge_imu_fusion_msg.vz;
    twist_bno055.twist.angular =  twist_bno055.twist.angular;

}


int main(int argc, char **argv)
{
	
  // initialize ROS node
  ros::init(argc, argv, ROS_NODE_NAME);
  // ROS node reference 
  ros::NodeHandle n;

  if(n.hasParam("IMU/ANGLE_DRIFT")) n.getParam("IMU/ANGLE_DRIFT", ANGLE_DRIFT);
  //ROS_ERROR("%d", ANGLE_DRIFT);
  if(n.hasParam("IMU/USE_BNO055")) n.getParam("IMU/USE_BNO055", USE_BNO055);


  // publisher for force topic 
  ros::Publisher pose_bno055_pub = n.advertise<geometry_msgs::PoseStamped>("pose_bno055", 10); 
  ros::Publisher twist_bno055_pub = n.advertise<geometry_msgs::TwistStamped>("twist_bno055", 10); 

  ros::Publisher pose_beaconIMU_pub = n.advertise<geometry_msgs::PoseStamped>("pose_beaconIMU", 10); 
  ros::Publisher twist_beaconIMU_pub = n.advertise<geometry_msgs::TwistStamped>("twist_beaconIMU", 10); 

  ros::Publisher state_pub = n.advertise<std_msgs::Float64MultiArray>("state", 10);


  ros::Subscriber subIMURaw = n.subscribe(HEDGE_IMU_RAW_TOPIC_NAME, 1000, IMURawCallback);
  ros::Subscriber subIMUFusion = n.subscribe(HEDGE_IMU_FUSION_TOPIC_NAME, 1000, IMUFusionCallback);
  ros::Subscriber bno055_sub= n.subscribe("/imu/data", 1, bno055callback);


  n.param("system_dynamics/step", step, 0.1);
  ros::Rate loop_rate(1/step);
  ros::Time begin = ros::Time::now();

  while (ros::ok())
  {
       ros::spinOnce();
       
        if (USE_BNO055 ==1)
    {
      state[0] = pose_bno055.pose.position.x;
      state[1] = pose_bno055.pose.position.y;
      state[2] = yaw_bno055;
      state[3] = twist_bno055.twist.linear.x;
      state[4] = twist_bno055.twist.linear.y;
      state[5] = twist_bno055.twist.angular.z;
    }
    else
     {
      state[0] = pose_beaconIMU.pose.position.x;
      state[1] = pose_beaconIMU.pose.position.y;
      state[2] = yaw_beaconIMU;
      state[3] = twist_beaconIMU.twist.linear.x;
      state[4] = twist_beaconIMU.twist.linear.y;
      state[5] = twist_beaconIMU.twist.angular.z;
     }    

    geometry_msgs::PoseStamped pose_bno055_msg;
    pose_bno055_msg = pose_bno055; 
    pose_bno055_pub.publish(pose_bno055_msg);

    geometry_msgs::TwistStamped twist_bno055_msg;
    twist_bno055_msg = twist_bno055; 
    twist_bno055_pub.publish(twist_bno055_msg);


    geometry_msgs::PoseStamped pose_beaconIMU_msg;
    pose_beaconIMU_msg = pose_beaconIMU; 
    pose_beaconIMU_pub.publish(pose_beaconIMU_msg);

    geometry_msgs::TwistStamped twist_beaconIMU_msg;
    twist_beaconIMU_msg = twist_beaconIMU; 
    twist_beaconIMU_pub.publish(twist_beaconIMU_msg);


    //publish state
    std_msgs::Float64MultiArray stateMsg;
    stateMsg.data.resize(6);
    stateMsg.data = state;
    state_pub.publish(stateMsg);

    loop_rate.sleep();
  }
  return 0;

}


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
#include <nav_msgs/Odometry.h>
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
#define HEDGE_POSITION_WITH_ANGLE_TOPIC_NAME "hedge_pos_ang"
#define HEDGE_POSITION_TOPIC_NAME "hedge_pos"

#define SCALE_HEDGE 3.0
#define ROBOT_LENGTH 0.20;


#define PI 3.1415926

double step;

double orientation_qx;
double orientation_qy;
double orientation_qz;
double orientation_qw;

double ANGLE_DRIFT_BNO055 = 0;
int BNO055_ANGLE_Calibration_Count=0;
int BNO055_Calibration_Finish =0;

double ANGLE_DRIFT_BEACONIMU= 0;
int BEACONIMU_ANGLE_Calibration_Count=0;
int BEACONIMU_Calibration_Finish =0;

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

double twobeacon_x;
double twobeacon_y;
double twobeacon_heading;

std::vector<double> state(6);


void hedgePosAngCallback(const marvelmind_nav::hedge_pos_ang& hedge_pos_msg)
{				
  twobeacon_x = hedge_pos_msg.x_m;
  twobeacon_y = hedge_pos_msg.y_m;
  twobeacon_heading = hedge_pos_msg.angle +135;
  if (twobeacon_heading>180)
    twobeacon_heading = twobeacon_heading-360;
  twobeacon_heading = twobeacon_heading*PI/180.0;
  
  // ROS_INFO("Hedgehog data: Address= %d, timestamp= %d, X=%.3f  Y= %.3f  Z=%.3f  Angle: %.1f  flags=%d", 	
  //   (int) hedge_pos_msg.address, 
  //   (int) hedge_pos_msg.timestamp_ms, 
  //   (float) hedge_pos_msg.x_m, (float) hedge_pos_msg.y_m, (float) hedge_pos_msg.z_m,
  //   twobeacon_heading*180/PI,  
  //   (int) hedge_pos_msg.flags);		
				
  /*				
  if ((hedge_pos_msg.flags&(1<<0))==0)
  {				
    showRvizObject(hedge_pos_msg.address,hedge_pos_msg.x_m, hedge_pos_msg.y_m, hedge_pos_msg.z_m, objHedge);
  }  
  */
}

void hedgePosCallback(const marvelmind_nav::hedge_pos_a& hedge_pos_msg)
{
  ROS_INFO("Hedgehog data: Address= %d, timestamp= %d, X=%.3f  Y= %.3f  Z=%.3f  flags=%d", 	
    (int) hedge_pos_msg.address, 
    (int) hedge_pos_msg.timestamp_ms, 
    (float) hedge_pos_msg.x_m, (float) hedge_pos_msg.y_m, (float) hedge_pos_msg.z_m,  
    (int) hedge_pos_msg.flags);
    
  twobeacon_x = hedge_pos_msg.x_m;
  twobeacon_y = hedge_pos_msg.y_m;
}


void bno055callback(const sensor_msgs::Imu::ConstPtr& imu_raw)
{
  tf::Matrix3x3 m;

  m = tf::Matrix3x3(
  tf::Quaternion(imu_raw->orientation.x, imu_raw->orientation.y, imu_raw->orientation.z, imu_raw->orientation.w));
  m.getRPY(roll_bno055, pitch_bno055, yaw_bno055);
  
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


  pose_beaconIMU.pose.orientation.x= hedge_imu_fusion_msg.qx;
  pose_beaconIMU.pose.orientation.y= hedge_imu_fusion_msg.qy;
  pose_beaconIMU.pose.orientation.z= hedge_imu_fusion_msg.qz;
  pose_beaconIMU.pose.orientation.w= hedge_imu_fusion_msg.qw;
  
    
  orientation_qx= hedge_imu_fusion_msg.qx;
  orientation_qy= hedge_imu_fusion_msg.qy;
  orientation_qz= hedge_imu_fusion_msg.qz;
  orientation_qw= hedge_imu_fusion_msg.qw;
  tf::Matrix3x3 m;
  m = tf::Matrix3x3(tf::Quaternion(orientation_qx, orientation_qy, orientation_qz, orientation_qw));
  m.getRPY(roll_beaconIMU, pitch_beaconIMU, yaw_beaconIMU);
  yaw_beaconIMU = yaw_beaconIMU*180.0/PI +135;
  if (yaw_beaconIMU>180)
  yaw_beaconIMU = yaw_beaconIMU-360;
  yaw_beaconIMU = yaw_beaconIMU*PI/180.0;
  
  /*
  yaw_beaconIMU = -yaw_beaconIMU;
  
  if(BEACONIMU_Calibration_Finish)
  {
    yaw_beaconIMU= yaw_beaconIMU-ANGLE_DRIFT_BEACONIMU;
    //ROS_INFO("BEACONIMU yaw drift: %f", ANGLE_DRIFT_BEACONIMU);
    }
  else
  {      ANGLE_DRIFT_BEACONIMU = ANGLE_DRIFT_BEACONIMU+yaw_beaconIMU;
        BEACONIMU_ANGLE_Calibration_Count = BEACONIMU_ANGLE_Calibration_Count+1;
          //ROS_INFO("BEACONIMU calibration count: %d", BEACONIMU_ANGLE_Calibration_Count);	 
    if(BEACONIMU_ANGLE_Calibration_Count == 200)
    {
      BEACONIMU_Calibration_Finish=1;
        ANGLE_DRIFT_BEACONIMU = ANGLE_DRIFT_BEACONIMU/200;      
    }
  }
  if (yaw_beaconIMU>PI)
  yaw_beaconIMU = yaw_beaconIMU-2*PI;
  if (yaw_beaconIMU<-PI)
  yaw_beaconIMU = yaw_beaconIMU + 2*PI;
  */
  
  //ROS_INFO("beaconIMU roll, pitch, yaw:  %f.%f,%f\n", roll_beaconIMU, pitch_beaconIMU, yaw_beaconIMU);


  twist_beaconIMU.header.stamp = ros::Time::now();
  //twist.header.stamp.sec =  (int) (hedge_imu_fusion_msg.timestamp_ms/1000);
  //twist.header.stamp.nsec =  (int) ((hedge_imu_fusion_msg.timestamp_ms%1000)*(10^6));	

  twist_beaconIMU.twist.linear.x = hedge_imu_fusion_msg.vx;
  twist_beaconIMU.twist.linear.y = hedge_imu_fusion_msg.vy;
  twist_beaconIMU.twist.linear.z = hedge_imu_fusion_msg.vz;
   
  pose_bno055.header.stamp  =ros::Time::now();

  pose_bno055.pose.position.x = hedge_imu_fusion_msg.x_m;
  pose_bno055.pose.position.y = hedge_imu_fusion_msg.y_m; 
  pose_bno055.pose.position.z = hedge_imu_fusion_msg.z_m; 
  pose_bno055.pose.orientation = pose_bno055.pose.orientation;

  twist_bno055.header.stamp = ros::Time::now();
  twist_bno055.twist.linear.x = hedge_imu_fusion_msg.vx;
  twist_bno055.twist.linear.y = hedge_imu_fusion_msg.vy;
  twist_bno055.twist.linear.z = hedge_imu_fusion_msg.vz;
  twist_bno055.twist.angular =  twist_bno055.twist.angular;
}


int main(int argc, char **argv)
{
	
  // initialize ROS node
  ros::init(argc, argv, ROS_NODE_NAME);
  // ROS node reference 
  ros::NodeHandle n;

  if(n.hasParam("IMU/ANGLE_DRIFT")) n.getParam("IMU/ANGLE_DRIFT", ANGLE_DRIFT_BNO055);
  //ROS_ERROR("%d", ANGLE_DRIFT);
  if(n.hasParam("IMU/USE_BNO055")) n.getParam("IMU/USE_BNO055", USE_BNO055);


  // publisher for force topic 
  ros::Publisher pose_bno055_pub = n.advertise<geometry_msgs::PoseStamped>("pose_bno055", 10); 
  ros::Publisher twist_bno055_pub = n.advertise<geometry_msgs::TwistStamped>("twist_bno055", 10); 

  ros::Publisher pose_beaconIMU_pub = n.advertise<geometry_msgs::PoseStamped>("pose_beaconIMU", 10); 
  ros::Publisher twist_beaconIMU_pub = n.advertise<geometry_msgs::TwistStamped>("twist_beaconIMU", 10); 

  ros::Publisher state_pub = n.advertise<nav_msgs::Odometry>("odometry/filtered", 10);


  ros::Subscriber subIMURaw = n.subscribe(HEDGE_IMU_RAW_TOPIC_NAME, 1000, IMURawCallback);
  ros::Subscriber subIMUFusion = n.subscribe(HEDGE_IMU_FUSION_TOPIC_NAME, 1000, IMUFusionCallback);
  ros::Subscriber subHedgeWithAngle = n.subscribe(HEDGE_POSITION_WITH_ANGLE_TOPIC_NAME, 1000, hedgePosAngCallback);
  ros::Subscriber bno055_sub= n.subscribe("imu/data", 1, bno055callback);

  std::string id;
  n.param<std::string>("roboat_id",id,"");
  std::string base_link_frame;
  if (id.empty()) {
    base_link_frame = "base_link";
  }
  else {
    base_link_frame = "base_link_"+id;
  }

  double draught = 0.025; //fixed draught, used only for visualization so defined here explicitly

  n.param("system_dynamics/step", step, 0.1);
  ros::Rate loop_rate(1/step);
  
  while (ros::ok())
  {
    ros::spinOnce();
    /*       
    if (USE_BNO055 ==1)
    {
      state[0] = pose_bno055.pose.position.x;
      state[1] = pose_bno055.pose.position.y;
      state[2] = -yaw_bno055;
      state[3] = twist_bno055.twist.linear.x;
      state[4] = twist_bno055.twist.linear.y;
      state[5] = -twist_bno055.twist.angular.z;
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
    */

    state[0] = twobeacon_x;
    state[1] = twobeacon_y;
    state[2] = twobeacon_heading;
    state[3] = 0;
    state[4] = 0;
    state[5] = -twist_bno055.twist.angular.z;
      
      
    // ROS_INFO("x,y,heading,u,v,r:  %f,%f,%f,%f,%f,%f\n",  state[0],  state[1],  state[2]*180/PI,  state[3],  state[4], state[5]);
    pose_bno055_pub.publish(pose_bno055);
    twist_bno055_pub.publish(twist_bno055);

    pose_beaconIMU_pub.publish(pose_beaconIMU);
    twist_beaconIMU_pub.publish(twist_beaconIMU);

    //publish state
    // std_msgs::Float64MultiArray state_msg;
    // state_msg.data.resize(6);
    // state_msg.data = state;

    nav_msgs::Odometry state_msg;
    state_msg.pose.pose.position.x = state[0];
    state_msg.pose.pose.position.y = -state[1];
    state_msg.pose.pose.position.z = -0;

    state_msg.pose.pose.orientation = tf::createQuaternionMsgFromRollPitchYaw(0, 0, -state[2]);

    // No velocities for now
    state_msg.twist.twist.linear.x = 0.0;
    state_msg.twist.twist.linear.y = -0.0;
    state_msg.twist.twist.angular.z = -0.0;

    //publishes the state odometry data    
    state_pub.publish(state_msg);

    // map -> odom static transformation (both are the same)
    static tf::TransformBroadcaster tf_broadcast;
    // static tf::Transform map_to_odom = tf::Transform(tf::createQuaternionFromRPY(0, 0, 0), tf::Vector3(0, 0, 0));
    // tf_broadcast.sendTransform(tf::StampedTransform(map_to_odom, ros::Time::now(), "map", "odom"));
    
    // odom -> base_link transformation (for visualization, matching what the EKF filter will do)
    tf::Transform odom_to_base = tf::Transform(tf::createQuaternionFromRPY(0, 0, -state[2]), tf::Vector3(state[0], -state[1], -draught));
    tf_broadcast.sendTransform(tf::StampedTransform(odom_to_base, ros::Time::now(), "odom", base_link_frame));

    loop_rate.sleep();
  }
  return 0;
}

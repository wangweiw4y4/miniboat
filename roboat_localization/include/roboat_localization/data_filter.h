// Copyright (c) 2021. MIT & AMS institute - Roboat.
#ifndef DATA_FILTER_H
#define DATA_FILTER_H

#include <ros/ros.h>

#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>

// transforms
#include <tf/transform_datatypes.h>
#include <tf/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>

// ROS messages
#include <nav_msgs/Odometry.h>
#include <sensor_msgs/Imu.h>


// BEACONS
#include "marvelmind_nav/hedge_pos_ang.h"
#include "marvelmind_nav/hedge_imu_raw.h"
// #include "marvelmind_nav/hedge_imu_fusion.h"


// class definition
class DataFilter {
 private:

  ros::NodeHandle nh_;

  ros::Subscriber sub_IMU_; //bno055
  ros::Subscriber sub_paired_hedge_IMU_; 
  ros::Subscriber sub_paired_hedge_odom_; 
  
  ros::Publisher pub_IMU_, pub_paired_hedge_IMU_;
  ros::Publisher pub_paired_hedge_odom_;
  
  // tf frames
  std::string odom_, nmpc_, base_link_, imu_link_, dual_beacon_link_;

  // configuration parameters
  double hedge_pos_cov_, hedge_acc_cov_, hedge_gyr_cov_, hedge_compass_cov_;
  double dualhedge_heading_offset_;

  void imuHandler(const sensor_msgs::Imu::ConstPtr &imu_raw);
  void pairedHedgeIMUHandler(const marvelmind_nav::hedge_imu_raw &hedge_imu_raw_msg);
  void pairedHedgeOdomHandler(const marvelmind_nav::hedge_pos_ang &hedge_pos_msg); 

  ros::Timer map_update_timer_;
  void map_odom(const ros::TimerEvent& event);

 public:
  DataFilter();  

};

class DataTS {
private:
  int ntime_ts_,itime_first_,itime_last_;
  double time_span_ts_;
  double min_covariance_;
  std::vector<double> time_ts_, dat_ts_;
  
public:
  DataTS(double time_span);
  void addPoint(double time, double val);
  double getMean();
  double getCovariance();
};

#endif  // DATA_FILTER_H
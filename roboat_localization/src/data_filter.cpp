// Copyright (c) 2021. MIT & AMS institute - Roboat.
#include "roboat_localization/data_filter.h"

using namespace std;

DataFilter::DataFilter() 
{
  // SUBSCRIBERS
  /*imu*/
  sub_IMU_ = nh_.subscribe("imu/data", 1, &DataFilter::imuHandler, this);
  sub_paired_hedge_IMU_ = nh_.subscribe("hedge_imu_raw", 1000, &DataFilter::pairedHedgeIMUHandler, this);
  /* paired beacon signals are already processed by the marvelmind library, so need to do the 
  calculations here, and we directly listen to the combined signal */  
  sub_paired_hedge_odom_ = nh_.subscribe("hedge_pos_ang", 1000, &DataFilter::pairedHedgeOdomHandler, this);
  
  // PUBLISHERS 
  pub_IMU_ = nh_.advertise<sensor_msgs::Imu>("data_filter/imu0", 1);
  pub_paired_hedge_IMU_ = nh_.advertise<sensor_msgs::Imu>("data_filter/imu1", 1);
  pub_paired_hedge_odom_ = nh_.advertise<nav_msgs::Odometry>("data_filter/paired_beacons", 1);
  
  // TF FRAMES
  std::string id;
  nh_.param<std::string>("roboat_id",id,"");
  base_link_ = "base_link";
  imu_link_ = "imu1_link"; 
  dual_beacon_link_ = "dual_beacon_link";
  if (!id.empty()) {
    base_link_ += "_"+id;
    imu_link_ += "_"+id; 
    dual_beacon_link_ += "_"+id;
  }
  
  nh_.getParam("roboat_filter/hedge_ref_pos_cov", hedge_pos_cov_);
  nh_.getParam("roboat_filter/hedge_ref_acc_cov", hedge_acc_cov_);
  nh_.getParam("roboat_filter/hedge_ref_gyr_cov", hedge_gyr_cov_);
  nh_.getParam("roboat_filter/hedge_ref_compass_cov", hedge_compass_cov_);
  nh_.getParam("roboat_filter/dualhedge_heading_offset", dualhedge_heading_offset_);
  // nh_.getParam("roboat_filter/minGPSCovTh", min_GPS_cov_threshold_);
  // nh_.getParam("roboat_filter/maxGPSCovTh", max_GPS_cov_threshold_);
  // // nh_.getParam("roboat_filter/headingOffset", headingOffset);
  // nh_.getParam("roboat_filter/gpsBaseline", gpsBaseline);
}

// void DataFilter::spin_node() {
//   double step;
//   nh_.param("system_dynamics/step", step, 0.1);
//   ros::Rate loop_rate(1/step);
  
//   // synchronously 
//   while (ros::ok())
//   {
//     ros::spinOnce();
    
//     // map -> odom static transformation (both are the same)
//     static tf::TransformBroadcaster tf_broadcast;
//     static tf::Transform map_to_odom = tf::Transform(tf::createQuaternionFromRPY(0, 0, 0), tf::Vector3(0, 0, 0));
//     tf_broadcast.sendTransform(tf::StampedTransform(map_to_odom, ros::Time::now(), "map", "odom"));
    
//   }
//   return;
// }

void DataFilter::imuHandler(const sensor_msgs::Imu::ConstPtr &msg) {
  // convert IMU
  sensor_msgs::Imu imu_out = *msg;
  imu_out.header.frame_id = imu_link_;
  imu_out.header.stamp = ros::Time::now();

  // checks that orientation is properly defined
  tf::Quaternion q(msg->orientation.x, msg->orientation.y, msg->orientation.z, msg->orientation.w);
  if (sqrt(q.x()*q.x() + q.y()*q.y() + q.z()*q.z() + q.w()*q.w()) < 0.1) {
    ROS_ERROR("Invalid quaternion, please use a 9-axis IMU!");
  }

  pub_IMU_.publish(imu_out);
  return;
}

void DataFilter::pairedHedgeIMUHandler(const marvelmind_nav::hedge_imu_raw &msg)
{
  /* Parses the IMU from the marvelmind_nav into a generic imu messge to be filtered 
  
  Covariances are not published by the marvelmind_nav, so it should be computed since we are
  fusing data with IMU1. However, for simplicity, I use reference values provided as a configuration 
  parameter for now
  */
	sensor_msgs::Imu imu_out;

  imu_out.header.stamp = ros::Time::now(); //ros::Time(msg.timestamp_ms);
  imu_out.header.frame_id = dual_beacon_link_;

  // Linear acceleration
  imu_out.linear_acceleration.x = msg.acc_x;
  imu_out.linear_acceleration.y = msg.acc_y;
  imu_out.linear_acceleration.z = msg.acc_z;
  // 
  imu_out.linear_acceleration_covariance.assign(0.0);
  imu_out.linear_acceleration_covariance[0] = hedge_acc_cov_;
  imu_out.linear_acceleration_covariance[4] = hedge_acc_cov_;
  imu_out.linear_acceleration_covariance[8] = hedge_acc_cov_;
  
  // Angular velocity
  double gyro2vel = 0.0175 * M_PI / 180; // 0.0175 dps/LSB
  imu_out.angular_velocity.x = msg.gyro_x * gyro2vel;
  imu_out.angular_velocity.y = -msg.gyro_y * gyro2vel;
  imu_out.angular_velocity.z = -msg.gyro_z * gyro2vel;
  // 
  imu_out.angular_velocity_covariance.assign(0.0);
  imu_out.angular_velocity_covariance[0] = hedge_gyr_cov_;
  imu_out.angular_velocity_covariance[4] = hedge_gyr_cov_;
  imu_out.angular_velocity_covariance[8] = hedge_gyr_cov_;
  
  // Compass orientation
  // (assumed is provided as roll/pitch/yaw, check!)
  imu_out.orientation = tf::createQuaternionMsgFromRollPitchYaw(msg.compass_x, msg.compass_y, msg.compass_z);
  // 
  imu_out.orientation_covariance.assign(0.0);
  imu_out.orientation_covariance[0] = hedge_compass_cov_;
  imu_out.orientation_covariance[4] = hedge_compass_cov_;
  imu_out.orientation_covariance[8] = hedge_compass_cov_;
  
  // calls the generic imuHandler to 
  pub_paired_hedge_IMU_.publish(imu_out);
  return;
}

void DataFilter::pairedHedgeOdomHandler(const marvelmind_nav::hedge_pos_ang &msg) {
  
  /* marvelmind_nav do not provide covariance info. 
  Since this is the only sensor providing position data, we use the reference value provided
  as a configuration parameters, rather than computing it here */

  // Publish odometry for EKF (global)
  nav_msgs::Odometry beaconOdometry;
  beaconOdometry.header.stamp = ros::Time::now(); //ros::Time(msg.timestamp_ms);
  beaconOdometry.header.frame_id = "odom";
  beaconOdometry.child_frame_id = dual_beacon_link_;

  beaconOdometry.pose.pose.position.x = msg.x_m;
  beaconOdometry.pose.pose.position.y = msg.y_m;
  beaconOdometry.pose.pose.position.z = -0.025; //used only for visualization, so defined here explicitly

  double heading = msg.angle + dualhedge_heading_offset_;
  if (heading>180) {
    heading -= 360;
  }
  else if (heading<-180) {
    heading += 360;
  }
  heading *= M_PI/180.0;  
  
  beaconOdometry.pose.pose.orientation = tf::createQuaternionMsgFromRollPitchYaw(0, 0, heading);
  std::fill(beaconOdometry.pose.covariance.begin(), beaconOdometry.pose.covariance.end(), 0.0);
  beaconOdometry.pose.covariance[0] = hedge_pos_cov_;
  beaconOdometry.pose.covariance[7] = hedge_pos_cov_;
  beaconOdometry.pose.covariance[14] = hedge_pos_cov_;

  pub_paired_hedge_odom_.publish(beaconOdometry);
  return;
}


// ////////////////////////////////////////////////////////////////////////////
// COVARIANCE estimate
// ////////////////////////////////////////////////////////////////////////////
DataTS::DataTS(double time_span) : time_span_ts_(time_span) {
  //time-series for filtering
  ntime_ts_ = 0;
  itime_first_ = -1;
  itime_last_ = -1;
  //time-series needs to be initalized with >0 size, to avoid conflicts with indexing and avoid excesive resizing 
  dat_ts_.assign(100,0.0);
  time_ts_.assign(100,0.0);
  //min covariance, so it ensures is non-zero when running
  min_covariance_=1e-6;
}

void DataTS::addPoint(double time, double val)
{
  //adds a new entry to the boat position time-series
  bool start_ts = (itime_last_ == -1);
  itime_last_++;
  
  //takes care of re-start or end-of-array situation
  if (start_ts)
  {
    ntime_ts_=0;
    itime_first_=0; //we are starting the sequence, so initializes also the first counter
  }
  else if (itime_last_==(int)dat_ts_.size())
  {
    itime_last_=0;
  }

  //stores new points 
  if (itime_first_!=itime_last_ || start_ts) //replaces old unused content
  {
    time_ts_[itime_last_] = time;
    dat_ts_[itime_last_] = val;
  }
  else //we have run out of space, expand the storage space. Quite innefficient, but should enter here only a few times
  {
    time_ts_.insert(time_ts_.begin()+itime_last_, time);
    dat_ts_.insert(dat_ts_.begin()+itime_last_, val);
    itime_first_++;
  }
  //increases number of stored points counter
  ntime_ts_++;

  //removes the old points
  while(time - time_ts_[itime_first_] > time_span_ts_)
  {
    itime_first_++;
    ntime_ts_--;
    if (itime_first_==(int)dat_ts_.size())
    {
      itime_first_=0; //re-starts and eventually will get to itime_last_
    }
  }
  return;
}

double DataTS::getMean() {
  double mean=0.0;
  int ktime_prev, ktime = itime_first_;
  for (int i=0; i<ntime_ts_-1; i++)
  {
    ktime_prev = ktime;
    ktime++;
    if (ktime == (int)dat_ts_.size()) {
      ktime = 0;
    }
    mean += 0.5*(dat_ts_[ktime_prev]+dat_ts_[ktime]) *(time_ts_[ktime] - time_ts_[ktime_prev]);
  }
  mean = mean/(time_ts_[itime_last_]-time_ts_[itime_first_]);
  return mean;
}

double DataTS::getCovariance() {
  double mean = getMean();
  double covariance=0.0;
  int ktime_prev, ktime = itime_first_;
  for (int i=0; i<ntime_ts_-1; i++)
  {
    ktime_prev = ktime;
    ktime++;
    if (ktime == (int)dat_ts_.size()) {
      ktime = 0;
    }
    covariance += pow(0.5*(dat_ts_[ktime_prev]+dat_ts_[ktime])-mean, 2.0) *(time_ts_[ktime] - time_ts_[ktime_prev]);
  }
  covariance = std::max(covariance/(time_ts_[itime_last_]-time_ts_[itime_first_]), min_covariance_);
  return covariance;
}

// Copyright (c) 2021. MIT & AMS institute - Roboat.
#include <ros/ros.h>

#include <nav_msgs/Odometry.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/NavSatFix.h>
#include <tf/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>

#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

#include <pcl/point_cloud.h>

#include <utility>
#include <vector>
#include <cmath>
#include <deque>
#include <ctime>
#include <array>
#include <functional>

#include <roboat_localization/WGS84toCartesian.hpp>

using namespace std;

class DataFilter {
 private:

  ros::NodeHandle nh;

  ros::Subscriber subIMU;
  ros::Subscriber subGPS;
  ros::Subscriber subGPS2;
  ros::Subscriber subOdometry;

  message_filters::Subscriber<sensor_msgs::NavSatFix> subFilterGPS1;
  message_filters::Subscriber<sensor_msgs::NavSatFix> subFilterGPS2;
  typedef message_filters::sync_policies::ApproximateTime<sensor_msgs::NavSatFix, sensor_msgs::NavSatFix> MySyncPolicy;
  typedef message_filters::Synchronizer<MySyncPolicy> MySync;
  message_filters::Synchronizer<MySyncPolicy> *sync;

  ros::Publisher pubIMU;
  ros::Publisher pubGPS;
  ros::Publisher pubGPS2;
  ros::Publisher pubHeading;
  ros::Publisher pubGPSVelocity;

  // IMU Extrinsics
  Eigen::Matrix3d extRot;
  Eigen::Matrix3d extRPY;
  Eigen::Quaterniond extQRPY;

  std::array<double, 2> WGS84Reference{{52.37344, 4.91549}};

  // GPS Covariance Threshold
  double minGPSCovTh;
  double maxGPSCovTh;
  double headingOffset;
  double gpsBaseline;
  double heading;

  geometry_msgs::TransformStamped transform_gps1;
  geometry_msgs::TransformStamped transform_gps2;

  geometry_msgs::Pose prior_odom_pose;

 public:
  DataFilter() {
    subIMU = nh.subscribe<sensor_msgs::Imu>("sensor/imu", 1, &DataFilter::imuHandler, this);
    subGPS = nh.subscribe<sensor_msgs::NavSatFix>("sensor/gps", 1, std::bind(&DataFilter::gpsHandler, this,
                                                                             std::placeholders::_1,
                                                                             std::ref(pubGPS)));

    subGPS2 = nh.subscribe<sensor_msgs::NavSatFix>("sensor/gps2", 1, std::bind(&DataFilter::gpsHandler, this,
                                                                               std::placeholders::_1,
                                                                               std::ref(pubGPS2)));

    subFilterGPS1.subscribe(nh, "sensor/heading_gps1", 1);
    subFilterGPS2.subscribe(nh, "sensor/heading_gps2", 1);
    sync = new MySync(MySyncPolicy(10), subFilterGPS1, subFilterGPS2);
    sync->registerCallback(boost::bind(&DataFilter::dualGPSHandler, this, _1, _2));

    pubIMU = nh.advertise<sensor_msgs::Imu>("filtered/imu", 1);
    pubGPS = nh.advertise<nav_msgs::Odometry>("filtered/gps", 1);
    pubGPS2 = nh.advertise<nav_msgs::Odometry>("filtered/gps2", 1);
    pubHeading = nh.advertise<nav_msgs::Odometry>("filtered/heading", 1);

    vector<double> extRotV, extRPYV;
    nh.param<vector<double >>("roboat_filter/extrinsicRot", extRotV, vector<double>());
    nh.param<vector<double >>("roboat_filter/extrinsicRPY", extRPYV, vector<double>());
    extRot = Eigen::Map<const Eigen::Matrix<double, -1, -1, Eigen::RowMajor>>(extRotV.data(), 3, 3);
    extRPY = Eigen::Map<const Eigen::Matrix<double, -1, -1, Eigen::RowMajor>>(extRPYV.data(), 3, 3);
    extQRPY = Eigen::Quaterniond(extRPY);

    nh.getParam("roboat_filter/minGPSCovTh", minGPSCovTh);
    nh.getParam("roboat_filter/maxGPSCovTh", maxGPSCovTh);
    nh.getParam("roboat_filter/headingOffset", headingOffset);
    nh.getParam("roboat_filter/gpsBaseline", gpsBaseline);

    try {
      tf2_ros::Buffer tf_buffer;
      tf2_ros::TransformListener tf_odom_sat(tf_buffer);
      transform_gps1 = tf_buffer.lookupTransform("navsat_link", "base_link",
                                                 ros::Time(0), ros::Duration(4));

      transform_gps2 = tf_buffer.lookupTransform("navsat2_link", "base_link",
                                                 ros::Time(0), ros::Duration(4));

      ROS_INFO("Static gps transforms; x/y offset: %f/%f %f/%f",
               transform_gps1.transform.translation.x,
               transform_gps1.transform.translation.y,
               transform_gps2.transform.translation.x,
               transform_gps2.transform.translation.y);
    }
    catch (tf::TransformException &ex) {
      ROS_WARN("%s", ex.what());
    }
  }

  void imuHandler(const sensor_msgs::Imu::ConstPtr &msg) {
    // map -> odom static transformation
    static tf::TransformBroadcaster tf_map_odom;
    static tf::Transform map_to_odom = tf::Transform(tf::createQuaternionFromRPY(0, 0, 0), tf::Vector3(0, 0, 0));
    tf_map_odom.sendTransform(tf::StampedTransform(map_to_odom, msg->header.stamp, "map", "odom"));

    // convert IMU
    sensor_msgs::Imu imu_out = *msg;
    imu_out.header.frame_id = "imu_link";

    // rotate acceleration
    Eigen::Vector3d acc(msg->linear_acceleration.x, msg->linear_acceleration.y, msg->linear_acceleration.z);
    acc = extRot*acc;
    imu_out.linear_acceleration.x = acc.x();
    imu_out.linear_acceleration.y = acc.y();
    imu_out.linear_acceleration.z = acc.z();

    // rotate gyroscope
    Eigen::Vector3d gyr(msg->angular_velocity.x, msg->angular_velocity.y, msg->angular_velocity.z);
    gyr = extRot*gyr;
    imu_out.angular_velocity.x = gyr.x();
    imu_out.angular_velocity.y = gyr.y();
    imu_out.angular_velocity.z = gyr.z();

    // rotate roll pitch yaw
    Eigen::Quaterniond q_from(msg->orientation.w, msg->orientation.x, msg->orientation.y, msg->orientation.z);
    Eigen::Quaterniond q_final = q_from*extQRPY;
    imu_out.orientation.x = q_final.x();
    imu_out.orientation.y = q_final.y();
    imu_out.orientation.z = q_final.z();
    imu_out.orientation.w = q_final.w();

    imu_out.angular_velocity_covariance = msg->angular_velocity_covariance;
    imu_out.linear_acceleration_covariance = msg->linear_acceleration_covariance;
    imu_out.orientation_covariance = msg->orientation_covariance;
    
    // Obtain orientation
    // tf::Quaternion orientation;
    // tf::Matrix3x3 tfMatrix;
    // double fixed_roll, fixed_pitch, fixed_yaw;
    // tf::quaternionMsgToTF(msg->orientation, orientation);
    // tfMatrix = tf::Matrix3x3(orientation);
    // tfMatrix.getRPY(fixed_roll, fixed_pitch, fixed_yaw);
    // imu_out.orientation = tf::createQuaternionMsgFromRollPitchYaw(0, 0, -fixed_yaw - 65 / 180.0 * M_PI);

    if (sqrt(
        q_final.x()*q_final.x() + q_final.y()*q_final.y() + q_final.z()*q_final.z() + q_final.w()*q_final.w()) <
        0.1) {
      ROS_ERROR("Invalid quaternion, please use a 9-axis IMU!");
      ros::shutdown();
    }

    pubIMU.publish(imu_out);
  }

  void gpsHandler(const sensor_msgs::NavSatFix::ConstPtr &msg, ros::Publisher &pub) {
    sensor_msgs::NavSatFix gps = *msg;

    static tf::TransformBroadcaster tf_map_odom;
    static tf::Transform map_to_odom = tf::Transform(tf::createQuaternionFromRPY(0, 0, 0), tf::Vector3(0, 0, 0));
    tf_map_odom.sendTransform(tf::StampedTransform(map_to_odom, msg->header.stamp, "map", "odom"));

    if (gps.position_covariance[0] < minGPSCovTh || gps.position_covariance[0] > maxGPSCovTh ||
        gps.position_covariance[4] < minGPSCovTh || gps.position_covariance[4] > maxGPSCovTh) {
      ROS_WARN("GPS covarience too high for gps. Discarding odometry pose. %f %f", gps.position_covariance[0], gps.position_covariance[4]);
      return;
    }

    // GPS to Euclidean
    std::array<double, 2> gps_coordinate{gps.latitude, gps.longitude};
    std::array<double, 2> gps_position{wgs84::toCartesian(WGS84Reference, gps_coordinate)};

    // Publish odometry for EKF (global)
    nav_msgs::Odometry gpsOdometry;
    gpsOdometry.header.stamp = gps.header.stamp;
    gpsOdometry.header.frame_id = "odom";
    gpsOdometry.child_frame_id = "base_link";

    // Find the transformation of the GPS coordinates based on the static transform of the navsat_link. This effectively
    // moves the GPS x/y position to the base_link. NOTE: Intuitively this should be done via the tf by setting the correct
    // frame id's, however this doesn't seem to be the case.
    double x_tf, y_tf;
    if (pub==pubGPS) {
      x_tf = -transform_gps1.transform.translation.y*std::sin(heading) + transform_gps1.transform.translation.x*std::cos(heading);
      y_tf = transform_gps1.transform.translation.y*std::cos(heading) + transform_gps1.transform.translation.x*std::sin(heading);
    } else {
      x_tf = -transform_gps2.transform.translation.y*std::sin(heading) + transform_gps2.transform.translation.x*std::cos(heading);
      y_tf = transform_gps2.transform.translation.y*std::cos(heading) + transform_gps2.transform.translation.x*std::sin(heading);
    }

    gpsOdometry.pose.pose.position.x = gps_position[0] + x_tf;
    gpsOdometry.pose.pose.position.y = gps_position[1] + y_tf;
    gpsOdometry.pose.pose.position.z = 0;

    gpsOdometry.pose.pose.orientation = tf::createQuaternionMsgFromRollPitchYaw(0, 0, heading);
    std::fill(gpsOdometry.pose.covariance.begin(), gpsOdometry.pose.covariance.end(), 0);
    gpsOdometry.pose.covariance[0] = gps.position_covariance[0];
    gpsOdometry.pose.covariance[7] = gps.position_covariance[4];
    gpsOdometry.pose.covariance[14] = 0.25;

    pub.publish(gpsOdometry);
  }

  void dualGPSHandler(const sensor_msgs::NavSatFix::ConstPtr &msg1, const sensor_msgs::NavSatFix::ConstPtr &msg2) {
    sensor_msgs::NavSatFix gps1 = *msg1;
    sensor_msgs::NavSatFix gps2 = *msg2;

    if (gps1.position_covariance[0] < minGPSCovTh || gps1.position_covariance[0] > maxGPSCovTh ||
        gps1.position_covariance[4] < minGPSCovTh || gps1.position_covariance[4] > maxGPSCovTh) {
      ROS_WARN("GPS covarience too high for gps1. Cannot calculate heading.");
      return;
    }

    if (gps2.position_covariance[0] < minGPSCovTh || gps2.position_covariance[0] > maxGPSCovTh ||
        gps2.position_covariance[4] < minGPSCovTh || gps2.position_covariance[4] > maxGPSCovTh) {
      ROS_WARN("GPS covarience too high for gps2. Cannot calculate heading.");
      return;
    }

    std::array<double, 2> gps_coordinate1{gps1.latitude, gps1.longitude};
    std::array<double, 2> gps_position1{wgs84::toCartesian(WGS84Reference, gps_coordinate1)};
    std::array<double, 2> gps_coordinate2{gps2.latitude, gps2.longitude};
    std::array<double, 2> gps_position2{wgs84::toCartesian(WGS84Reference, gps_coordinate2)};

    double diff_x = gps_position1[0] - gps_position2[0];
    double diff_y = gps_position1[1] - gps_position2[1];

    if (sqrt(diff_x*diff_x + diff_y*diff_y) < 1.0) {
      ROS_WARN_ONCE("GPS baseline too close, try moving them further apart.");
      return;
    }

    // calculate heading
    heading = atan2(diff_y, diff_x) + headingOffset/180.0*M_PI;

    // calculate covariance
    double cov1 = sqrt(max(gps1.position_covariance[0], gps1.position_covariance[4]));
    double cov2 = sqrt(max(gps2.position_covariance[0], gps2.position_covariance[4]));
    double cov3 = abs(atan2(cov1 + cov2, gpsBaseline));

    nav_msgs::Odometry odomHeading;
    odomHeading.header.stamp = msg1->header.stamp;
    odomHeading.header.frame_id = "odom";
    odomHeading.child_frame_id = "base_link";
    odomHeading.pose.pose.orientation = tf::createQuaternionMsgFromRollPitchYaw(0, 0, heading);
    std::fill(odomHeading.pose.covariance.begin(), odomHeading.pose.covariance.end(), 0);
    odomHeading.pose.covariance[35] = cov3*cov3;
    pubHeading.publish(odomHeading);
  }

};

int main(int argc, char **argv) {
  ros::init(argc, argv, "roboat_filter");

  DataFilter DF;

  ROS_INFO("\033[1;32m----> roboat_filter: Data Filter.\033[0m");

  ros::MultiThreadedSpinner spinner(5);
  spinner.spin();

  return 0;
}

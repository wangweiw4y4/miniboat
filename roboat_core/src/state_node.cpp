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
#include <geometry_msgs/Pose2D.h>
#include <geometry_msgs/PoseArray.h>
#include <sensor_msgs/Imu.h>
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

#include <roboat_core/State.h>

#define ROS_NODE_NAME "state_node"

std::vector<double> state(6);


void stateCallback(const nav_msgs::Odometry::ConstPtr& msg)
{
    double yaw;

    state[0] = msg.pose.pose.position.x;
    state[1] = -msg.pose.pose.position.y; //
    yaw = -tf::getYaw(tf::Quaternion(msg.pose.pose.orientation.x, msg.pose.pose.orientation.y,
                                     msg.pose.pose.orientation.z, msg.pose.pose.orientation.w));

    state[2] = yaw;
    state[3] = msg.twist.twist.linear.x;
    state[4] = -msg.twist.twist.linear.y;
    // state[5] = -msg.twist.twist.angular.z;

    // ROS_WARN("miniboat state is %f, %f, %f, %f, %f, %f", state[0], state[1], state[2], state[3], state[4], state[5]);
}

void imuCallback(const sensor_msgs::Imu::ConstPtr& msg)
{
    state[5] = -msg.angular_velocity.z;
}

int main(int argc, char *argv[])
{
    ros::init(argc, argv, ROS_NODE_NAME);
    int rate = 50;
    ros::Rate loop_rate(rate);

    ros::NodeHandle n;
    ros::Publisher state_pub;
    ros::Subscriber state_sub;
    ros::Subscriber imu_sub;
    
    state_pub = n.advertise<roboat_core::State>("boat_state", 1);
    state_sub = n.subscribe("odometry/filtered", 1, stateCallback);
    imu_sub = n.subscribe("imu/data", 1, imuCallback);

    while (ros::ok())
    {  
        ros::spinOnce();
        
        roboat_core::State state_msg;
        std::copy(state.begin(), state.end(), &state_msg.data[0]);
        state_pub.publish(state_msg);

        
        loop_rate.sleep();
    }
    return 0;
}
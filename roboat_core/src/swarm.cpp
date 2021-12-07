/* ************************************************************************* */
/* LATCHING MODEL CONTROL */
/* ************************************************************************* */
/*
Class that controls to motion of the boat to approach to the target
*/

#include "roboat_core/swarm.h"


Swarm::Swarm(ros::NodeHandle &nh) : nh_(nh)
{
  /*After being initialized, runs within the class the control node, since no
  other control scheme is considered for now*/

  //For now, we will identify all miniboats with a unique positive integer
  nh_.getParam("swarm/n_boats",n_boats_);
  // ROS_INFO("n boats %d",n_boats_);

  //resize the arrays within showing the number 
  state_.resize(n_boats_);
  swarm_state_.resize(n_boats_);

  for (int i = 0; i<n_boats_; i++) {
    state_[i].resize(6);
    swarm_state_[i].initialize(nh_, i, &state_[i]);
  }
}

SwarmState::SwarmState()
{
  state_roll_over_count_ = 0;
}

void SwarmState::initialize(ros::NodeHandle &nh, int idx, std::vector<double>* state) 
{
  nh_ = nh;
  state_ = state;
  //subscribe to the topic
  // std::stringstream ss;
  // ss << std::setw(2) << std::setfill('0') << idx;
  // std::string topic = "/miniboat" +  ss.str() + "/state";
  std::string topic = "/miniboat" +  std::to_string(idx) + "/state";
  // ROS_INFO("rostopic subscribed: %s", topic.c_str());
  state_sub_ = nh_.subscribe(topic, 1, &SwarmState::stateCallback, this);
  return;
}

void SwarmState::stateCallback(const nav_msgs::Odometry msg) {
    
    /*If state is advertised as an array, data can just be copied */
    // for(int i=0; i<6; i++) state[i] = array.data[i];
    // pose_received = true;
    
    /*If state is advertised as odometry, easier to visualize in rviz*/
    double yaw;

    // convert from map to roboat map reference (negative y and yaw)
    (*state_)[0] = msg.pose.pose.position.x;
    (*state_)[1] = -msg.pose.pose.position.y; //
    yaw = -tf::getYaw(tf::Quaternion(msg.pose.pose.orientation.x, msg.pose.pose.orientation.y,
                                    msg.pose.pose.orientation.z, msg.pose.pose.orientation.w));

    // Unwrap the angle when roll over happens
    double yaw_diff = (*state_)[2] - yaw - state_roll_over_count_ * 2 * M_PI;
    if (yaw_diff > M_PI)
    {
      state_roll_over_count_++;
    }
    else if (yaw_diff < -M_PI)
    {
      state_roll_over_count_--;
    }
    (*state_)[2] = yaw + state_roll_over_count_ * 2 * M_PI;

    // convert from map to roboat map reference (negative y and yaw)
    (*state_)[3] = msg.twist.twist.linear.x;
    (*state_)[4] = -msg.twist.twist.linear.y;
    (*state_)[5] = -msg.twist.twist.angular.z;
    
    return;
}
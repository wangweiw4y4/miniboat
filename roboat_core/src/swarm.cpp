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

  //Assumes all miniboats are properly identified int he YAML config file, with no repeated ids
  std::vector<std::string> boat_ids;
  nh_.getParam("swarm/boat_ids",boat_ids);
  
  //identifies the index of the miniboat itself within the swarm (-1 if not found)
  idx_= -1;
  std::string id = ros::this_node::getNamespace();
  id.erase(id.begin()); //first character is '/' for the global namespace, so takes it away
  // ROS_INFO("miniboat id: %s",id.c_str());
  for (int i = 0; i<boat_ids.size(); i++) {
    if (boat_ids[i]==id) {
      idx_=i;
      break;
    } 
    // ROS_INFO("boat %d: %s",i,boat_ids[i].c_str());
  }
  
  //resize the arrays within showing the number 
  n_boats_ = boat_ids.size();
  state_.resize(n_boats_);
  swarm_state_.resize(n_boats_);

  for (int i = 0; i<n_boats_; i++) {
    state_[i].resize(6);
    swarm_state_[i].initialize(nh_, boat_ids[i], &state_[i]);
  }
}

int Swarm::getIdx() {
  return idx_;
}

SwarmState::SwarmState()
{
  state_roll_over_count_ = 0;
}

void SwarmState::initialize(ros::NodeHandle &nh, std::string id, std::vector<double>* state) 
{
  nh_ = nh;
  state_ = state;
  //subscribe to the topic
  std::string topic = "/" +  id + "/state"; 
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
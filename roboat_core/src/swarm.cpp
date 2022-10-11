/* ************************************************************************* */
/* LATCHING MODEL CONTROL */
/* ************************************************************************* */
/*
Class that controls to motion of the boat to approach to the target
*/

#include "roboat_core/swarm.h"

Swarm::Swarm(ros::NodeHandle &nh) : nh_(nh)
{
  initialize(nh_);
}

void Swarm::initialize(ros::NodeHandle &nh)
{
  nh_ = nh;
  /*After being initialized, runs within the class the control node, since no
  other control scheme is considered for now*/

  // Assumes all miniboats are properly identified int he YAML config file, with no repeated ids
  nh_.getParam("swarm/boat_ids", boat_ids_);
  n_boats_ = boat_ids_.size();

  // identifies the index of the miniboat itself within the swarm (-1 if not found)
  idx_ = -1;
  std::string id = ros::this_node::getNamespace();
  id.erase(id.begin()); // first character is '/' for the global namespace, so takes it away
  // ROS_INFO("miniboat id: %s",id.c_str());
  for (int i = 0; i < n_boats_; i++)
  {
    if (boat_ids_[i] == id)
    {
      idx_ = i;
      break;
    }
    // ROS_INFO("boat %d: %s",i,boat_ids_[i].c_str());
  }

  // resize the arrays within showing the number
  state_.resize(n_boats_);
  last_beat_.resize(n_boats_);
  swarm_state_.resize(n_boats_);

  for (int i = 0; i < n_boats_; i++)
  {
    state_[i].resize(6);
    swarm_state_[i].initialize(nh_, boat_ids_[i], &state_[i], &last_beat_[i]);
  }
}

int Swarm::getIdx()
{
  return idx_;
}

int Swarm::getBoatN()
{
  return n_boats_;
}

std::vector<std::string> Swarm::getBoatNames()
{
  return boat_ids_;
}

SwarmState::SwarmState()
{
}

void SwarmState::initialize(ros::NodeHandle &nh, std::string id, std::vector<double> *state, double *last_beat)
{
  nh_ = nh;
  state_ = state;
  last_beat_ = last_beat;
  // subscribe to the topic (keep "/" apart so topic name is relative as in the rest of code)
  //std::string topic = "/" + id + "/" + "odometry/filtered";
  std::string topic = "/" + id + "/" + "boat_state";
  // ROS_INFO("rostopic subscribed: %s", topic.c_str());
  state_sub_ = nh_.subscribe(topic, 1, &SwarmState::stateCallback, this);
  return;
}

void SwarmState::stateCallback(const roboat_core::State msg)
{

  /*If state is advertised as an array, data can just be copied */
  // for(int i=0; i<6; i++) state[i] = array.data[i];
  // pose_received = true;


  (*state_)[0] = msg.data[0];
  (*state_)[1] = msg.data[1]; //
  (*state_)[2] = msg.data[2];

  // convert from map to roboat map reference (negative y and yaw)
  (*state_)[3] = msg.data[3];
  (*state_)[4] = msg.data[4];
  (*state_)[5] = msg.data[5];


  //*last_beat_ = msg.header.stamp.toSec();

  /*If state is advertised as odometry, easier to visualize in rviz*/
  //double yaw;

  // convert from map to roboat map reference (negative y and yaw)
  //(*state_)[0] = msg.pose.pose.position.x;
  //(*state_)[1] = -msg.pose.pose.position.y; //

  //yaw = -tf::getYaw(tf::Quaternion(msg.pose.pose.orientation.x, msg.pose.pose.orientation.y,
  //                                 msg.pose.pose.orientation.z, msg.pose.pose.orientation.w));
  //(*state_)[2] = yaw;

  // convert from map to roboat map reference (negative y and yaw)
  //(*state_)[3] = msg.twist.twist.linear.x;
  //(*state_)[4] = -msg.twist.twist.linear.y;
  //(*state_)[5] = -msg.twist.twist.angular.z;

  return;
}
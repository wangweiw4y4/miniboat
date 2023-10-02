#include "roboat_core/swarm_admm.h"

Swarm::Swarm(ros::NodeHandle &nh) : nh_(nh)
{
  initialize(nh_);
}

void Swarm::initialize(ros::NodeHandle &nh)
{
  nh_ = nh;

  // Assumes all miniboats are properly identified int he YAML config file, with no repeated ids
  nh_.getParam("swarm/boat_ids", boat_ids_);
  nh_.param("swarm/N", N_, 20);
  nh_.param("swarm/ocpX_states", ocpX_states_, 2);
  n_boats_ = boat_ids_.size();

  N_ = N_+1;

  // identifies the index of the miniboat itself within the swarm (-1 if not found)
  idx_ = -1;
  std::string id = ros::this_node::getNamespace();
  id.erase(id.begin()); // first character is '/' for the global namespace, so takes it away
  // ROS_ERROR("miniboat id: %s",id.c_str());
  for (int i = 0; i < n_boats_; i++)
  {
    if (boat_ids_[i] == id)
    {
      idx_ = i;
      break;
    }
    // ROS_ERROR("boat %d: %s",i,boat_ids_[i].c_str());
  }

  // resize the arrays within showing the number
  state_.resize(n_boats_);
  trajectory_.resize(n_boats_);
  copy_.resize(n_boats_);
  multiplier_.resize(n_boats_);
  last_beat_.resize(n_boats_);
  swarm_state_.resize(n_boats_);
  swarm_trajectory_.resize(n_boats_);
  swarm_copy_.resize(n_boats_);
  swarm_multiplier_.resize(n_boats_);

  for (int i = 0; i < n_boats_; i++)
  {
    state_[i].resize(6);
    swarm_state_[i].initialize(nh_, boat_ids_[i], &state_[i], &last_beat_[i]);
    trajectory_[i].resize(ocpX_states_*N_);
    swarm_trajectory_[i].initialize(nh_, boat_ids_[i], &trajectory_[i], &last_beat_[i]);
    copy_[i].resize(ocpX_states_*N_*(n_boats_-1));
    swarm_copy_[i].initialize(nh_, boat_ids_[i], &copy_[i], &last_beat_[i]);
    multiplier_[i].resize(ocpX_states_*N_*(n_boats_-1));
    swarm_multiplier_[i].initialize(nh_, boat_ids_[i], &multiplier_[i], &last_beat_[i]);
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

  (*state_)[0] = msg.data[0];
  (*state_)[1] = msg.data[1];
  (*state_)[2] = msg.data[2];

  (*state_)[3] = msg.data[3];
  (*state_)[4] = msg.data[4];
  (*state_)[5] = msg.data[5];

  return;
}

SwarmTrajectory::SwarmTrajectory()
{
}

void SwarmTrajectory::initialize(ros::NodeHandle &nh, std::string id, std::vector<double> *trajectory, double *last_beat)
{
  nh_ = nh;
  trajectory_ = trajectory;
  last_beat_ = last_beat;
  // subscribe to the topic (keep "/" apart so topic name is relative as in the rest of code)
  std::string topic = "/" + id + "/" + "trajectory";
  // ROS_INFO("rostopic subscribed: %s", topic.c_str());
  trajectory_sub_ = nh_.subscribe(topic, 1, &SwarmTrajectory::trajectoryCallback, this);

  return;
}

void SwarmTrajectory::trajectoryCallback(const roboat_core::FloatArray msg)
{
  (*trajectory_) = msg.data;

  return;
}

SwarmCopy::SwarmCopy()
{
}

void SwarmCopy::initialize(ros::NodeHandle &nh, std::string id, std::vector<double> *copy, double *last_beat)
{
  nh_ = nh;
  copy_ = copy;
  last_beat_ = last_beat;
  // subscribe to the topic (keep "/" apart so topic name is relative as in the rest of code)
  std::string topic = "/" + id + "/" + "local_copies";
  // ROS_INFO("rostopic subscribed: %s", topic.c_str());
  copy_sub_ = nh_.subscribe(topic, 1, &SwarmCopy::copyCallback, this);

  return;
}

void SwarmCopy::copyCallback(const roboat_core::FloatArray msg)
{
  (*copy_) = msg.data;

  return;
}

SwarmMultiplier::SwarmMultiplier()
{
}

void SwarmMultiplier::initialize(ros::NodeHandle &nh, std::string id, std::vector<double> *multiplier, double *last_beat)
{
  nh_ = nh;
  multiplier_ = multiplier;
  last_beat_ = last_beat;
  // subscribe to the topic (keep "/" apart so topic name is relative as in the rest of code)
  std::string topic = "/" + id + "/" + "lambda_multipliers";
  // ROS_INFO("rostopic subscribed: %s", topic.c_str());
  multiplier_sub_ = nh_.subscribe(topic, 1, &SwarmMultiplier::multiplierCallback, this);

  return;
}

void SwarmMultiplier::multiplierCallback(const roboat_core::FloatArray msg)
{
  (*multiplier_) = msg.data;

  return;
}
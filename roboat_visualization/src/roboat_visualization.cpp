#include <roboat_visualization/roboat_visualization.h>

using namespace std;

RoboatVisualization::RoboatVisualization() : dx(0), dy(0) {};

void RoboatVisualization::initialize(ros::NodeHandle &nh, std::string id)
{
  //loads node into class
  nh_=nh;
  
  //builds name space, base_link, and id based on input parameters
  std::string ns;
  if (id.empty()) {
    ns=id; //keeps it empty so it uses relative namespace where class is loaded
    base_link_frame_="base_link";
    id_="roboat";
  }
  else {
    ns="/"+id+"/"; //uses id to build global namespace
    base_link_frame_ = "base_link_"+id;
    id_=id;
  }

  nh.getParam("/visualization/update_rate",update_rate);
  nh.getParam("/visualization/distance_till_update",distance_till_update);
  nh.getParam("/visualization/path_length",path_length);
  
  pub_path = nh.advertise<nav_msgs::Path>(ns+"visual/path", 1);
  pub_marker = nh.advertise<visualization_msgs::Marker>(ns+"visual/robot", 1);
  pub_force_marker = nh.advertise<visualization_msgs::Marker>(ns+"visual/hud/force", 1);
  pub_priority_marker = nh.advertise<visualization_msgs::Marker>(ns+"visual/hud/priority", 1);

  sub_priority = nh.subscribe(ns+"thrust_state", 1, &RoboatVisualization::thrustStateCallback, this);
  sub_force = nh.subscribe(ns+"force", 1, &RoboatVisualization::forceCallback, this);
  current_odometry = nh.subscribe(ns+"odometry/filtered", 1, &RoboatVisualization::odometryCallback, this);

  path_update_timer = nh.createTimer(ros::Duration(1.0/update_rate), &RoboatVisualization::pathHandler, this);
}

void RoboatVisualization::thrustStateCallback(const roboat_msgs::ThrustState& msg)
{
  current_thrust_state = msg;
}

void RoboatVisualization::odometryCallback(const nav_msgs::Odometry& msg)
{
  dx = msg.twist.twist.linear.x;
  dy = msg.twist.twist.linear.y;
  dTheta = msg.twist.twist.angular.z;
  dx *= 3.6;
  dy *= 3.6;
}

void RoboatVisualization::forceCallback(const roboat_core::Force& msg)
{
  f1 = msg.data[0];
  f2 = msg.data[1];
  f3 = msg.data[2];
  f4 = msg.data[3];
}

void RoboatVisualization::pathHandler(const ros::TimerEvent& event)
{
  // map -> odom static transformation (both are the same)
  static tf::TransformBroadcaster tf_broadcast;
  static tf::Transform map_to_odom = tf::Transform(tf::createQuaternionFromRPY(0, 0, 0), tf::Vector3(0, 0, 0));
  tf_broadcast.sendTransform(tf::StampedTransform(map_to_odom, ros::Time::now(), "map", "odom"));

  /*WARNING: for the transforms to work, time need to be synchronized between the server and the miniboats, 
      otherwise lister will fail and no miniboat is plotted */
  static tf::TransformListener listener;
  static tf::StampedTransform transform;
  try
  {
    listener.lookupTransform("map", base_link_frame_, ros::Time(0), transform);
  }
  catch (tf::TransformException ex)
  { 
    // ROS_INFO("Transfrom missing: %s",base_link_frame_.c_str());
    return;
  }

  geometry_msgs::TransformStamped trans_stamped;
  transformStampedTFToMsg(transform, trans_stamped);
  geometry_msgs::PoseStamped pose_stamped;
  pose_stamped.header = trans_stamped.header;
  pose_stamped.pose.position.x = trans_stamped.transform.translation.x;
  pose_stamped.pose.position.y = trans_stamped.transform.translation.y;
  pose_stamped.pose.position.z = trans_stamped.transform.translation.z;
  pose_stamped.pose.orientation.x = trans_stamped.transform.rotation.x;
  pose_stamped.pose.orientation.y = trans_stamped.transform.rotation.y;
  pose_stamped.pose.orientation.z = trans_stamped.transform.rotation.z;
  pose_stamped.pose.orientation.w = trans_stamped.transform.rotation.w;

  visualizeRobot(pose_stamped);

  static double last_x = transform.getOrigin().x();
  static double last_y = transform.getOrigin().y();
  static double last_z = transform.getOrigin().z();
  double new_x = transform.getOrigin().x();
  double new_y = transform.getOrigin().y();
  double new_z = transform.getOrigin().z();

  // We only update the path when we change position beyond certain value
  if (sqrt((last_x - new_x) * (last_x - new_x) + (last_y - new_y) * (last_y - new_y) +
            (last_z - new_z) * (last_z - new_z)) >= distance_till_update)
  {
    last_x = new_x;
    last_y = new_y;
    last_z = new_z;
  }
  else
    return;

  // Update the visual path for the roboat
  static nav_msgs::Path pathMsg;
  pathMsg.header = pose_stamped.header;
  pathMsg.poses.push_back(pose_stamped);

  if (pub_path.getNumSubscribers() != 0)
    pub_path.publish(pathMsg);

  while (!pathMsg.poses.empty() &&
          pathMsg.poses.front().header.stamp.toSec() < pathMsg.poses.back().header.stamp.toSec() - path_length)
    pathMsg.poses.erase(pathMsg.poses.begin());
}

void RoboatVisualization::visualizeRobot(geometry_msgs::PoseStamped pose_stamped)
{
  static visualization_msgs::Marker marker_node;
  static visualization_msgs::Marker priority_node;
  static visualization_msgs::Marker force_marker;
  static bool initialized = false;
  if (!initialized)
  {
    marker_node.action = visualization_msgs::Marker::ADD;
    marker_node.type = visualization_msgs::Marker::MESH_RESOURCE;
    marker_node.mesh_resource = "package://roboat_visualization/meshes/miniboat.obj";
    marker_node.ns = id_;
    marker_node.id = 0;
    marker_node.scale.x = 1;
    marker_node.scale.y = 1;
    marker_node.scale.z = 1;
    marker_node.color.a = 1;

    priority_node.action = visualization_msgs::Marker::ADD;
    priority_node.type = visualization_msgs::Marker::TEXT_VIEW_FACING;
    priority_node.ns = id_;
    priority_node.id = 1;
    priority_node.pose.position.z = 3;
    priority_node.scale.z = 2;
    priority_node.color.r = 0.1;
    priority_node.color.g = 0.1;
    priority_node.color.b = 0.1;
    priority_node.color.a = 1;

    force_marker.action = visualization_msgs::Marker::ADD;
    force_marker.type = visualization_msgs::Marker::TEXT_VIEW_FACING;
    force_marker.ns = id_;
    force_marker.id = 1;
    force_marker.pose.position.z = 3;
    force_marker.scale.z = 2;
    force_marker.color.r = 0.1;
    force_marker.color.g = 0.1;
    force_marker.color.b = 0.1;
    force_marker.color.a = 1;
    initialized = true;
  }

  marker_node.header = pose_stamped.header;
  marker_node.pose = pose_stamped.pose;

  priority_node.header = pose_stamped.header;
  priority_node.pose.position.x = pose_stamped.pose.position.x;

  // We offset the text to the roboat's position for clearer viewing
  int offset = 5;
  priority_node.pose.position.y = pose_stamped.pose.position.y + offset;
  priority_node.pose.position.x = pose_stamped.pose.position.x + offset;

  marker_node.color.r = 0.95 - (current_thrust_state.priority * 0.2);
  marker_node.color.g = 0.95 - (current_thrust_state.priority * 0.2);
  marker_node.color.b = 0.95 - (current_thrust_state.priority * 0.2);

  std::ostringstream stream_obj;
  stream_obj << std::fixed;
  stream_obj << std::setprecision(2);
  stream_obj << dx;

  std::ostringstream stream_obj2;
  stream_obj2 << std::fixed;
  stream_obj2 << std::setprecision(2);
  stream_obj2 << dy;

  std::ostringstream stream_obj3;
  stream_obj3 << std::fixed;
  stream_obj3 << std::setprecision(2);
  stream_obj3 << dTheta;

  priority_node.text = "Priority: " + TH_CMD_PRIORITY_STRING[current_thrust_state.priority] +
                        "\nx': " + stream_obj.str() + "\ny': " + stream_obj2.str() + "\ntheta': " + stream_obj3.str();

  force_marker.header = pose_stamped.header;
  force_marker.pose = pose_stamped.pose;

  force_marker.header = pose_stamped.header;
  force_marker.pose.position.x = pose_stamped.pose.position.x;

  int negative_offset = -5;
  force_marker.pose.position.y = pose_stamped.pose.position.y + negative_offset;
  force_marker.pose.position.x = pose_stamped.pose.position.x + negative_offset;

  std::ostringstream stream_force1;
  stream_force1 << std::fixed;
  stream_force1 << std::setprecision(2);
  stream_force1 << f1;
  std::ostringstream stream_force2;
  stream_force2 << std::fixed;
  stream_force2 << std::setprecision(2);
  stream_force2 << f2;
  std::ostringstream stream_force3;
  stream_force3 << std::fixed;
  stream_force3 << std::setprecision(2);
  stream_force3 << f3;
  std::ostringstream stream_force4;
  stream_force4 << std::fixed;
  stream_force4 << std::setprecision(2);
  stream_force4 << f4;

  force_marker.text = "f1: " + stream_force1.str() + "\nf2: " + stream_force2.str() + "\nf3: " + stream_force3.str() +
                      "\nf4: " + stream_force4.str();

  pub_priority_marker.publish(priority_node);
  pub_marker.publish(marker_node);
  pub_force_marker.publish(force_marker);
}

/* ************************************************************************* */
/* LATCHING CONTROL NODE */
/* ************************************************************************* */

#include <roboat_core/swarm.h>
#include <ros/ros.h>

int main(int argc, char** argv)
{
  ros::init(argc, argv, "swarm_node");
  ros::NodeHandle nh;
  //ros::NodeHandle priv_nh("~"); //in case we want to load a private node for private topics within the class
  
  //Loads the class
  ROS_INFO("SETUP");
  Swarm swarm(nh);
  ROS_INFO("SETUP DONE");

  //spins the node
  ros::spin();
  
  return 0;
}
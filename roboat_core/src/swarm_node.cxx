/* ************************************************************************* */
/* SWARM COORDINATION NODE */
/* ************************************************************************* */

#include <roboat_core/swarm.h>
#include <ros/ros.h>

int main(int argc, char **argv)
{
  ros::init(argc, argv, "swarm_node");
  ros::NodeHandle nh;
  // ros::NodeHandle priv_nh("~"); //in case we want to load a private node for private topics within the class

  // Loads the class
  ROS_INFO("SETUP");
  Swarm swarm(nh);
  ROS_INFO("SETUP DONE");

  // spins the node
  //  ros::spin();

  // publishes the positions stored synchronously instead for testing
  ros::Rate loop_rate(50);
  int n_boats = swarm.getBoatN();
  std::vector<std::string> ids = swarm.getBoatNames();
  while (ros::ok())
  {
    ros::spinOnce();

    std::stringstream ss;
    for (int i = 0; i < n_boats; i++)
    {
      ss << ids[i] << ": [" << swarm.state_[i][0] << ","
         << swarm.state_[i][1] << ","
         << swarm.state_[i][2] * 180.0 / M_PI << "]; ";
    }
    ROS_INFO("%s", ss.str().c_str());

    // waits until next iteration ----------------
    loop_rate.sleep();
  }

  return 0;
}
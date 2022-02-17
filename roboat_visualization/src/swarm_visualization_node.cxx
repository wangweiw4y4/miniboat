/* ************************************************************************* */
/* SWARM VISUALIZATION NODE */
/* ************************************************************************* */

#include <roboat_visualization/roboat_visualization.h>

int main(int argc, char** argv)
{
  ros::init(argc, argv, "swarm_visualization_node");
  ros::NodeHandle nh;
  
  std::vector<std::string> boat_ids;
  nh.getParam("swarm/boat_ids",boat_ids);
  
  std::vector<RoboatVisualization> RV(boat_ids.size());
  for (uint i = 0; i<RV.size(); i++) {
    RV[i].initialize(nh,boat_ids[i]);
  }

  ROS_INFO("\033[1;32m----> swarm visualization node \033[0m");

  ros::spin();

  return 0;
}

// Copyright (c) 2021. MIT & AMS institute - Roboat.
#include "roboat_localization/data_filter.h"

/*simple test node that publish regularly a fixed position, excecuted directly from terminal running:
rosrun roboat_localization test_node __ns:=<miniboat_id>
*/

int main(int argc, char **argv) {
  ros::init(argc, argv, "roboat_localization_test");
  ros::NodeHandle nh;

  ros::Rate loop_rate(10);

  ros::Publisher hedge_pos_ang_publisher = nh.advertise<marvelmind_nav::hedge_pos_ang>("hedge_pos_ang", 10);

  marvelmind_nav::hedge_pos_ang odom_msg;
  odom_msg.x_m=0.1;
  odom_msg.y_m=0.1;
  odom_msg.z_m=0.0;
  odom_msg.angle=0.0;
  odom_msg.timestamp_ms=ros::Time::now().toSec();
  
  // synchronously 
  while (ros::ok())
  {
    ros::spinOnce();
    
    hedge_pos_ang_publisher.publish(odom_msg);
    
    loop_rate.sleep();

  }

  return 0;
}

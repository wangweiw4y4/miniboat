#include <ros/ros.h>

#include <std_msgs/String.h>

int main(int argc, char** argv)
{
    ros::init(argc, argv, "miniboat");

    ros::NodeHandle nh;

    ros::Publisher pub = nh.advertise<std_msgs::String> ("boat_topic", 1);

    std::string id;
    nh.param<std::string>("/robot_id", id, "not_valid");

    std_msgs::String ros_s;
    ros_s.data = "I am a miniboat, my id is -> " + id;

    ros::Rate r(2);
    while (ros::ok())
    {
        if (pub.getNumSubscribers() != 0)
        {
            pub.publish(ros_s);
            std::cout << "Some one is subscribing me, my id is -> " + id << std::endl;
        }
        r.sleep();
    }
    
    return 0;
}
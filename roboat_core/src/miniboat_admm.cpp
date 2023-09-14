/** ----------------------------------------------------------------------------
 * @file:     miniboat_pf.cpp
 * @date:     September 8, 2023
 * @datemod:  September 8, 2023
 * @author:   Alejandro Gonzalez-Garcia
 * @email:    alexglzg97@gmail.com
 * 
 * @brief: Potential fields algortithm and PID for distributed formation control. 
 * ---------------------------------------------------------------------------*/

#include <iostream>
#include "ros/ros.h"
#include "geometry_msgs/Pose2D.h"
#include "geometry_msgs/Vector3.h"
#include "nav_msgs/Odometry.h"
#include "std_msgs/Float64.h"
#include "std_msgs/UInt8.h"
#include <math.h>
#include <eigen3/Eigen/Dense>
#include <tf2/LinearMath/Quaternion.h>
#include "roboat_core/swarm_admm.h"
#include "roboat_core/FloatArray.h"

using namespace Eigen;

class MiniboatADMM
{
public:

    Vector2f reference;
    
    float value_;

    int N;
    int ocpX_states;
    
    Swarm swarm_;
    int swarm_size_, idx_;
    double detection_max_lapse_;

    roboat_core::FloatArray trajectory;
    roboat_core::FloatArray local_copies;
    roboat_core::FloatArray lambda_multipliers;

    std::vector<double> new_trajectory;
    std::vector<double> new_local_copies;
    std::vector<double> new_lambda_multipliers;
    double zeta_ji[14][40];
    double lambda_ji[14][40];

    MiniboatADMM()
    {
        //ROS Publishers and Subscribers
        trajectory_pub = nh.advertise<roboat_core::FloatArray>("trajectory", 1);
        local_copies_pub = nh.advertise<roboat_core::FloatArray>("local_copies", 1);
        lambda_multipliers_pub = nh.advertise<roboat_core::FloatArray>("lambda_multipliers", 1);

        reference_pose_sub = nh.subscribe("assignment/reference_pose", 1, &MiniboatADMM::reference_callback, this);

        swarm_.initialize(nh);
        swarm_size_ = swarm_.getBoatN();
        idx_ = swarm_.getIdx();
        if (idx_ == -1 || swarm_size_ <= 0)
        {
            ROS_ERROR("wrong swarm definition");
            idx_ = 0;
        }
        nh.param("/pf/detection_max_lapse_", detection_max_lapse_, 1.0);

        std::string id, ns;
        nh.param<std::string>("roboat_id", id, "");
        if (!id.empty())
        {
            ns = "/" + id + "/"; // uses id to build global namespace
            // shape_msg_.header.frame_id = "odom_" + id;
        }
        else
        {
            ns = id;
            // shape_msg_.header.frame_id = "odom";
        }

        nh.param("swarm/N", N, 40);
        nh.param("swarm/ocpX_states", ocpX_states, 2);

    }

    void reference_callback(const geometry_msgs::Pose2D::ConstPtr& _ref)
    {
        reference(0) = _ref->x; //ref in x
        reference(1) = _ref->y; //ref in y
    }

    void time_step()
    {
        new_trajectory.clear();
        for (int i = 0; i < 80; i++)
        {
            new_trajectory.push_back(0.0);
        } 
        trajectory.data = new_trajectory;
        trajectory_pub.publish(trajectory);

        new_local_copies.clear();
        for (int i = 0; i < 560; i++)
        {
            new_local_copies.push_back(idx_);
        }
        local_copies.data = new_local_copies;
        local_copies_pub.publish(local_copies);

        new_lambda_multipliers.clear();
        for (int i = 0; i < 560; i++)
        {
            new_lambda_multipliers.push_back(idx_);
        }
        lambda_multipliers.data = new_lambda_multipliers;
        lambda_multipliers_pub.publish(lambda_multipliers);

        //TODO: Read trajectory array
        //value_ = swarm_.trajectory_[0][0];
        //ROS_ERROR("%f", value_);

        for (int k = 0; k < swarm_size_; k++) {
            if (k == idx_) {
                continue;
            }
            if (k < idx_) {
                for (int l = 0; l < N; l++) {
                zeta_ji[2*k][l]     = swarm_.copy_[k][2*(idx_-1)*N+l];
                zeta_ji[(2*k)+1][l] = swarm_.copy_[k][2*(idx_-1)*N+l+N];
                }
            }
            if (k > idx_) {
                for (int l = 0; l < N; l++) {
                zeta_ji[2*(k-1)][l]   = swarm_.copy_[k][2*(idx_)*N+l];
                zeta_ji[2*(k-1)+1][l] = swarm_.copy_[k][2*(idx_)*N+l+N];
                }
            }
        }

        for (int k = 0; k < swarm_size_; k++) {
            if (k == idx_) {
                continue;
            }
            if (k < idx_) {
                for (int l = 0; l < N; l++) {
                lambda_ji[2*k][l]     = swarm_.multiplier_[k][2*(idx_-1)*N+l];
                lambda_ji[(2*k)+1][l] = swarm_.multiplier_[k][2*(idx_-1)*N+l+N];
                }
            }
            if (k > idx_) {
                for (int l = 0; l < N; l++) {
                lambda_ji[2*(k-1)][l]   = swarm_.multiplier_[k][2*(idx_)*N+l];
                lambda_ji[2*(k-1)+1][l] = swarm_.multiplier_[k][2*(idx_)*N+l+N];
                }
            }
        }

    }

private:
    ros::NodeHandle nh;
    ros::Publisher trajectory_pub;
    ros::Publisher local_copies_pub;
    ros::Publisher lambda_multipliers_pub;
    ros::Subscriber reference_pose_sub;

};

//Main
int main(int argc, char *argv[])
{
    ros::init(argc, argv, "miniboat_admm");
    MiniboatADMM miniboatADMM;
    int rate = 10;
    ros::Rate loop_rate(rate);
    ros::Duration(1).sleep();

  while (ros::ok())
  {
    miniboatADMM.time_step();
    ros::spinOnce();
    loop_rate.sleep();
  }

    return 0;
}
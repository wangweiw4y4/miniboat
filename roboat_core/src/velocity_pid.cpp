#include "ros/ros.h"
#include "std_msgs/String.h"

#include <ros/package.h>
#include "std_msgs/String.h"
#include "std_msgs/Int16.h"
#include <std_msgs/Float32MultiArray.h>
#include <std_msgs/Float64.h>
#include <std_msgs/Float64MultiArray.h>
#include <std_msgs/Int16MultiArray.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseArray.h>
#include <tf/transform_datatypes.h>
#include <tf/transform_broadcaster.h>
#include <tf/transform_listener.h>
#include <tf/transform_datatypes.h>
#include <nav_msgs/Odometry.h>
// other
#include <Eigen/Dense>
#include <math.h>
#include <iostream>
#include <algorithm>
#include <vector>

#include <roboat_core/Force.h>

#define ROS_NODE_NAME "velocity_pid"


Eigen::VectorXd force(4);

Eigen::MatrixXd B(3,4);
Eigen::MatrixXd B_inv(4,3);

std::vector<double> state(6);

class ProportionalIntegralDerivative
{
public:

    double orientation_qx;
    double orientation_qy;
    double orientation_qz;
    double orientation_qw;

    double pid_maxforce = 0.05;
    double base_force = 0.1;
    double a = 0.12;

    double p_u = 3.0;
    double i_u = 2.0;
    double d_u = 0.1;

    double p_v = 3.0;
    double i_v = 2.0;
    double d_v = 0.1;

    double p_psi = 3.0;
    double i_psi = 2.0;
    double d_psi = 0.1;

    bool pid_flag = true;

    double step = 0.1;

    double req_theta = M_PI / 4;
    double req_force = 0.25;

    double desired_yaw = 0.0;
    double desired_angular_velocity = 0;
    double desired_u = 0.0;
    double desired_v = 0.0;

    double eu;
    double eu_last = 0;
    double eui;
    double eud;
    double ev;
    double ev_last = 0;
    double evi;
    double evd;
    double epsi;
    double epsi_last = 0;
    double epsii;
    double epsid;

    double Tu;
    double Tv;
    double Tr;
    Eigen::Vector3d miniboat_tau;

    Eigen::Matrix2d rotation;

    roboat_core::Force forceMsg;

    ProportionalIntegralDerivative()
    {

        force_pub = n.advertise<roboat_core::Force>("pid_force", 1);
        state_sub = n.subscribe("odometry/filtered", 1, &ProportionalIntegralDerivative::stateCallback, this);

        static const double dp_u = 0.5;
        static const double di_u = 0.2;
        static const double dd_u = 0.1;
        static const double dp_v = 0.5;
        static const double di_v = 0.2;
        static const double dd_v = 0.1;
        static const double dp_psi = 0.5;
        static const double di_psi = 0.2;
        static const double dd_psi = 0.1;

        n.param("/position_pid/p_u", p_u, dp_u);
        n.param("/position_pid/i_u", i_u, di_u);
        n.param("/position_pid/d_u", d_u, dd_u);
        n.param("/position_pid/p_v", p_v, dp_v);
        n.param("/position_pid/i_v", i_v, di_v);
        n.param("/position_pid/d_v", d_v, dd_v);
        n.param("/position_pid/p_psi", p_psi, dp_psi);
        n.param("/position_pid/i_psi", i_psi, di_psi);
        n.param("/position_pid/d_psi", d_psi, dd_psi);

        desired_yaw = 0.0;
        desired_angular_velocity = 0;
        desired_u = 0.0;
        desired_v = 0.0;

        state[0] = 0.0;
        state[1] = 0.0;
        state[2] = 0.0;
        state[3] = 0.0;
        state[4] = 0.0;
        state[5] = 0.0;

        eu_last = 0;
        eu_last = 0;
        epsi_last = 0;
        
        force = Eigen::VectorXd::Zero(4);
    }

    void stateCallback(const nav_msgs::Odometry msg)
    {
        double yaw;

        state[0] = msg.pose.pose.position.x;
        state[1] = -msg.pose.pose.position.y; //
        yaw = -tf::getYaw(tf::Quaternion(msg.pose.pose.orientation.x, msg.pose.pose.orientation.y,
                                        msg.pose.pose.orientation.z, msg.pose.pose.orientation.w));

        state[2] = yaw;
        state[3] = msg.twist.twist.linear.x;
        state[4] = -msg.twist.twist.linear.y;
        state[5] = -msg.twist.twist.angular.z;

        ROS_INFO("miniboat state is %f, %f, %f, %f, %f, %f", state[0], state[1], state[2], state[3], state[4], state[5]);
    }

    double pos_tau(double tau)
    {
        double val;
        if (tau >= 0){
            val = 1;
        }
        else{
            val = 0;
        }
        return val;
    }

    double neg_tau(double tau)
    {
        double val;
        if (tau <= 0){
            val = -1;
        }
        else{
            val = 0;
        }
        return val;
    }

    void control()
    {
        if (pid_flag)
        {
            eu = desired_u - state[3];
            eui = (step)*(eu + eu_last)/2 + eui; //integral of the surge speed error
            eud = (eu - eu_last) / step; //derivate of the surge speed error
            eu_last = eu;

            ev = desired_v - state[4];
            evi = (step)*(ev + ev_last)/2 + evi; //integral of the sway speed error
            evd = (ev - ev_last) / step; //derivate of the sway speed error
            ev_last = ev;

            epsi = desired_yaw - state[2];
            if (abs(epsi) > M_PI)
            {
                epsi = (epsi/abs(epsi))*(abs(epsi)-2*M_PI);
            }
            epsid = desired_angular_velocity - state[5];
            epsii = (step)*(epsi + epsi_last)/2 + epsii;
            epsi_last = epsi;

            Tu = (p_u * eu) + (i_u * eui) + (d_u * eud);
            Tv = (p_v * ev) + (i_v * evi) + (d_v * evd);
            Tr = (p_psi * epsi) + (i_psi * epsii) + (d_psi * epsid);
            
            miniboat_tau << Tu, Tv, Tr;

            B << pos_tau(Tu), pos_tau(Tu), neg_tau(Tu), neg_tau(Tu),
                pos_tau(Tv), neg_tau(Tv), pos_tau(Tv), neg_tau(Tv),
                a*pos_tau(Tr), a*neg_tau(Tr), a*neg_tau(Tr), a*pos_tau(Tr);

            B_inv = B.transpose()*(B*B.transpose()).inverse();

            force = B_inv*miniboat_tau;

            if (force(0) > pid_maxforce){
                force(0) = pid_maxforce;
            }
            if (force(1) > pid_maxforce){
                force(1) = pid_maxforce;
            }
            if (force(2) > pid_maxforce){
                force(2) = pid_maxforce;
            }
            if (force(3) > pid_maxforce){
                force(3) = pid_maxforce;
            }
            ROS_INFO("pid force:  %f,%f,%f,%f\n", force(0), force(1), force(2),force(3));

            Eigen::VectorXd::Map(&forceMsg.data[0], force.size()) = force;
            force_pub.publish(forceMsg);

        }
    }

    private:
        ros::NodeHandle n;

        ros::Publisher force_pub;

        ros::Subscriber state_sub;
};

int main(int argc, char *argv[])
{
    ros::init(argc,argv,ROS_NODE_NAME);
    ProportionalIntegralDerivative proportionalIntegralDerivative;
    int rate = 10;
    ros::Rate loop_rate(rate);

    while (ros::ok())
    {
        proportionalIntegralDerivative.control();
        ros::spinOnce();
        loop_rate.sleep();
    }
    return 0;
}
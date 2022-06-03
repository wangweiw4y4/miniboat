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

#define ROS_NODE_NAME "position_pid"

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

    double p_x = 3.0;
    double i_x = 2.0;
    double d_x = 0.1;

    double p_y = 3.0;
    double i_y = 2.0;
    double d_y = 0.1;

    double p_psi = 3.0;
    double i_psi = 2.0;
    double d_psi = 0.1;

    bool pid_flag = true;

    double step = 0.1;

    double req_theta = M_PI / 4;
    double req_force = 0.25;

    double desired_yaw = 0.0;
    double desired_angular_velocity = 0;
    double desired_x = 2.0;
    double desired_x_velocity = 0;
    double desired_y = 1.5;
    double desired_y_velocity = 0;

    std::vector<double> state;
    double x_dot;
    double y_dot;

    double ex;
    double ex_last = 0;
    double exi;
    double exd;
    double ey;
    double ey_last = 0;
    double eyi;
    double eyd;
    double epsi;
    double epsi_last = 0;
    double epsii;
    double epsid;

    Eigen::VectorXd force;
    double Tx;
    double Ty;
    double Tu;
    double Tv;
    double Tr;
    Eigen::Vector2d T_inertial;
    Eigen::Vector2d T_body;
    Eigen::Vector3d miniboat_tau;

    Eigen::Matrix2d rotation;
    Eigen::MatrixXd B;
    Eigen::MatrixXd B_inv;

    roboat_core::Force forceMsg;

    ProportionalIntegralDerivative()
    {

        force_pub = n.advertise<roboat_core::Force>("pid_force", 1);
        state_sub = n.subscribe("odometry/filtered", 1, &ProportionalIntegralDerivative::stateCallback, this);

        static const double dp_x = 0.5;
        static const double di_x = 0.2;
        static const double dd_x = 0.1;
        static const double dp_y = 0.5;
        static const double di_y = 0.2;
        static const double dd_y = 0.1;
        static const double dp_psi = 0.5;
        static const double di_psi = 0.2;
        static const double dd_psi = 0.1;

        n.param("/position_pid/p_x", p_x, dp_x);
        n.param("/position_pid/i_x", i_x, di_x);
        n.param("/position_pid/d_x", d_x, dd_x);
        n.param("/position_pid/p_y", p_y, dp_y);
        n.param("/position_pid/i_y", i_y, di_y);
        n.param("/position_pid/d_y", d_y, dd_y);
        n.param("/position_pid/p_psi", p_psi, dp_psi);
        n.param("/position_pid/i_psi", i_psi, di_psi);
        n.param("/position_pid/d_psi", d_psi, dd_psi);

        desired_yaw = 0.0;
        desired_angular_velocity = 0;
        desired_x = 2.0;
        desired_x_velocity = 0;
        desired_y = 1.5;
        desired_y_velocity = 0;

        ex_last = 0;
        ey_last = 0;
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
            ex = desired_x - state[0];
            x_dot = state[3]*cos(state[2]) - state[4]*sin(state[2]);
            exd = desired_x_velocity - x_dot;
            exi = (ex - ex_last) / step;
            ex_last = ex;

            ey = desired_y - state[1];
            y_dot = state[3]*sin(state[2]) + state[4]*cos(state[2]);
            eyd = desired_y_velocity - y_dot;
            eyi = (ey - ey_last) / step;
            ey_last = ey;

            epsi = desired_yaw - state[2];
            if (abs(epsi) > M_PI)
            {
                epsi = (epsi/abs(epsi))*(abs(epsi)-2*M_PI);
            }
            epsid = desired_angular_velocity - state[5];
            epsii = (epsi - epsi_last) / step;
            epsi_last = epsi;

            Tx = (p_x * ex) + (i_x * exi) + (d_x * exd);
            Ty = (p_y * ey) + (i_y * eyi) + (d_y * eyd);
            Tr = (p_psi * epsi) + (i_psi * epsii) + (d_psi * epsid);
            
            rotation << cos(state[2]), -sin(state[2]),
                        sin(state[2]), cos(state[2]);
            
            T_inertial(0) = Tx;
            T_inertial(1) = Ty;

            T_body = rotation.transpose()*T_inertial;

            Tu = T_body(0);
            Tv = T_body(1);
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
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

    double pid_maxforce = 0.4;
    double base_force = 0.1;
    double a = 0.12;
    double d = 0.078; //distance between thruster and boat center
    double cs45;
    double minf;
    double maxf;
    double pid_aux_1 = -1.42;
    double pid_aux_2 = -0.0063;

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

    double Tx;
    double Ty;
    double Tu;
    double Tv;
    double Tr;
    Eigen::Vector2d T_inertial;
    Eigen::Vector2d T_body;
    Eigen::Vector3d miniboat_tau;
    Eigen::Vector3d auxiliar_tau;

    Eigen::Matrix2d rotation;

    roboat_core::Force forceMsg;

    ProportionalIntegralDerivative()
    {

        force_pub = n.advertise<roboat_core::Force>("pid_force", 1);
        state_sub = n.subscribe("odometry/filtered", 1, &ProportionalIntegralDerivative::stateCallback, this);

        static const double dp_x = 0.005;
        static const double di_x = 0.002;
        static const double dd_x = 0.001;
        static const double dp_y = 0.005;
        static const double di_y = 0.002;
        static const double dd_y = 0.001;
        static const double dp_psi = 0.005;
        static const double di_psi = 0.002;
        static const double dd_psi = 0.001;

        n.param("position_pid/p_x", p_x, dp_x);
        n.param("position_pid/i_x", i_x, di_x);
        n.param("position_pid/d_x", d_x, dd_x);
        n.param("position_pid/p_y", p_y, dp_y);
        n.param("position_pid/i_y", i_y, di_y);
        n.param("position_pid/d_y", d_y, dd_y);
        n.param("position_pid/p_psi", p_psi, dp_psi);
        n.param("position_pid/i_psi", i_psi, di_psi);
        n.param("position_pid/d_psi", d_psi, dd_psi);
        desired_yaw = 0.0;
        desired_angular_velocity = 0;
        desired_x = 2.0;
        desired_x_velocity = 0;
        desired_y = 1.5;
        desired_y_velocity = 0;

        state[0] = 0.0;
        state[1] = 0.0;
        state[2] = 0.0;
        state[3] = 0.0;
        state[4] = 0.0;
        state[5] = 0.0;

        ex_last = 0;
        ey_last = 0;
        epsi_last = 0;
        
        force = Eigen::VectorXd::Zero(4);
        cs45 = sqrt(2)/2;
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

        ROS_WARN("miniboat state is %f, %f, %f, %f, %f, %f", state[0], state[1], state[2], state[3], state[4], state[5]);
    }

    void control()
    {
        if (pid_flag)
        {
            ex = desired_x - state[0];
            x_dot = state[3]*cos(state[2]) - state[4]*sin(state[2]);
            exd = desired_x_velocity - x_dot;
            exi = (step)*(ex + ex_last)/2 + exi;
            if (abs(ex) < 0.002)
            {
                exi = 0;
            }
            ex_last = ex;

            ey = desired_y - state[1];
            y_dot = state[3]*sin(state[2]) + state[4]*cos(state[2]);
            eyd = desired_y_velocity - y_dot;
            if (abs(ey) < 0.002)
            {
                eyi = 0;
            }
            eyi = (step)*(ey + ey_last)/2 + eyi;
            ey_last = ey;

            epsi = desired_yaw - state[2];
            if (abs(epsi) >= M_PI)
            {
                epsi = (epsi/abs(epsi))*(abs(epsi)-2*M_PI);
            }
            epsid = desired_angular_velocity - state[5];
            epsii = (step)*(epsi + epsi_last)/2 + epsii;
            if (abs(epsi) < 0.005)
            {
                epsii = 0;
            }
            epsi_last = epsi;

            Tx = (p_x * ex) + (i_x * exi) + (d_x * exd);
            Ty = (p_y * ey) + (i_y * eyi) + (d_y * eyd);
            Tr = (p_psi * epsi) + (i_psi * epsii) + (d_psi * epsid) - (pid_aux_1*state[3]*state[4]) - (pid_aux_2*sqrt(state[3]*state[3] + state[4]*state[4])*state[5]);
            
            rotation << cos(state[2]), -sin(state[2]),
                        sin(state[2]), cos(state[2]);
            
            T_inertial(0) = Tx;
            T_inertial(1) = Ty;

            T_body = rotation.transpose()*T_inertial;

            Tu = T_body(0);
            Tv = T_body(1);
            miniboat_tau << Tu, Tv, Tr;

            B << cs45, cs45, -cs45, -cs45,
                cs45, -cs45, cs45, -cs45,
                d, -d, -d, d;

            B_inv = B.transpose()*(B*B.transpose()).inverse();

            force = B_inv*miniboat_tau;

            minf = std::min(force(0),force(1));
            minf = std::min(force(1),force(2));
            minf = std::min(force(2),force(3));

            if (minf < 0)
            {
                force(0) = force(0) - minf;
                force(1) = force(1) - minf;
                force(2) = force(2) - minf;
                force(3) = force(3) - minf;
            }

            maxf = std::max(force(0),force(1));
            maxf = std::max(force(1),force(2));
            maxf = std::max(force(2),force(3));

            if (maxf > pid_maxforce)
            {
                force(0) = force(0)*pid_maxforce/maxf;
                force(1) = force(1)*pid_maxforce/maxf;
                force(2) = force(2)*pid_maxforce/maxf;
                force(3) = force(3)*pid_maxforce/maxf;
            }

            auxiliar_tau = B*force;
            auxiliar_tau(2) = miniboat_tau(2);

            force = B_inv*auxiliar_tau;

            minf = std::min(force(0),force(1));
            minf = std::min(force(1),force(2));
            minf = std::min(force(2),force(3));

            if (minf < 0)
            {
                force(0) = force(0) - minf;
                force(1) = force(1) - minf;
                force(2) = force(2) - minf;
                force(3) = force(3) - minf;
            }

            maxf = std::max(force(0),force(1));
            maxf = std::max(force(1),force(2));
            maxf = std::max(force(2),force(3));

            if (maxf > pid_maxforce)
            {
                force(0) = force(0)*pid_maxforce/maxf;
                force(1) = force(1)*pid_maxforce/maxf;
                force(2) = force(2)*pid_maxforce/maxf;
                force(3) = force(3)*pid_maxforce/maxf;
            }

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
            if (force(0) < 0){
                force(0) = 0;
            }
            if (force(1) < 0){
                force(1) = 0;
            }
            if (force(2) < 0){
                force(2) = 0;
            }
            if (force(3) < 0){
                force(3) = 0;
            }
            ROS_WARN("pid force:  %f,%f,%f,%f\n", force(0), force(1), force(2),force(3));
            ROS_WARN("PID error is %f, %f, %f", ex, ey, epsi);
            ROS_WARN("PID tau is %f, %f, %f, %f, %f", Tx, Ty, Tu, Tv, Tr);

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
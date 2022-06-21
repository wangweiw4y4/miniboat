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
#include <geometry_msgs/Pose2D.h>
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

#define ROS_NODE_NAME "velocity_cascade_smc"


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

    double pid_maxforce = 0.5;
    double max_r = 0.25;
    double base_force = 0.1;
    double a = 0.12;
    double d = 0.078; //distance between thruster and boat center
    double cs45;
    double minf;
    double maxf;
    double pid_aux_1 = -2.5;
    double pid_aux_2 = -0.1;
    double pid_aux_3 = -0.0;

    double k1_u;
    double k2_u;
    double lambda_u;

    double k1_v;
    double k2_v;
    double lambda_v;

    double k1_r;
    double k2_r;
    double lambda_r;

    double p_psi;
    double i_psi;
    double d_psi;

    bool pid_flag = true;

    double step = 0.02;

    double req_theta = M_PI / 4;
    double req_force = 0.25;

    double desired_yaw = 0.0;
    double desired_angular_velocity = 0;
    double desired_r = 0.0;
    double desired_u = 0.0;
    double desired_v = 0.0;

    double su;
    double sign_su;
    double eu;
    double eu_last = 0.0;
    double eui;
    double eud;
    double sv;
    double sign_sv;
    double ev;
    double ev_last = 0.0;
    double evi;
    double evd;
    double spsi;
    double sign_spsi;
    double epsi;
    double epsi_last = 0.0;
    double epsii;
    double epsid;
    double sr;
    double sign_sr;
    double er;
    double er_last = 0.0;
    double eri;
    double erd;

    double Tu;
    double Tv;
    double Tr;
    Eigen::Vector3d miniboat_tau;
    Eigen::Vector3d auxiliar_tau;

    Eigen::Matrix2d rotation;

    roboat_core::Force forceMsg;

    ProportionalIntegralDerivative()
    {

        force_pub = n.advertise<roboat_core::Force>("pid_force", 1);
        state_sub = n.subscribe("odometry/filtered", 1, &ProportionalIntegralDerivative::stateCallback, this);
        reference_sub = n.subscribe("velocity_reference", 1, &ProportionalIntegralDerivative::referenceCallback, this);

        static const double dk1_u = 0.1;
        static const double dk2_u = 0.001;
        static const double dlambda_u = 0.001;
        static const double dk1_v = 0.1;
        static const double dk2_v = 0.001;
        static const double dlambda_v = 0.001;
        static const double dk1_r = 0.1;
        static const double dk2_r = 0.001;
        static const double dlambda_r = 0.001;
        static const double dp_psi = 0.004;
        static const double di_psi = 0.0;
        static const double dd_psi = 0.006;

        n.param("velocity_cascade_smc/k1_u", k1_u, dk1_u);
        n.param("velocity_cascade_smc/k2_u", k2_u, dk2_u);
        n.param("velocity_cascade_smc/lambda_u", lambda_u, dlambda_u);
        n.param("velocity_cascade_smc/k1_v", k1_v, dk1_v);
        n.param("velocity_cascade_smc/k2_v", k2_v, dk2_v);
        n.param("velocity_cascade_smc/lambda_v", lambda_v, dlambda_v);
        n.param("velocity_cascade_smc/k1_r", k1_r, dk1_r);
        n.param("velocity_cascade_smc/k2_r", k2_r, dk2_r);
        n.param("velocity_cascade_smc/lambda_r", lambda_r, dlambda_r);
        n.param("velocity_cascade_smc/p_psi", p_psi, dp_psi);
        n.param("velocity_cascade_smc/i_psi", i_psi, di_psi);
        n.param("velocity_cascade_smc/d_psi", d_psi, dd_psi);

        desired_yaw = 0.0;
        desired_angular_velocity = 0;
        desired_r = 0;
        desired_u = 0.0;
        desired_v = 0.0;

        state[0] = 0.0;
        state[1] = 0.0;
        state[2] = 0.0;
        state[3] = 0.0;
        state[4] = 0.0;
        state[5] = 0.0;

        eu_last = 0.0;
        ev_last = 0.0;
        er_last = 0.0;
        epsi_last = 0.0;
        eui = 0.0;
        evi = 0.0;
        eri = 0.0;
        epsii = 0.0;

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

    void referenceCallback(const geometry_msgs::Pose2D msg)
    {
        desired_u = msg.x;
        desired_v = msg.y; 
    }

    void control()
    {
        if (pid_flag)
        {
            epsi = desired_yaw - state[2];
            if (abs(epsi) >= M_PI)
            {
                epsi = (epsi/abs(epsi))*(abs(epsi)-2.0*M_PI);
            }
            epsid = desired_angular_velocity - state[5];
            epsii = (step)*(epsi + epsi_last)/2.0 + epsii;
            if (abs(epsi) <= 0.005)
            {
                epsii = 0.0;
            }
            epsi_last = epsi;
            
            desired_r = (p_psi * epsi) + (i_psi * copysign(epsii,epsi)) + (d_psi * epsid);

            if (abs(epsi) <= 0.1 )
            {
            desired_r = (0.1* p_psi * epsi) + (i_psi * copysign(epsii,epsi)) + (d_psi * epsid);
            }

            if (abs(epsi) >= 1.5)
            {
            desired_r = (1.5* p_psi * epsi) + (i_psi * copysign(epsii,epsi)) + (d_psi * epsid);
            }

            if (abs(desired_r) >= max_r){
                desired_r = copysign(max_r,desired_r);
            }
            
            eu = desired_u - state[3];
            eui = (step)*(eu + eu_last)/2 + eui; //integral of the surge speed error
            eu_last = eu;
            su = eu + lambda_u*eui;

            ev = desired_v - state[4];
            evi = (step)*(ev + ev_last)/2 + evi; //integral of the sway speed error
            ev_last = ev;
            sv = ev + lambda_v*evi;

            er = desired_r - state[5];
            eri = (step)*(er + er_last)/2 + eri; //integral of the yaw speed error
            er_last = er;
            sr = er + lambda_r*eri;
            
            if (su == 0)
            {
                sign_su = 0;
            }
            else {
                sign_su = copysign(1,su);
            }

            if (sv == 0)
            {
                sign_sv = 0;
            }
            else {
                sign_sv = copysign(1,sv);
            }

            if (sr == 0)
            {
                sign_sr = 0;
            }
            else {
                sign_sr = copysign(1,sr);
            }

            Tu = k1_u*pow(abs(su),0.5)*sign_su + k2_u*su;
            Tv = k1_v*pow(abs(sv),0.5)*sign_sv + k2_v*sv;
            Tr = k1_r*pow(abs(sr),0.5)*sign_sr + k2_r*sr - (pid_aux_1*state[3]*state[4]) - (pid_aux_2*sqrt(state[3]*state[3] + state[4]*state[4])*state[5]);// - (pid_aux_2*abs(state[5])*state[5]);

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
            ROS_WARN("SMC desired r and psi error is %f, %f", desired_r, epsi);
            ROS_WARN("SMC force:  %f,%f,%f,%f\n", force(0), force(1), force(2),force(3));
            ROS_WARN("SMC error is %f, %f, %f", eu, ev, er);
            ROS_WARN("SMC tau is %f, %f, %f", Tu, Tv, Tr);

            Eigen::VectorXd::Map(&forceMsg.data[0], force.size()) = force;
            force_pub.publish(forceMsg);

        }
    }

    private:
        ros::NodeHandle n;

        ros::Publisher force_pub;

        ros::Subscriber state_sub;
        ros::Subscriber reference_sub;
};

int main(int argc, char *argv[])
{
    ros::init(argc,argv,ROS_NODE_NAME);
    ProportionalIntegralDerivative proportionalIntegralDerivative;
    int rate = 50;
    ros::Rate loop_rate(rate);

    while (ros::ok())
    {
        proportionalIntegralDerivative.control();
        ros::spinOnce();
        loop_rate.sleep();
    }
    return 0;
}
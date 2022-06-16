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

#define ROS_NODE_NAME "velocity_asmc"


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
    double pid_aux_1 = -3.5;//-1.42
    double pid_aux_2 = -0.1;//-0.0063

    double k1_u;
    double k2_u;
    double lambda_u;
    double ka_u;
    double mu_u;
    double km_u;
    double k1d_u;
    double k1last_u;

    double k1_v;
    double k2_v;
    double lambda_v;
    double ka_v;
    double mu_v;
    double km_v;
    double k1d_v;
    double k1last_v;

    double k1_psi;
    double k2_psi;
    double lambda_psi;
    double ka_psi;
    double mu_psi;
    double km_psi;
    double k1d_psi;
    double k1last_psi;

    bool pid_flag = true;

    double step = 0.02;

    double req_theta = M_PI / 4;
    double req_force = 0.25;

    double desired_yaw = 0.0;
    double desired_angular_velocity = 0;
    double desired_u = 0.0;
    double desired_v = 0.0;

    double su;
    double sign_su;
    double eu;
    double eu_last = 0;
    double eui;
    double eud;
    double sv;
    double sign_sv;
    double ev;
    double ev_last = 0;
    double evi;
    double evd;
    double spsi;
    double sign_spsi;
    double epsi;
    double epsi_last = 0;
    double epsii;
    double epsid;

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

        static const double dka_u = 0.01;
        static const double dk2_u = 0.001;
        static const double dlambda_u = 0.001;
        static const double dmu_u = 0.05;
        static const double dkm_u = 0.001;
        static const double dka_v = 0.01;
        static const double dk2_v = 0.001;
        static const double dlambda_v = 0.001;
        static const double dmu_v = 0.05;
        static const double dkm_v = 0.001;
        static const double dka_psi = 0.01;
        static const double dk2_psi = 0.001;
        static const double dlambda_psi = 1.0;
        static const double dmu_psi = 0.1;
        static const double dkm_psi = 0.001;

        n.param("velocity_asmc/ka_u", ka_u, dka_u);
        n.param("velocity_asmc/k2_u", k2_u, dk2_u);
        n.param("velocity_asmc/lambda_u", lambda_u, dlambda_u);
        n.param("velocity_asmc/mu_u", mu_u, dmu_u);
        n.param("velocity_asmc/km_u", km_u, dkm_u);
        n.param("velocity_asmc/ka_v", ka_v, dka_v);
        n.param("velocity_asmc/k2_v", k2_v, dk2_v);
        n.param("velocity_asmc/lambda_v", lambda_v, dlambda_v);
        n.param("velocity_asmc/mu_v", mu_v, dmu_v);
        n.param("velocity_asmc/km_v", km_v, dkm_v);
        n.param("velocity_asmc/ka_psi", ka_psi, dka_psi);
        n.param("velocity_asmc/k2_psi", k2_psi, dk2_psi);
        n.param("velocity_asmc/lambda_psi", lambda_psi, dlambda_psi);
        n.param("velocity_asmc/mu_psi", mu_psi, dmu_psi);
        n.param("velocity_asmc/km_psi", km_psi, dkm_psi);
 
        desired_yaw = 0.0;
        desired_angular_velocity = 0;
        desired_u = 0.08;
        desired_v = 0.08;

        state[0] = 0.0;
        state[1] = 0.0;
        state[2] = 0.0;
        state[3] = 0.0;
        state[4] = 0.0;
        state[5] = 0.0;

        eu_last = 0;
        eu_last = 0;
        epsi_last = 0;
        k1last_u = 0;
        k1last_v = 0;
        k1last_psi = 0;
        k1_u = km_u;
        k1_v = km_v;
        k1_psi = km_psi;
        
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
            eu = desired_u - state[3];
            eui = (step)*(eu + eu_last)/2 + eui; //integral of the surge speed error
            eu_last = eu;
            su = eu + lambda_u*eui;

            ev = desired_v - state[4];
            evi = (step)*(ev + ev_last)/2 + evi; //integral of the sway speed error
            ev_last = ev;
            sv = ev + lambda_v*evi;

            epsi = desired_yaw - state[2];
            if (abs(epsi) >= M_PI)
            {
                epsi = (epsi/abs(epsi))*(abs(epsi)-2*M_PI);
            }
            epsid = desired_angular_velocity - state[5];
            spsi = epsid + lambda_psi*epsi;

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

            if (spsi == 0)
            {
                sign_spsi = 0;
            }
            else {
                sign_spsi = copysign(1,spsi);
            }

            if (k1_u > km_u)
            {
                double signvar = abs(su) - mu_u;
                double sign_u_sm;
                if (signvar == 0)
                {
                    sign_u_sm = 0;
                }
                else 
                {
                    sign_u_sm = copysign(1,signvar);
                }
                k1d_u = ka_u * sign_u_sm;
            }
            else
            {
                k1d_u = km_u;
            }
            k1_u = (step)*(k1d_u + k1last_u)/2 + k1_u; //integral to get the speed adaptative gain
            k1last_u = k1d_u;

            if (k1_v > km_v)
            {
                double signvar = abs(sv) - mu_v;
                double sign_v_sm;
                if (signvar == 0)
                {
                    sign_v_sm = 0;
                }
                else 
                {
                    sign_v_sm = copysign(1,signvar);
                }
                k1d_v = ka_v * sign_v_sm;
            }
            else
            {
                k1d_v = km_v;
            }
            k1_v = (step)*(k1d_v + k1last_v)/2 + k1_v; //integral to get the speed adaptative gain
            k1last_v = k1d_v;

            if (k1_psi > km_psi)
            {
                double signvar = abs(spsi) - mu_psi;
                double sign_psi_sm;
                if (signvar == 0)
                {
                    sign_psi_sm = 0;
                }
                else 
                {
                    sign_psi_sm = copysign(1,signvar);
                }
                k1d_psi = ka_psi * sign_psi_sm;
            }
            else
            {
                k1d_psi = km_psi;
            }
            k1_psi = (step)*(k1d_psi + k1last_psi)/2 + k1_psi; //integral to get the speed adaptative gain
            k1last_psi = k1d_psi;

            Tu = k1_u*pow(abs(su),0.5)*sign_su + k2_u*su;
            Tv = k1_v*pow(abs(sv),0.5)*sign_sv + k2_v*sv;
            Tr = k1_psi*pow(abs(spsi),0.5)*sign_spsi + k2_psi*spsi - (pid_aux_1*abs(state[3])*state[4]) - (pid_aux_2*sqrt(state[3]*state[3] + state[4]*state[4])*state[5]);
            
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
            ROS_WARN("SMC force:  %f,%f,%f,%f\n", force(0), force(1), force(2),force(3));
            ROS_WARN("SMC error is %f, %f, %f", eu, ev, epsi);
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
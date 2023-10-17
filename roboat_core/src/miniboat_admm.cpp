/** ----------------------------------------------------------------------------
 * @file:     miniboat_admm.cpp
 * @date:     September 8, 2023
 * @datemod:  September 27, 2023
 * @author:   Alejandro Gonzalez-Garcia
 * @email:    alexglzg97@gmail.com
 * 
 * @brief: ADMM algorithm for distributed formation control. 
 * ---------------------------------------------------------------------------*/

#include <iostream>
#include "ros/ros.h"
#include "geometry_msgs/Pose2D.h"
#include "geometry_msgs/PoseStamped.h"
#include "geometry_msgs/Vector3.h"
#include "nav_msgs/Odometry.h"
#include "nav_msgs/Path.h"
#include "std_msgs/Float64.h"
#include "std_msgs/UInt16.h"
#include <math.h>
#include <eigen3/Eigen/Dense>
#include <tf2/LinearMath/Quaternion.h>
#include "roboat_core/swarm_admm.h"
#include "roboat_core/FloatArray.h"
#include <casadi/casadi.hpp>

using namespace Eigen;
using namespace casadi;

class MiniboatADMM
{
public:

    Vector2d reference;
    Matrix2d rotation;
    double psi;
    Vector2d body_vel;
    double max_allowed_vel;
    double max_vel;
    Vector2d global_vel;
    
    //Potential field position holder
    double distance;
    Vector2d last_reference;
    Vector2d pose;
    double k;
    double scale;
    
    bool admm_initialized;
    int counter;
    int counter_mpc;
    int counter_delatch;
    double desired_yaw;
    bool ocpX_flag;
    bool ocpZ_flag;

    int Nhor;
    int Nhor_plus_one;
    int ocpX_states;
    double mu;
    
    Swarm swarm_;
    int swarm_size_, idx_;
    double detection_max_lapse_;

    std::string FUNCTIONS_DIR;
    std::string FUNCTIONS_DIR_DEFAULT;
    casadi::Function ocpX_function;
    casadi::Function ocpZ_function;
    casadi::Function ocpC_function;

    roboat_core::FloatArray trajectory;
    roboat_core::FloatArray local_copies;
    roboat_core::FloatArray lambda_multipliers;
    geometry_msgs::Pose2D vel_ref;
    //nav_msgs::Path horizon_path;
    //geometry_msgs::PoseStamped horizon_pose;

    // ocpX Parameters
    MatrixXd xref;
    MatrixXd yref;
    MatrixXd multi_i;
    MatrixXd copy_i;
    MatrixXd multi_ji;
    MatrixXd copy_ji;
    MatrixXd X_0;

    // ocpZ Parameters
    MatrixXd z_x;
    MatrixXd z_y;
    MatrixXd copy_ij;
    //multi_i is used again
    MatrixXd traj_i;
    MatrixXd multi_ij;
    MatrixXd traj_ij;
    MatrixXd boat_diam;

    // CBF Parameters
    // Reusing MatrixXd boat_diam;
    MatrixXd x_i;
    MatrixXd y_i;
    MatrixXd u_mpc;
    MatrixXd v_mpc;
    MatrixXd C_j;
    MatrixXd V_j;

    // ocpX Outputs
    MatrixXd x_res;
    MatrixXd y_res;
    MatrixXd u_res;
    MatrixXd v_res;

    // ocpZ Outputs
    MatrixXd zx_res;
    MatrixXd zy_res;
    MatrixXd copy_ij_res;

    // CBF Outputs
    MatrixXd u_cbf;
    MatrixXd v_cbf;

    // ocpX vectors
    std::vector<const double*> arg_ocpX;
    std::vector<double*> res_ocpX;
    std::vector<casadi_int> iw_ocpX;
    std::vector<double> w_ocpX;
    int mem_ocpX;

    // ocpZ vectors
    std::vector<const double*> arg_ocpZ;
    std::vector<double*> res_ocpZ;
    std::vector<casadi_int> iw_ocpZ;
    std::vector<double> w_ocpZ;
    int mem_ocpZ;

    // CBF vectors
    std::vector<const double*> arg_ocpC;
    std::vector<double*> res_ocpC;
    std::vector<casadi_int> iw_ocpC;
    std::vector<double> w_ocpC;
    int mem_ocpC;

    std::vector<double> new_trajectory;
    std::vector<double> new_local_copies;
    std::vector<double> new_lambda_multipliers;
    
    MiniboatADMM()
    {
        //ROS Publishers and Subscribers
        trajectory_pub = nh.advertise<roboat_core::FloatArray>("trajectory", 1);
        local_copies_pub = nh.advertise<roboat_core::FloatArray>("local_copies", 1);
        lambda_multipliers_pub = nh.advertise<roboat_core::FloatArray>("lambda_multipliers", 1);
        vel_ref_pub = nh.advertise<geometry_msgs::Pose2D>("velocity_reference", 1);
        path_pub = nh.advertise<nav_msgs::Path>("horizon_path", 1);

        reference_pose_sub = nh.subscribe("assignment/reference_pose", 1, &MiniboatADMM::reference_callback, this);
        counter_reset_sub = nh.subscribe("/counter_restart", 1, &MiniboatADMM::reset_callback, this);
        latching_sub = nh.subscribe("latching", 1, &MiniboatADMM::latching_callback, this);

        swarm_.initialize(nh);
        swarm_size_ = swarm_.getBoatN();
        idx_ = swarm_.getIdx();
        if (idx_ == -1 || swarm_size_ <= 0)
        {
            ROS_ERROR("wrong swarm definition");
            idx_ = 0;
        }

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

        FUNCTIONS_DIR_DEFAULT = "/home/ubuntu/catkin_ws/src/miniboat/roboat_core/scripts/ipopt_definitions";
        nh.param("swarm/Nhor", Nhor, 20);
        nh.param("swarm/ocpX_states", ocpX_states, 2);
        nh.param("swarm/mu", mu, 1.0);
        nh.param("functions/path", FUNCTIONS_DIR, FUNCTIONS_DIR_DEFAULT);
        Nhor_plus_one = Nhor + 1;

        ocpX_function = casadi::Function::load(FUNCTIONS_DIR + "/ocpX.casadi");
        ocpZ_function = casadi::Function::load(FUNCTIONS_DIR + "/ocpZ.casadi");
        ocpC_function = casadi::Function::load(FUNCTIONS_DIR + "/ocpCBF.casadi");

        // initialize ocpX parameters
        xref = MatrixXd::Zero(1,1);
        yref = MatrixXd::Zero(1,1);
        multi_i = MatrixXd::Zero(ocpX_states, Nhor_plus_one);
        copy_i = MatrixXd::Zero(ocpX_states, Nhor_plus_one);
        multi_ji = MatrixXd::Zero(ocpX_states*(swarm_size_-1), Nhor_plus_one);
        copy_ji = MatrixXd::Zero(ocpX_states*(swarm_size_-1), Nhor_plus_one);
        X_0 = MatrixXd::Zero(ocpX_states, 1);

        // initialize ocpZ parameters
        z_x = MatrixXd::Zero(1,Nhor_plus_one);
        z_y = MatrixXd::Zero(1,Nhor_plus_one);
        copy_ij = MatrixXd::Zero(ocpX_states*(swarm_size_-1), Nhor_plus_one);
        // multi_i is already initialized in ocpZ
        traj_i = MatrixXd::Zero(ocpX_states, Nhor_plus_one);
        multi_ij = MatrixXd::Zero(ocpX_states*(swarm_size_-1), Nhor_plus_one);
        traj_ij = MatrixXd::Zero(ocpX_states*(swarm_size_-1), Nhor_plus_one);
        boat_diam = MatrixXd::Zero(1,1);
        boat_diam(0,0) = 0.3;

        // initialize CBF parameters
        // boat_diam is already initialized in ocpZ
        x_i = MatrixXd::Zero(1,1);
        y_i = MatrixXd::Zero(1,1);
        u_mpc = MatrixXd::Zero(1, 1);
        v_mpc = MatrixXd::Zero(1, 1);
        C_j = MatrixXd::Zero(ocpX_states*(swarm_size_-1), 1);
        V_j = MatrixXd::Zero(ocpX_states*(swarm_size_-1), 1);
        
        // initialize ocpX outputs
        x_res = MatrixXd::Zero(1, Nhor_plus_one);
        y_res = MatrixXd::Zero(1, Nhor_plus_one);
        u_res = MatrixXd::Zero(1, Nhor);
        v_res = MatrixXd::Zero(1, Nhor);

        // initialize ocpZ outputs
        zx_res = MatrixXd::Zero(1, Nhor_plus_one);
        zy_res = MatrixXd::Zero(1, Nhor_plus_one);
        copy_ij_res = MatrixXd::Zero(ocpX_states*(swarm_size_-1), Nhor_plus_one);

        // initialize CBF outputs
        u_cbf = MatrixXd::Zero(1, 1);
        v_cbf = MatrixXd::Zero(1, 1);

        // initialize ocpX vectors
        arg_ocpX = std::vector<const double*>(ocpX_function.sz_arg());
        res_ocpX = std::vector<double*>(ocpX_function.sz_res());
        iw_ocpX = std::vector<casadi_int>(ocpX_function.sz_iw());
        w_ocpX = std::vector<double>(ocpX_function.sz_w());
        mem_ocpX = ocpX_function.checkout();

        //initialize ocpZ vectors
        arg_ocpZ = std::vector<const double*>(ocpZ_function.sz_arg());
        res_ocpZ = std::vector<double*>(ocpZ_function.sz_res());
        iw_ocpZ = std::vector<casadi_int>(ocpZ_function.sz_iw());
        w_ocpZ = std::vector<double>(ocpZ_function.sz_w());
        mem_ocpZ = ocpZ_function.checkout();

        //initialize CBF vectors
        arg_ocpC = std::vector<const double*>(ocpC_function.sz_arg());
        res_ocpC = std::vector<double*>(ocpC_function.sz_res());
        iw_ocpC = std::vector<casadi_int>(ocpC_function.sz_iw());
        w_ocpC = std::vector<double>(ocpC_function.sz_w());
        mem_ocpC = ocpC_function.checkout();

        // initialize ocpX args
        arg_ocpX[0] = &xref(0,0);
        arg_ocpX[1] = &yref(0,0);
        arg_ocpX[2] = &multi_i(0,0);
        arg_ocpX[3] = &copy_i(0,0);
        arg_ocpX[4] = &multi_ji(0,0);
        arg_ocpX[5] = &copy_ji(0,0);
        arg_ocpX[6] = &X_0(0,0);

        // initialize ocpZ args
        arg_ocpZ[0] = &z_x(0,0);
        arg_ocpZ[1] = &z_y(0,0);
        arg_ocpZ[2] = &copy_ij(0,0);
        arg_ocpZ[3] = &multi_i(0,0);
        arg_ocpZ[4] = &traj_i(0,0);
        arg_ocpZ[5] = &multi_ij(0,0);
        arg_ocpZ[6] = &traj_ij(0,0);
        arg_ocpZ[7] = &boat_diam(0,0);

        // initialize ocpC args
        arg_ocpC[0] = &boat_diam(0,0);
        arg_ocpC[1] = &x_i(0,0);
        arg_ocpC[2] = &y_i(0,0);
        arg_ocpC[3] = &u_mpc(0,0);
        arg_ocpC[4] = &v_mpc(0,0);
        arg_ocpC[5] = &C_j(0,0);
        //arg_ocpC[6] = &V_j(0,0);

        // initialize ocpX results
        res_ocpX[0] = &x_res(0,0);
        res_ocpX[1] = &y_res(0,0);
        res_ocpX[2] = &u_res(0,0);
        res_ocpX[3] = &v_res(0,0);

        // initialize ocpZ results
        res_ocpZ[0] = &zx_res(0,0);
        res_ocpZ[1] = &zy_res(0,0);
        res_ocpZ[2] = &copy_ij_res(0,0);

        // initialize CBF results
        res_ocpC[0] = &u_cbf(0,0);
        res_ocpC[1] = &v_cbf(0,0);

        admm_initialized = false;
        counter = 0;
        counter_mpc = 0;
        counter_delatch = 10;
        desired_yaw = 0.0;
        ocpX_flag = true;
        ocpZ_flag = false;

        reference(0) = swarm_.state_[idx_][0];//idx_;
        reference(1) = swarm_.state_[idx_][1];//idx_;

        last_reference(0) = swarm_.state_[idx_][0];//0.0;
        last_reference(1) = swarm_.state_[idx_][1];//0.0;

        pose << 0.0, 0.0;
        max_allowed_vel = 0.04;
        k = 0.4;
        scale = 1.0;

    }

    void reference_callback(const geometry_msgs::Pose2D::ConstPtr& _ref)
    {
        reference(0) = _ref->x; //ref in x
        reference(1) = _ref->y; //ref in y
        boat_diam(0,0) = _ref->theta;
    }

    void reset_callback(const std_msgs::UInt16::ConstPtr& _counter)
    {
        counter = _counter->data;
        counter_mpc = _counter->data;
        last_reference(0) = swarm_.state_[idx_][0];
        last_reference(1) = swarm_.state_[idx_][1];
        //multi_i = MatrixXd::Zero(ocpX_states, Nhor_plus_one);
        //multi_ij = MatrixXd::Zero(ocpX_states*(swarm_size_-1), Nhor_plus_one);
        ROS_ERROR("Counter reset");
    }

    void latching_callback(const std_msgs::UInt16::ConstPtr& _latch)
    {
        if (_latch->data == 2){
            counter_delatch = 0;
            desired_yaw = ((idx_ % 2) - 0.5)*3.0;
            ROS_ERROR("delatch %f", desired_yaw);
        }
    }

    void time_step()
    {
        if (ocpX_flag){
            // update ocpX parameters (reference and initial state)
            xref(0,0) = reference(0);
            yref(0,0) = reference(1);
            X_0(0,0) = swarm_.state_[idx_][0];
            X_0(1,0) = swarm_.state_[idx_][1];
            psi = swarm_.state_[idx_][2];
            
            // solve ocpX
            ocpX_function(casadi::get_ptr(arg_ocpX), casadi::get_ptr(res_ocpX), casadi::get_ptr(iw_ocpX), casadi::get_ptr(w_ocpX), mem_ocpX);

            // unpackage solved trajectory for communication with agents (x_i, y_i) and ocpZ parameter update (initial z_x, z_y, trajectory x_i,y_i)
            new_trajectory.clear();
            for (int i = 0; i < Nhor_plus_one; i++)
            {
                new_trajectory.push_back(x_res(0,i));
                z_x(0,i) = x_res(0,i);
                traj_i(0,i) = x_res(0,i);
            } 
            for (int i = 0; i < Nhor_plus_one; i++)
            {
                new_trajectory.push_back(y_res(0,i));
                z_y(0,i) = y_res(0,i);
                traj_i(1,i) = y_res(0,i);
            }
            if (admm_initialized == false){
                new_trajectory.clear();
                for (int i = 0; i < Nhor_plus_one; i++)
                {
                    new_trajectory.push_back(swarm_.state_[idx_][0]);
                    z_x(0,i) = swarm_.state_[idx_][0];
                    traj_i(0,i) = swarm_.state_[idx_][0];
                } 
                for (int i = 0; i < Nhor_plus_one; i++)
                {
                    new_trajectory.push_back(swarm_.state_[idx_][1]);
                    z_y(0,i) = swarm_.state_[idx_][1];
                    traj_i(1,i) = swarm_.state_[idx_][1];
                }
            }
            trajectory.data = new_trajectory;
            trajectory_pub.publish(trajectory);

            nav_msgs::Path horizon_path;
            geometry_msgs::PoseStamped horizon_pose;
            for (int i = 0; i < Nhor_plus_one; i++){
                horizon_pose.header.stamp = ros::Time::now();
                horizon_pose.header.frame_id = "world";
                horizon_pose.pose.position.x = traj_i(0,i);
                horizon_pose.pose.position.y = -traj_i(1,i);
                horizon_pose.pose.position.z = 0.0;

                horizon_path.header.stamp = ros::Time::now();
                horizon_path.header.frame_id = "world";
                horizon_path.poses.push_back(horizon_pose);
            }
            path_pub.publish(horizon_path);

        }        

        if (ocpZ_flag){

            // receive trajectories of other agents for ocpZ parameter (x_ij, y_ij)
            for (int k = 0; k < swarm_size_; k++) {
                if (k == idx_) {
                    continue;
                }
                if (k < idx_) {
                    for (int l = 0; l < Nhor_plus_one; l++) {
                    traj_ij(2*k,l)     = swarm_.trajectory_[k][l];
                    traj_ij((2*k)+1,l) = swarm_.trajectory_[k][l+Nhor_plus_one];
                    copy_ij(2*k,l)     = swarm_.trajectory_[k][l];
                    copy_ij((2*k)+1,l) = swarm_.trajectory_[k][l+Nhor_plus_one];
                    }
                }
                if (k > idx_) {
                    for (int l = 0; l < Nhor_plus_one; l++) {
                    traj_ij(2*(k-1),l)   = swarm_.trajectory_[k][l];
                    traj_ij(2*(k-1)+1,l) = swarm_.trajectory_[k][l+Nhor_plus_one];
                    copy_ij(2*(k-1),l)   = swarm_.trajectory_[k][l];
                    copy_ij(2*(k-1)+1,l) = swarm_.trajectory_[k][l+Nhor_plus_one];
                    }
                }
            }

            if (admm_initialized == false){
                for (int k = 0; k < swarm_size_; k++) {
                    if (k == idx_) {
                        continue;
                    }
                    if (k < idx_) {
                        for (int l = 0; l < Nhor_plus_one; l++) {
                        traj_ij(2*k,l)     = swarm_.state_[k][0];
                        traj_ij((2*k)+1,l) = swarm_.state_[k][1];
                        copy_ij(2*k,l)     = swarm_.state_[k][0];
                        copy_ij((2*k)+1,l) = swarm_.state_[k][1];
                        }
                    }
                    if (k > idx_) {
                        for (int l = 0; l < Nhor_plus_one; l++) {
                        traj_ij(2*(k-1),l)   = swarm_.state_[k][0];
                        traj_ij(2*(k-1)+1,l) = swarm_.state_[k][1];
                        copy_ij(2*(k-1),l)   = swarm_.state_[k][0];
                        copy_ij(2*(k-1)+1,l) = swarm_.state_[k][1];
                        }
                    }
                }
            }

            // solve ocpZ
            ocpZ_function(casadi::get_ptr(arg_ocpZ), casadi::get_ptr(res_ocpZ), casadi::get_ptr(iw_ocpZ), casadi::get_ptr(w_ocpZ), mem_ocpZ);

            // unpackage new trajectory estimates/local copies (Z_ij) for communication with other agents
            new_local_copies.clear();
            for (int k = 0; k < ocpX_states*(swarm_size_-1); k++){
                for (int l = 0; l < Nhor_plus_one; l++) {
                        new_local_copies.push_back(copy_ij_res(k,l));
                }
            } 
            if (admm_initialized == false){
                new_local_copies.clear();
                for (int i = 0; i < ((swarm_size_-1)*(Nhor_plus_one)); i++)
                {
                    new_local_copies.push_back(idx_);
                }
            }
            local_copies.data = new_local_copies;
            local_copies_pub.publish(local_copies);

            // update lambda multipliers i and ij
            new_lambda_multipliers.clear();        
            for (int i = 0; i < Nhor_plus_one; i++){
                multi_i(0,i) = multi_i(0,i) + mu*(zx_res(0,i)-x_res(0,i)); 
                multi_i(1,i) = multi_i(1,i) + mu*(zy_res(0,i)-y_res(0,i)); 
            }
            for (int k = 0; k < ocpX_states*(swarm_size_-1); k++){
                for (int l = 0; l < Nhor_plus_one; l++) {
                    multi_ij(k,l) = multi_ij(k,l) + mu*(copy_ij_res(k,l) - traj_ij(k,l));
                }
            }
            if (admm_initialized == false){
                for (int i = 0; i < Nhor_plus_one; i++){
                    multi_i(0,i) = 0.0; 
                    multi_i(1,i) = 0.0; 
                }
                for (int k = 0; k < ocpX_states*(swarm_size_-1); k++){
                    for (int l = 0; l < Nhor_plus_one; l++) {
                        multi_ij(k,l) = 0.0;
                    }
                }
                admm_initialized = true;
            }
            for (int k = 0; k < ocpX_states*(swarm_size_-1); k++){
                for (int l = 0; l < Nhor_plus_one; l++) {
                    new_lambda_multipliers.push_back(multi_ij(k,l));
                }
            }
            lambda_multipliers.data = new_lambda_multipliers;
            lambda_multipliers_pub.publish(lambda_multipliers);
        }

        // update new trajectory estimate/local copy (Z_i) for ocpX
        for (int i = 0; i < Nhor_plus_one; i++)
        {
            copy_i(0,i) = zx_res(0,i);
        } 
        for (int i = 0; i < Nhor_plus_one; i++)
        {
            copy_i(1,i) = zy_res(0,i);
        }

        // update other agents' guesses of current agent trajectory (Z_ji) ocpX parameter
        for (int k = 0; k < swarm_size_; k++) {
            if (k == idx_) {
                continue;
            }
            if (k < idx_) {
                for (int l = 0; l < Nhor_plus_one; l++) {
                copy_ji(2*k,l)     = swarm_.copy_[k][2*(idx_-1)*Nhor_plus_one+l];
                copy_ji((2*k)+1,l) = swarm_.copy_[k][2*(idx_-1)*Nhor_plus_one+l+Nhor_plus_one];
                }
            }
            if (k > idx_) {
                for (int l = 0; l < Nhor_plus_one; l++) {
                copy_ji(2*(k-1),l)   = swarm_.copy_[k][2*(idx_)*Nhor_plus_one+l];
                copy_ji(2*(k-1)+1,l) = swarm_.copy_[k][2*(idx_)*Nhor_plus_one+l+Nhor_plus_one];
                }
            }
        }

        // update other agents' multipliers (lambda_ji) ocpX parameter
        for (int k = 0; k < swarm_size_; k++) {
            if (k == idx_) {
                continue;
            }
            if (k < idx_) {
                for (int l = 0; l < Nhor_plus_one; l++) {
                multi_ji(2*k,l)     = swarm_.multiplier_[k][2*(idx_-1)*Nhor_plus_one+l];
                multi_ji((2*k)+1,l) = swarm_.multiplier_[k][2*(idx_-1)*Nhor_plus_one+l+Nhor_plus_one];
                }
            }
            if (k > idx_) {
                for (int l = 0; l < Nhor_plus_one; l++) {
                multi_ji(2*(k-1),l)   = swarm_.multiplier_[k][2*(idx_)*Nhor_plus_one+l];
                multi_ji(2*(k-1)+1,l) = swarm_.multiplier_[k][2*(idx_)*Nhor_plus_one+l+Nhor_plus_one];
                }
            }
        }

        if (ocpX_flag){
            ocpX_flag = false;
            ocpZ_flag = true;
        }
        else{
            ocpX_flag = true;
            ocpZ_flag = false;
        }

        if (counter_delatch >= 40){
            desired_yaw = 0.0;
        }

        if (counter > 400){
            if (counter_mpc > 1){
                x_i(0,0) = swarm_.state_[idx_][0];
                y_i(0,0) = swarm_.state_[idx_][1];
                u_mpc(0,0) = u_res(0,0);
                v_mpc(0,0) = v_res(0,0);
                for (int k = 0; k < swarm_size_; k++) {
                    if (k == idx_) {
                        continue;
                    }
                    if (k < idx_) {
                        C_j(2*k,0)     = swarm_.state_[k][0];
                        C_j((2*k)+1,0) = swarm_.state_[k][1];
                        V_j(2*k,0)     = swarm_.state_[k][3];
                        V_j((2*k)+1,0) = swarm_.state_[k][4];
                        }
                    if (k > idx_) {
                        C_j(2*(k-1),0)   = swarm_.state_[k][0];
                        C_j(2*(k-1)+1,0) = swarm_.state_[k][1];
                        V_j(2*(k-1),0)   = swarm_.state_[k][3];
                        V_j(2*(k-1)+1,0) = swarm_.state_[k][4];
                        }
                }
                ocpC_function(casadi::get_ptr(arg_ocpC), casadi::get_ptr(res_ocpC), casadi::get_ptr(iw_ocpC), casadi::get_ptr(w_ocpC), mem_ocpC);
                
                global_vel << u_cbf(0,0), v_cbf(0,0);
                //global_vel << u_res(0,0), v_res(0,0); //if cbf wants to be avoided
                rotation << cos(psi), -sin(psi),
                            sin(psi), cos(psi);
                body_vel = rotation.transpose() * global_vel;
                max_vel = std::max(abs(body_vel(0)),abs(body_vel(1)));    
                if (max_vel > max_allowed_vel)
                {
                    body_vel(0) = body_vel(0)*max_allowed_vel/max_vel;
                    body_vel(1) = body_vel(1)*max_allowed_vel/max_vel;
                }
                vel_ref.x = body_vel(0);
                vel_ref.y = body_vel(1);
                vel_ref.theta = desired_yaw;
                vel_ref_pub.publish(vel_ref);
                counter_mpc = 0;
            }
            counter_mpc += 1;
        }
        else{
            if (counter_mpc > 1){
                /*vel_ref.x = 0.0;
                vel_ref.y = 0.0;
                vel_ref.theta = 0.0;*/
                pose(0) = swarm_.state_[idx_][0];
                pose(1) = swarm_.state_[idx_][1];
                distance = pow((pow(pose(0)-last_reference(0),2) + pow(pose(1)-last_reference(1),2)),0.5);
                global_vel = k * exp(distance / scale) * (last_reference - pose);
                rotation << cos(psi), -sin(psi),
                            sin(psi), cos(psi);
                body_vel = rotation.transpose() * global_vel;
                max_vel = std::max(abs(body_vel(0)),abs(body_vel(1)));    
                if (max_vel > max_allowed_vel)
                {
                    body_vel(0) = body_vel(0)*max_allowed_vel/max_vel;
                    body_vel(1) = body_vel(1)*max_allowed_vel/max_vel;
                }
                vel_ref.x = body_vel(0);
                vel_ref.y = body_vel(1);
                vel_ref.theta = desired_yaw;
                vel_ref_pub.publish(vel_ref);
                counter_mpc = 0;
            }
            counter_mpc += 1;
        }
        counter += 1;
        counter_delatch += 1;

        if (counter == 400){
            ROS_ERROR("ADMM initialized");
            ROS_ERROR("miniboat %d, x_ref %f, y_ref %f", idx_+1, xref(0,0), yref(0,0));
        }

    }

private:
    ros::NodeHandle nh;
    ros::Publisher trajectory_pub;
    ros::Publisher local_copies_pub;
    ros::Publisher lambda_multipliers_pub;
    ros::Publisher vel_ref_pub;
    ros::Publisher path_pub;
    ros::Subscriber reference_pose_sub;
    ros::Subscriber counter_reset_sub;
    ros::Subscriber latching_sub;

};

//Main
int main(int argc, char *argv[])
{
    ros::init(argc, argv, "miniboat_admm");
    MiniboatADMM miniboatADMM;
    int rate = 10;
    ros::Rate loop_rate(rate);
    ros::Duration(2).sleep();

    while (ros::ok())
    {
        miniboatADMM.time_step();
        ros::spinOnce();
        loop_rate.sleep();
    }
    miniboatADMM.ocpX_function.release(miniboatADMM.mem_ocpX);
    miniboatADMM.ocpZ_function.release(miniboatADMM.mem_ocpZ);
    miniboatADMM.ocpC_function.release(miniboatADMM.mem_ocpC);

    return 0;
}
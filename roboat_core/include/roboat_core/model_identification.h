// Copyright 2022 MIT, all rights reserved
#ifndef MODEL_IDENTIFICATION_HPP
#define MODEL_IDENTIFICATION_HPP

#include<iostream>
#include<fstream>
#include<sstream>
#include<string>

#include "ros/ros.h"
#include "ros/package.h"
#include <nav_msgs/Odometry.h>
#include <roboat_core/Force.h>

#include <roboat_core/miscfun.h>
#include <tf/transform_datatypes.h>

enum CALIBRATION_STEPS
{
  CAL_FORWARD,
  CAL_BACKWARD,
  CAL_RIGHT,
  CAL_LEFT,
  NUMBER_OF_CALIBRATION_STEPS
};

enum MOTORS //motors controlled by ESC
{
  MOT_FRONT_RIGHT,
  MOT_FRONT_LEFT,
  MOT_BACK_RIGHT,
  MOT_BACK_LEFT,
  NUMBER_OF_MOTORS
};


class ModelIdentification
{
  private:
    ros::NodeHandle nh_;

    //Parameters (read from config file)
    double xmax_, ymax_, xmin_, ymin_; //bounding box
    double n_points_; //number of points for each element
    double ramp_time_, ref_time_, ref_force_, ref_vel_; //reference
    double pos_threshold_, dir_threshold_, vel_threshold_; //paramters used to determine motion/station
    double yaw_cmd_; //parameter to adjust yaw angle when going out of bounds
    double yaw_threshold_; //parameter to determine valid heading angle
    
    bool calibrating_, rotating_cw_, rotating_ccw_, is_ramp_;
    double t0_, t_start_, x_start_, y_start_, yaw_start_, t_, x_, y_, yaw_, vx_, vy_, vyaw_; 
    //time series of x,y,theta to determine the deviation
    // DataTS x_ts_, y_ts_, th_ts_;

    std::string file_name_;
    Eigen::VectorXf forces_;
    Eigen::MatrixXf friction_coeffs_;

    CALIBRATION_STEPS step_;
    int i_point_;
    bool first_val_;
    std::vector<int> i_mot_;
    double yaw_diff_prev_, fric_prev_, reverse_sign_;
    double x_bound_mid_[4], y_bound_mid_[4];
    
    void loadCoeffs();
    void saveCoeffs(); 
    
    bool isOutBounds(); //returns true if outside boundaries
    double yawDiff(double yaw_a, double yaw_b); //positive clockwise
    std::vector<int> chooseSide(std::vector<int> side_a, std::vector<int> side_b, double yaw_diff);
    
    void odomCallback(const nav_msgs::Odometry msg);
    void adjustCoeff(double yaw_diff);
    

  public:
    ModelIdentification();
    void initialize(ros::NodeHandle &nh);
    void startCalibration();
    void stopCalibration();
    bool isCalibrated(); 
    void getFriction(double *friction, double *thrust);
    void getForce(double *thrust);
    
};

#endif //MODEL_IDENTIFICATION_HPP

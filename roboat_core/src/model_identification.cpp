// Copyright 2022 MIT, all rights reserved
#include <roboat_core/model_identification.h>

ModelIdentification::ModelIdentification () {
  /* Class loaded within the Command class, which is the one that spins the node */
  return;
}

void ModelIdentification::initialize(ros::NodeHandle &nh)
{
  nh_ = nh;
  ros::Subscriber state_sub = nh.subscribe("odometry/filtered", 1, &ModelIdentification::odomCallback, this); 

  std::vector<double> bounds;
  nh_.getParam("model_identification/bounds", bounds);
  nh_.getParam("model_identification/n_steps", n_points_);

  nh_.getParam("model_identification/ref_len", ref_time_);
  nh_.getParam("model_identification/ref_force", ref_force_);
  nh_.getParam("model_identification/ref_vel", ref_vel_);
  
  nh_.getParam("model_identification/pos_threshold", pos_threshold_);
  nh_.getParam("model_identification/dir_threshold", dir_threshold_);
  nh_.getParam("model_identification/vel_threshold", vel_threshold_);

  nh_.getParam("model_identification/yaw_cmd", yaw_cmd_);
  nh_.getParam("model_identification/yaw_threshold", yaw_threshold_);

  //boundary related variables
  xmin_ = bounds[0]; ymin_ = bounds[1];
  xmax_ = bounds[2]; ymax_ = bounds[3];
  x_bound_mid_[0]=0.5*(xmin_+xmax_); y_bound_mid_[0]=ymin_; 
  x_bound_mid_[1]=xmax_; y_bound_mid_[1]=0.5*(ymin_+ymax_);
  x_bound_mid_[2]=x_bound_mid_[0]; y_bound_mid_[2]=ymax_;
  x_bound_mid_[3]=xmin_; y_bound_mid_[3]=y_bound_mid_[1]; 

  //calibration forces
  for (int i = 0; i< n_points_; i++) {
    forces_(i) = (i+1)/n_points_ * ref_force_;
  }

  //load existing calibration file (if it exists, otherwise, friction forces always set to zero)
  file_name_ = ros::package::getPath("roboat_core") + "/config/friction.txt";
  try {
    loadCoeffs();
  }
  catch (...) {
    ROS_INFO("missing or wrong formatted friction file, setting coefficients to zero");
    friction_coeffs_.Zero(n_points_,MOTORS::NUMBER_OF_MOTORS);
  }

  calibrating_=false;
  rotating_cw_=false;
  rotating_ccw_=false;
  is_ramp_=false;
}

void ModelIdentification::startCalibration() 
{
  //triggers calibration
  calibrating_ = true;
  step_ = CALIBRATION_STEPS::CAL_FORWARD;
  i_point_ = 0; 
  is_ramp_=true;
  t0_=t_;
  return;
}

void ModelIdentification::stopCalibration() {
  calibrating_ = false;
  return;
}

bool ModelIdentification::isCalibrated()
{
  return !calibrating_;
}

void ModelIdentification::getFriction(double *friction, double *thrust) {
  for (int i=0; i<(int)MOTORS::NUMBER_OF_MOTORS; i++) {
    if (thrust[i]==0.0) {
      friction[i]=0.0;
    }
    else {
      //interpolates
      friction[i] = interp_lin(forces_,friction_coeffs_,i,thrust[i]);
    }
  }
  return;
}

void ModelIdentification::getForce(double *thrust) 
{
  if (rotating_cw_) {
    thrust[MOTORS::MOT_FRONT_LEFT] = yaw_cmd_;
    thrust[MOTORS::MOT_FRONT_RIGHT] = 0.0;
    thrust[MOTORS::MOT_BACK_LEFT] = yaw_cmd_;
    thrust[MOTORS::MOT_BACK_RIGHT] = 0.0;
  }
  else if (rotating_ccw_) {
    thrust[MOTORS::MOT_FRONT_LEFT] = 0.0;
    thrust[MOTORS::MOT_FRONT_RIGHT] = yaw_cmd_;
    thrust[MOTORS::MOT_BACK_LEFT] = 0.0;
    thrust[MOTORS::MOT_BACK_RIGHT] = yaw_cmd_;
  }
  else {
    //used during calibration, provides force combined with calibration
    std::vector<int> idx;
    switch (step_) {
      case CALIBRATION_STEPS::CAL_FORWARD : idx={0,1}; break;
      case CALIBRATION_STEPS::CAL_BACKWARD : idx={2,3};  break;
      case CALIBRATION_STEPS::CAL_RIGHT : idx={1,3};  break;
      case CALIBRATION_STEPS::CAL_LEFT : idx={0,2}; break;
    }
    for (int i=0; i<MOTORS::NUMBER_OF_MOTORS; i++) {
      thrust[i]=0.0;
    }
    for (int i=0; i<idx.size(); i++) {
      thrust[idx[i]]=forces_(i_point_) + friction_coeffs_(idx[i]);
    }
  }
  return;
}


void ModelIdentification::loadCoeffs() {
  std::ifstream f(file_name_); //taking file as inputstream
   
  if (f) {
    //loads file as input stream
    std::vector<double> dat(std::istream_iterator<double>(f), {});
    int n=dat.size() / (MOTORS::NUMBER_OF_MOTORS + 1);
    Eigen::VectorXf lforces(n);
    friction_coeffs_.resize(n,MOTORS::NUMBER_OF_MOTORS);
    for (int i=0; i<n; i++) {
      lforces(i)=dat[i*MOTORS::NUMBER_OF_MOTORS+1];
      for (int j=0; j<MOTORS::NUMBER_OF_MOTORS; j++) {
        friction_coeffs_(i,j)=dat[i*MOTORS::NUMBER_OF_MOTORS+1+j];
      }
    }
    f.close();
    //adjust the data values to the reference force mapping specified if not matching
    if (n!=n_points_ || lforces(n-1)!=ref_force_) {
      Eigen::MatrixXf lfriction=friction_coeffs_;
      for (int i=0; i<n_points_; i++) {
        for (int j=0; j<MOTORS::NUMBER_OF_MOTORS; j++) {
          friction_coeffs_(i,j)=interp_lin(lforces,lfriction,j,forces_(i));
        }
      }
      saveCoeffs();
    }
  }
  else {
    //sets to zero if file does not exist
    ROS_INFO("friction calibration not available, setting friction coefficients to zero");
    friction_coeffs_.Zero(n_points_,MOTORS::NUMBER_OF_MOTORS); 
  }
  return;
}

void ModelIdentification::saveCoeffs() {
  std::ofstream f(file_name_); //taking file as onputstream
  for (int i=0; i<forces_.rows(); i++) {
    f << forces_(i);
    for (int j=0; j<MOTORS::NUMBER_OF_MOTORS; j++) {
      f << friction_coeffs_(i,j);
    }
  }
  f.close();
  return;
}

bool ModelIdentification::isOutBounds() {
  return (x_ > xmax_ || x_<xmin_ || y_>ymax_ || y_<ymin_);
}

double ModelIdentification::yawDiff(double yaw_a, double yaw_b) {
  //returns the rotation angle from yaw_a to yaw_b, positive if clockwise
  double yaw_diff = yaw_b - yaw_a;
  bool is_positive = yaw_diff > 0.0;
  if (is_positive) {
    yaw_diff = -yaw_diff;
  }
  if (yaw_diff > 2.0*M_PI-yaw_diff) {
    is_positive = !is_positive;
    yaw_diff = 2.0*M_PI-yaw_diff;
  }
  return  is_positive ? yaw_diff : -yaw_diff;
}

std::vector<int> ModelIdentification::chooseSide(std::vector<int> side_a, std::vector<int> side_b, double yaw_diff) 
{
  double fric_a=0.0, fric_b=0.0;
  for (int i=0; i<side_a.size(); i++) {
    fric_a += friction_coeffs_(side_a[i]);
  }
  for (int i=0; i<side_b.size(); i++) {
    fric_b += friction_coeffs_(side_b[i]);
  }
  fric_a = fric_a/side_a.size();
  fric_b = fric_b/side_b.size();
 
  if (fric_a == fric_b) {
    reverse_sign_ = (yaw_diff>0);
  }
  else {
    reverse_sign_ = (fric_b>fric_a);
  }
  return (reverse_sign_) ? side_b : side_a;
}

void ModelIdentification::odomCallback(const nav_msgs::Odometry msg) {
  if (!calibrating_) {
    return;
  }

  // convert from map to roboat map reference (negative y and yaw)
  t_ = msg.header.stamp.toSec();
  x_ = msg.pose.pose.position.x;
  y_ = -msg.pose.pose.position.y; //
  yaw_ = -tf::getYaw(tf::Quaternion(msg.pose.pose.orientation.x, msg.pose.pose.orientation.y,
                                  msg.pose.pose.orientation.z, msg.pose.pose.orientation.w));
  vx_ = msg.twist.twist.linear.x;
  vy_ = -msg.twist.twist.linear.y;
  vyaw_ = -msg.twist.twist.angular.z;

  // if beyond bounds, pivots to point towards the furthest boundary midpoint
  if (isOutBounds()) {
    // determines the index of the boundary mid point furthest from the boat
    double dist=0.0, dd;
    int i_bound;
    for (int i=0; i<4; i++) {
      dd = sqrt(pow(x_bound_mid_[i]-x_,2.0) + pow(y_bound_mid_[i]-y_,2.0));
      if (dd>dist) {
        i_bound = i;
        dist = dd;
      }
    }
    //computes the angle towards this point
    double yaw_diff = yawDiff(yaw_, atan2(x_bound_mid_[i_bound]-x_, y_bound_mid_[i_bound]-y_));
    if (yaw_diff>0) {
      rotating_cw_ = yaw_diff > yaw_threshold_;
      rotating_ccw_ = false;
    }
    else {
      rotating_cw_ = 0.0;
      rotating_ccw_ = -yaw_diff > yaw_threshold_;
    }
  }

  if (rotating_cw_ || rotating_ccw_) {
    is_ramp_ = true;
    t0_ = t_;
  }
  else {
    //updates calibrating status
    if (is_ramp_ && t_-t0_>ramp_time_) {
      //ramping time finished
      t_start_ = t_; 
      is_ramp_ = false;
    }
    else if (t_-t_start_>ref_time_) {
      //steady movement time finished, check if valid point
      double yaw_diff = yawDiff(yaw_start_,atan2(x_-x_start_, y_-y_start_));
      if (std::abs(yaw_diff) > dir_threshold_) {
        if (step_ < CALIBRATION_STEPS::CAL_LEFT) {
          //adjusts coefficients
          adjustCoeff(yaw_diff);
        }
        else {
          /* at this point it should be calibrated and checking it in 
          CALIBRATION_STEPS::CAL_RIGHT, should not enter here */
          ROS_WARN("calibration failed [point %d]", i_point_);
          i_point_++;
        }
      }
      else {
        //valid point, moves to the next one
        first_val_ = true;
        i_point_++;
        if (i_point_==n_points_) {
          //moves to the next step
          i_point_=0;
          step_ = (CALIBRATION_STEPS)((int)step_+1);
        }
        if (step_ == CALIBRATION_STEPS::NUMBER_OF_CALIBRATION_STEPS) {
          //reached the end
          saveCoeffs();
          calibrating_ = false;
        }
      }
      //re-start the counter
      is_ramp_ = true;
      t0_ = t_;
    }
  }
  return;
}

void ModelIdentification::adjustCoeff(double yaw_diff) {
  if (first_val_) { 
    //determines the motors that will be adjusted at this time
    std::vector<int> side_a, side_b;
    switch(step_) {
      case CALIBRATION_STEPS::CAL_FORWARD : side_a={0}; side_b={1}; break;
      case CALIBRATION_STEPS::CAL_BACKWARD : side_a={3}; side_b={2};  break;
      case CALIBRATION_STEPS::CAL_RIGHT : side_a={1,0}; side_b={3,2};  break;
      case CALIBRATION_STEPS::CAL_LEFT : side_a={2,3}; side_b={0,1}; break;
    }
    //chooses the side that will be adjusted
    i_mot_ = chooseSide(side_a,side_b,yaw_diff);

    //since it is the first point, it stores the current difference as the previous one
    yaw_diff_prev_ = yaw_diff;
    fric_prev_ = friction_coeffs_(i_mot_[0]);
  }

  //Computes the increment in friction coefficient to be applied
  double delta_fric=0.0;
  if (fric_prev_== friction_coeffs_(i_mot_[0])) {
    //first point, just increases it a fraction of the coefficient 
    delta_fric = (fric_prev_==0.0) ? 0.05 * (i_point_+1)/n_points_ * ref_force_ : 0.05 * std::abs(fric_prev_);
    if (reverse_sign_) { delta_fric = -delta_fric; }
  }
  else {
    //iterative scheme (gets the sign from the derivative)
    delta_fric = (yaw_diff*fric_prev_ - yaw_diff_prev_*friction_coeffs_(i_mot_[0]))/(yaw_diff - yaw_diff_prev_);
  }

  //assigns friction values, and stores them as prev for next step
  yaw_diff_prev_ = yaw_diff;
  fric_prev_ = friction_coeffs_(i_mot_[0]);
  friction_coeffs_(i_mot_[0]) += delta_fric;
  return;
}

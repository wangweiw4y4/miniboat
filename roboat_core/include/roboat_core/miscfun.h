// Copyright 2022 MIT, all rights reserved
#ifndef ROBOAT_UTILS_MISCFUN_H
#define ROBOAT_UTILS_MISCFUN_H

#include <ctype.h>
#include <ros/ros.h>
#include <stdint.h>

#include <Eigen/Dense>
#include <string>
#include <vector>

double interp_lin(Eigen::VectorXf &xData, Eigen::MatrixXf &yData, int col, double x, bool extrapolate = true);

class DataTS {
private:
  int ntime_ts_,itime_first_,itime_last_;
  double time_span_ts_;
  double min_covariance_;
  std::vector<double> time_ts_, dat_ts_;
  
public:
  DataTS(double time_span);
  void addPoint(double time, double val);
  double getMean();
  double getCovariance();
};

#endif  // ROBOAT_UTILS_MISCFUN_H

// Copyright 2022 MIT, all rights reserved
#include <roboat_core/miscfun.h>
#include <boost/filesystem.hpp>
#include <vector>
#include <ros/package.h>
#include <ros/param.h>

/* LINEAR INTERPOLATION
Returns interpolated value at x from parallel arrays ( xData, yData )
Assumes that xData has at least two elements, is sorted and is strictly monotonic increasing
boolean argument extrapolate determines behaviour beyond ends of array (if needed) */
double interp_lin(Eigen::VectorXf &xData, Eigen::MatrixXf &yData, int col, double x, bool extrapolate)
{
  int size = xData.size();

  int i = 0;                 // find left end of interval for interpolation
  if (x >= xData(size - 2))  // special case: beyond right end
  {
    i = size - 2;
  } else
  {
    while (x > xData(i + 1))
      i++;
  }
  double xL = xData(i), yL = yData(i,col), xR = xData(i + 1), yR = yData(i + 1,col);  // points on either side (unless beyond ends)
  if (!extrapolate)          // if beyond ends of array and not extrapolating
  {
    if (x < xL)
      yR = yL;
    if (x > xR)
      yL = yR;
  }

  double dydx = (yR - yL) / (xR - xL);  // gradient

  return yL + dydx * (x - xL);  // linear interpolation
}


// ////////////////////////////////////////////////////////////////////////////
// DATA TIME SERIES CLASS
// ////////////////////////////////////////////////////////////////////////////
DataTS::DataTS(double time_span) : time_span_ts_(time_span) {
  //time-series for filtering
  ntime_ts_ = 0;
  itime_first_ = -1;
  itime_last_ = -1;
  //time-series needs to be initalized with >0 size, to avoid conflicts with indexing and avoid excesive resizing 
  dat_ts_.assign(100,0.0);
  time_ts_.assign(100,0.0);
  //min covariance, so it ensures is non-zero when running
  min_covariance_=1e-6;
}

void DataTS::addPoint(double time, double val)
{
  //adds a new entry to the boat position time-series
  bool start_ts = (itime_last_ == -1);
  itime_last_++;
  
  //takes care of re-start or end-of-array situation
  if (start_ts)
  {
    ntime_ts_=0;
    itime_first_=0; //we are starting the sequence, so initializes also the first counter
  }
  else if (itime_last_==(int)dat_ts_.size())
  {
    itime_last_=0;
  }

  //stores new points 
  if (itime_first_!=itime_last_ || start_ts) //replaces old unused content
  {
    time_ts_[itime_last_] = time;
    dat_ts_[itime_last_] = val;
  }
  else //we have run out of space, expand the storage space. Quite innefficient, but should enter here only a few times
  {
    time_ts_.insert(time_ts_.begin()+itime_last_, time);
    dat_ts_.insert(dat_ts_.begin()+itime_last_, val);
    itime_first_++;
  }
  //increases number of stored points counter
  ntime_ts_++;

  //removes the old points
  while(time - time_ts_[itime_first_] > time_span_ts_)
  {
    itime_first_++;
    ntime_ts_--;
    if (itime_first_==(int)dat_ts_.size())
    {
      itime_first_=0; //re-starts and eventually will get to itime_last_
    }
  }
  return;
}

double DataTS::getMean() {
  double mean=0.0;
  int ktime_prev, ktime = itime_first_;
  for (int i=0; i<ntime_ts_-1; i++)
  {
    ktime_prev = ktime;
    ktime++;
    if (ktime == (int)dat_ts_.size()) {
      ktime = 0;
    }
    mean += 0.5*(dat_ts_[ktime_prev]+dat_ts_[ktime]) *(time_ts_[ktime] - time_ts_[ktime_prev]);
  }
  mean = mean/(time_ts_[itime_last_]-time_ts_[itime_first_]);
  return mean;
}

double DataTS::getCovariance() {
  double mean = getMean();
  double covariance=0.0;
  int ktime_prev, ktime = itime_first_;
  for (int i=0; i<ntime_ts_-1; i++)
  {
    ktime_prev = ktime;
    ktime++;
    if (ktime == (int)dat_ts_.size()) {
      ktime = 0;
    }
    covariance += pow(0.5*(dat_ts_[ktime_prev]+dat_ts_[ktime])-mean, 2.0) *(time_ts_[ktime] - time_ts_[ktime_prev]);
  }
  covariance = std::max(covariance/(time_ts_[itime_last_]-time_ts_[itime_first_]), min_covariance_);
  return covariance;
}

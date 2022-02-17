// ROS
#include "ros/ros.h"
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <geometry_msgs/TwistWithCovarianceStamped.h>
#include <nav_msgs/Path.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Float64MultiArray.h>
#include <tf/transform_datatypes.h>
#include <sensor_msgs/Imu.h>
// ACADO
#include "acado_common.h"
#include "acado_auxiliary_functions.h"

// Roboat
#include <roboat_core/Force.h>
#include <roboat_core/MPC.hpp>
#include <cmath>

/* Some convenient definitions. */
#define NX ACADO_NX   /* Number of differential state variables.  */
#define NXA ACADO_NXA /* Number of algebraic variables. */
#define NU ACADO_NU   /* Number of control forces. */
#define NOD ACADO_NOD /* Number of online data values. */

#define NY ACADO_NY   /* Number of measurements/references on nodes 0..N - 1. */
#define NYN ACADO_NYN /* Number of measurements/references on node N. */

#define N ACADO_N   /* Number of intervals in the horizon. */
#define NUM_STEPS 1 /* Number of real-time iterations */
#define VERBOSE 1   /* Show iterations: 1, silent: 0.  */

std::vector<double> state(6);


void MPC::stateCallback(const nav_msgs::Odometry msg) {
    
    /*If state is advertised as an array, data can just be copied */
    // for(int i=0; i<6; i++) state[i] = array.data[i];
    // pose_received = true;
    
    /*If state is advertised as odometry, easier to visualize in rviz*/
    double yaw;
    pose_received = true;

    // convert from map to roboat map reference (negative y and yaw)
    state[0] = msg.pose.pose.position.x;
    state[1] = -msg.pose.pose.position.y; //
    yaw = -tf::getYaw(tf::Quaternion(msg.pose.pose.orientation.x, msg.pose.pose.orientation.y,
                                    msg.pose.pose.orientation.z, msg.pose.pose.orientation.w));

    // Unwrap the angle when roll over happens
    double yaw_diff = state[2] - yaw - state_roll_over_count * 2 * M_PI;
    if (yaw_diff > M_PI)
    {
      state_roll_over_count += 1;
    }
    else if (yaw_diff < -M_PI)
    {
      state_roll_over_count -= 1;
    }
    state[2] = yaw + state_roll_over_count * 2 * M_PI;

    // convert from map to roboat map reference (negative y and yaw)
    state[3] = msg.twist.twist.linear.x;
    state[4] = -msg.twist.twist.linear.y;
    state[5] = -msg.twist.twist.angular.z;
    
}

// void MPC::poseCallback(const geometry_msgs::PoseWithCovarianceStamped msg)
// {
//   double roll, pitch;
//   tf::Matrix3x3 m;
//   if( true /**(abs(state[0]-msg.pose.pose.position.x) > 2.) &&
//       (abs(state[1]-msg.pose.pose.position.y) > 2.)**/ ) {
//     pose_received = true;

//     state[0] = msg.pose.pose.position.x;

//     // convert from Autoware map to roboat map reference
//     state[1] = -msg.pose.pose.position.y;

//     // quaternion in Autoware map to theta in roboat map reference
//     m = tf::Matrix3x3(
//         tf::Quaternion(msg.pose.pose.orientation.x, msg.pose.pose.orientation.y, msg.pose.pose.orientation.z, msg.pose.pose.orientation.w));
//     m.getRPY(roll, pitch, state[2]);
//     //ROS_ERROR("state2-pre: %f", state[2]);
//     state[2] *= -1;
//   } else{
//       ROS_ERROR("Can't control boat. Pose has high covariance.");
//   }
// }

// // add twist (velocity) to state
// void MPC::twistCallback(const geometry_msgs::TwistWithCovarianceStamped msg)
// {
//   state[3] = msg.twist.twist.linear.x;
//   state[4] = -msg.twist.twist.linear.y;
//   state[5] = -msg.twist.twist.angular.z;
// }

void MPC::pathCallback(const nav_msgs::Path::ConstPtr& msg)
{
  int i = 0;
  tf::Matrix3x3 m;
  double roll, pitch, yaw, *pos, *previous_pos;
  double dx; 
  double dy;
  double dTheta;

  for (std::vector<geometry_msgs::PoseStamped>::const_iterator it = msg->poses.begin();
       it != msg->poses.end() && i < num_steps; ++it, ++i)
  {
    
    pos = &trajectory[NX * i];

    *pos = it->pose.position.x;

    // convert from Autoware map to roboat map reference
    *(pos + 1) = it->pose.position.y;

    // quaternion in Autoware map to theta in roboat map reference
    m = tf::Matrix3x3(
        tf::Quaternion(it->pose.orientation.x, it->pose.orientation.y, it->pose.orientation.z, it->pose.orientation.w));
    m.getRPY(roll, pitch, yaw);
    *(pos + 2) = yaw;

    if (i == 0)
    {
      previous_pos = last_state;
    } 
    else
    {
      previous_pos = trajectory + NX * (i-1);
    }
     dx     = *(pos + 0) - *(previous_pos + 0);
     dy     = *(pos + 1) - *(previous_pos + 1);
     dTheta = *(pos + 2) - *(previous_pos + 2);

    // Ensure all trajectory angles have same sign
  	if (dTheta > M_PI)
  	{
  	  *(pos + 2) -= 2 * M_PI;
  	}
  	else if (dTheta < -M_PI)
  	{
  	  *(pos + 2) += 2 * M_PI;
  	}




    *(pos + 3) = ( cos(*(pos + 2))*(dx) + sin(*(pos + 2))*(dy))/step;
    *(pos + 4) = (-sin(*(pos + 2))*(dx) + cos(*(pos + 2))*(dy))/step;
    *(pos + 5) = (dTheta)/step;

    if(*(pos + 3) > 1      || *(pos + 3) < -1)                *(pos + 3) = last_state[3];
    if(*(pos + 4) > 1      || *(pos + 4) < -1)                *(pos + 4) = last_state[4];
    if(*(pos + 5) > M_PI/6 || *(pos + 5) < -M_PI/6)           *(pos + 5) = last_state[5]; 

    //trajectory[NX * i + 0] = *pos ;
    //trajectory[NX * i + 1] = *(pos + 1) ;
   // trajectory[NX * i + 2] = *(pos + 2) ;
    //trajectory[NX * i + 3] = *(pos + 3) ;
   // trajectory[NX * i + 4] = *(pos + 4) ;
    //trajectory[NX * i + 5] = *(pos + 5) ;

    //ROS_ERROR("%f, %f, %f, %f, %f, %f", trajectory[NX * i+0], trajectory[NX * i+1], trajectory[NX * i+2], trajectory[NX * i+3], trajectory[NX * i+4], trajectory[NX * i+5]);
    //ROS_ERROR("%f, %f, %f, %f, %f, %f", trajectory[0], trajectory[1], trajectory[2], trajectory[3], trajectory[4], trajectory[5]);
    //ROS_ERROR("%f, %f, %f, %f", dx, dy,step, dTheta);
    //ROS_ERROR("%f, %f, %f", *(pos + 3),*(pos + 4), *(pos + 5));
    //ROS_ERROR("%f, %f", *(pos + 2),*(previous_pos + 2));
  }
  
  //ROS_ERROR("%d", i);
  last_state[0] = trajectory[NX * (i-1) + 0];
  last_state[1] = trajectory[NX * (i-1) + 1];
  last_state[2] = trajectory[NX * (i-1) + 2];
  last_state[3] = trajectory[NX * (i-1) + 3];
  last_state[4] = trajectory[NX * (i-1) + 4];
  last_state[5] = trajectory[NX * (i-1) + 5];
  
  
  // prepare trajectory for use if at least length 1
  if (i > 0)
  {
    path_received = true;
    trajectoryFill(trajectory, i);
  }
  else
  {
    ROS_ERROR("path received is empty");
  }
}

// ensures full trajectory up to desired length (MPC_N instances of x,y,theta)
void MPC::trajectoryFill(double* trajectory, int pathSize)
{
  // store first point in trajectory as lastGoal
  if (pathSize > 0)
  {
    for (int j = 0; j < NX; j++)
      lastGoal[j] = trajectory[(pathSize-1)+j];
  }
  // use lastGoal as desired first point if trajectory is empty
  else if (pathSize == 0)
  {
    for (int j = 0; j < NX; j++)
      trajectory[j] = lastGoal[j];
    pathSize = 1;
  }

  // fill trajectory through index MPC_N-1 with index pathSize value
  for (int j = 6 * pathSize; j < NX * num_steps; j++)
  {
    trajectory[j] = trajectory[j - NX];
  }
}

std::vector<double> MPC::acadoForce(std::vector<double> state, double* trajectory)
{
  /* Some temporary variables. */
  int i, j;
  acado_timer t;
 
  for (i = 0; i < N; ++i)
  {
    for (j = 0; j < NX; ++j)
    {
      acadoVariables.y[i * NY + j] = trajectory[i * NX + j];
	  }
  }

  for (i = 0; i < NYN; ++i)
  {
    acadoVariables.yN[i] = trajectory[(N - 1) * NX + i];
  }

  /* MPC: initialize the current state feedback. */
  for (i = 0; i < NX; ++i)
    acadoVariables.x0[i] = state[i];
  
  /* Get the time before start of the loop. */
  acado_tic(&t);

  /* Perform the feedback step. */
  acado_feedbackStep();
  
  /* Read the elapsed time. */
  real_t te = acado_toc(&t);
  
  
    // Shift states and controls
  acado_shiftStates(2, 0, 0);
  acado_shiftControls( 0 );
  acado_preparationStep();

  ROS_DEBUG("[MPC_NODE] ACADO runtime: %fs", te);

  // acado_printDifferentialVariables();
  // acado_printControlVariables();

  std::vector<double> control(acadoVariables.u, acadoVariables.u + 4); //where is the definition of control()?

  return control;
}

nav_msgs::Path MPC::acadoPrediction()
{
  std::vector<double> prediction(acadoVariables.x,
                                 acadoVariables.x + NX * N); // 1d vector containing the predictions of the NX systems
                                                             // states for N steps in the future
  nav_msgs::Path path_msg;
  path_msg.header.frame_id = odom_frame_;
  path_msg.header.stamp = ros::Time::now();
  for (int i = 0; i < N; i += 1)
  {
    geometry_msgs::PoseStamped pose;
    pose.header.frame_id = odom_frame_;
    pose.header.stamp = ros::Time::now();
    pose.pose.position.x = prediction[i * NX + 0]; // i*NX skips ahead NX positions in the prediction vector
    pose.pose.position.y =
        -prediction[i * NX + 1]; // minus sine because of NED -> ENU coordinate frame transformation (ACAD0 -> odom)
    pose.pose.orientation = tf::createQuaternionMsgFromYaw(-prediction[i * NX + 2]);
    path_msg.poses.push_back(pose);
  }
  return path_msg;
}


MPC::MPC(ros::NodeHandle n)
{
  state = std::vector<double>(6);
  state_roll_over_count = 0;

  for (int i = 0; i < 6; i++)
    lastGoal[i] = 0.;

  n.param("system_dynamics/num_steps", num_steps, 40);

  // publisher for force topic
  force_pub = n.advertise<roboat_core::Force>("mpc_force", 10);

  // publisher of the path predicted by ACADO
  prediction_pub = n.advertise<nav_msgs::Path>("nmpc/path_prediction", 1);

  // subscriber for geometry_msgs Pose 
  path_sub = n.subscribe("roboat_path", 1, &MPC::pathCallback, this);

  // subscriber for geometry_msgs Pose 
 // pose_sub = n.subscribe("filtered_pose", 1, &MPC::poseCallback, this);

  // subscriber for geometry_msgs Twist 
//  twist_sub = n.subscribe("filtered_twist", 1, &MPC::twistCallback, this);

    //subscriber for sensor state from microcontroller
    ros::Subscriber state_sub = n.subscribe("odometry/filtered", 1, &MPC::stateCallback, this); 


  std::string id;
  n.param<std::string>("roboat_id",id,"");
  if (id.empty()) {
    odom_frame_ = "odom";
  }
  else {
    odom_frame_ = "odom_"+id;
  }
  
  n.param("system_dynamics/step", step, 0.1);

  ros::Rate loop_rate(1/step);

  ros::Time begin = ros::Time::now();

  std::vector<double> force(4, 0);
  
  /* Clear solver memory. */
  memset(&acadoWorkspace, 0, sizeof( acadoWorkspace ));
  memset(&acadoVariables, 0, sizeof( acadoVariables ));
  
  /* Initialize the solver. */
  acado_initializeSolver();
  
  int i;
     
  /* Initialize the states and controls. */
  for (i = 0; i < NX * (N + 1); ++i)  acadoVariables.x[ i ] = 0.0;
  for (i = 0; i < NU * N; ++i)  acadoVariables.u[ i ] = 0.0;
    
  /* Initialize the measurements/reference. */
  for (i = 0; i < NY * N; ++i)  acadoVariables.y[ i ] = 0.0;
  for (i = 0; i < NYN; ++i)  acadoVariables.yN[ i ] = 0.0;
  
  	// Warm-up the solver
	//
  acado_preparationStep();
     
  while (ros::ok())
  {
    ros::spinOnce();

    double currentTime = ros::Time::now().toSec();

    // ensure trajectory and state angles have same sign
    double dTheta = state[2] - trajectory[2];
    
    if (dTheta > M_PI)
    {
      state[2] -= 2 * M_PI;
    }
    else if (dTheta < -M_PI)
    {
      state[2] += 2 * M_PI;
    }
    
    nav_msgs::Path prediction;
    if (path_received && pose_received)
    { 
      ROS_DEBUG("[MPC_NODE] acting on received trajectory update");
      force = acadoForce(state, trajectory);
      // Publish the MPC force
      roboat_core::Force msg;
      std::copy(force.begin(), force.end(), &msg.data[0]);
      force_pub.publish(msg);

      // Publish the NMPC predicted trajectory
      prediction = acadoPrediction();
      prediction_pub.publish(prediction);
    }
    else {
      // Publishes empty trajectory to clear previous predicted trajectory
      prediction.header.frame_id = odom_frame_;
      prediction.header.stamp = ros::Time::now();
      prediction_pub.publish(prediction);
    }
    
    path_received = false;
    pose_received = false;

    double endTime = ros::Time::now().toSec();
    ROS_DEBUG("[MPC NODE] loop runtime: %fs\n", endTime - currentTime);
    loop_rate.sleep();
  }

  roboat_core::Force msg;
  memset(&msg.data[0], 0, sizeof(msg.data));
  force_pub.publish(msg);
}

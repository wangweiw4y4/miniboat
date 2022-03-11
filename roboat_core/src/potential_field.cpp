/** ----------------------------------------------------------------------------
 * @file:     roboat_pf.cpp
 * @date:     July 18, 2021
 * @datemod:  Nov 12, 2021
 * @author:   Alejandro Gonzalez-Garcia
 * @email:    alexglzg97@gmail.com
 * 
 * @brief: Potential fields algortithm for distributed formation control. 
 * ---------------------------------------------------------------------------*/

#include <roboat_core/potential_field.h>

using namespace Eigen;

PotentialField::PotentialField(ros::NodeHandle nh) : nh_(nh)
{
    //ROS Publishers and Subscribers
    shape_sub_ = nh_.subscribe("/shape", 10, &PotentialField::shapeCallback, this);
    tau_pub_ = nh_.advertise<geometry_msgs::Pose2D>("tau", 1);
    force_pub_ = nh_.advertise<roboat_core::Force>("pf_force", 1);
    heading_setpoint_pub_ = nh_.advertise<std_msgs::Float64>("heading_setpoint", 1);
    heading_pub_ = nh_.advertise<std_msgs::Float64>("heading", 1);
    heading_controleffort_sub_ = nh_.subscribe("control_effort", 1, &PotentialField::heading_controleffortCallback,this);



    swarm_.initialize(nh_);
    swarm_size_ = swarm_.getBoatN();
    idx_ = swarm_.getIdx();
    if (idx_ == -1 || swarm_size_<= 0) {
        ROS_ERROR("wrong swarm definition");
        idx_ = 0; 
    }
    nh_.param("/pf/detection_max_lapse_", detection_max_lapse_, 1.0);

    std::string id, ns;
    nh_.param<std::string>("roboat_id",id,"");
    if (!id.empty()) {
        ns="/"+id+"/"; //uses id to build global namespace
        shape_msg_.header.frame_id = "odom_" +id;
    }
    else {
        ns = id;
        shape_msg_.header.frame_id = "odom";
    }
    shape_pub_ = nh_.advertise<nav_msgs::Path>("visual/shape", 1);
    

    static const float d_target_region = 0.3; //half of the target square side
    static const float d_target_r0 = 1.0; //target r0
    static const float d_target_stf = 0.004; //target lattice force coefficient
    static const float d_target_srf = 0.008; //target repulsive force coefficient
    static const float d_multi_region = 2.0;
    static const float d_multi_r0 = 10.0;
    static const float d_multi_stf = 0.001;
    static const float d_multi_srf = 0.005;
    static const int d_shrink_time = 120;
    static const int d_inside_time = 45;
    static const float d_x_center = 1.0;
    static const float d_y_center = 3.0;
    static const float d_attractive_par_1 = 0.01;
    static const float d_attractive_par_2 = 0.5;
    
    nh_.param("pf/target_region", target_region, d_target_region);
    nh_.param("pf/target_r0", target_r0, d_target_r0);
    nh_.param("pf/target_stf", target_stf, d_target_stf);
    nh_.param("pf/target_srf", target_srf, d_target_srf);
    nh_.param("pf/multi_region", multi_region, d_multi_region);
    nh_.param("pf/multi_r0", multi_r0, d_multi_r0);
    nh_.param("pf/multi_stf", multi_stf, d_multi_stf);
    nh_.param("pf/multi_srf", multi_srf, d_multi_srf);
    nh_.param("pf/shrink_time", shrink_time, d_shrink_time);
    nh_.param("pf/inside_time", inside_time, d_inside_time);
    nh_.param("pf/x_center", x_center, d_x_center);
    nh_.param("pf/y_center", y_center, d_y_center);
    nh_.param("pf/attractive_par_1", attractive_par_1, d_attractive_par_1);
    nh_.param("pf/attractive_par_2", attractive_par_2, d_attractive_par_2);
    
    
    pose << 0.0, 0.0;
    number_of_robots = 0;
    counter = 0;
    des_shape = -1; //starts with no active shape being tracked
}

void PotentialField::heading_controleffortCallback(const std_msgs::Float64 & control_msg)
{
   control_effort =control_msg.data;
}


void PotentialField::shapeCallback(const roboat_msgs::Shape &msg) 
{
    ROS_INFO("new shape loaded: %d",msg.shape_code);
    des_shape = (float)msg.shape_code;
    //re-starts shape region
    region = multi_region*target_region; //half of the initial square side
    r0 = multi_r0*target_r0; //initial r0
    stf = multi_stf*target_stf; //initial lattice force coefficient
    srf = multi_srf*target_srf; //initial repulsive force coefficient
    counter = 0;
    //if no shape, 
    if (des_shape<0) {
        shape_msg_.header.stamp = ros::Time::now();
        shape_msg_.poses.clear();
        shape_pub_.publish(shape_msg_);
    }
    return;
}

std::vector<point_t> PotentialField::updateShape(){
    //if not code provided, uses as default the desired shape saved internally
    return updateShape(des_shape);
}

std::vector<point_t> PotentialField::updateShape(float shape_number_code){
    std::vector<point_t> shape_vert = { { 0.0,0.0 } };
    if (shape_number_code == 1){
        //rectangle
        shape_vert = { {-region/2 + x_center, -5*region/2 + y_center}, { region/2 + x_center,-5*region/2 + y_center }, { region/2 + x_center,5*region/2 + y_center }, { -region/2 + x_center,5*region/2 + y_center }  };
    }
    else if (shape_number_code == 2){
        //triangle
        shape_vert = { {-region/2 + x_center, -region/2 + y_center}, { region/2 + x_center,-region/2 + y_center }, { x_center,2*region/2 + y_center }  };
    }
    else { //default value (shape_number_code == 0 || shape_number_code == -1){
        //square
        shape_vert = { {-region/2 + x_center, -region/2 + y_center}, { region/2 + x_center,-region/2 + y_center }, { region/2 + x_center,region/2 + y_center }, { -region/2 + x_center,region/2 + y_center }  };
    }
    
    return shape_vert;
}

void PotentialField::timeStep(polygon_t _shape)
{
    //if no active shape being tracked, no need to continue
    if (des_shape < 0) {
        return;
    }

    //update region and force parameters as the shape shrinks
    if (counter >= counter_thresh){
        region = region - (region_rate);
        r0 = r0 - (r0_rate);
        srf = srf + (srf_rate);
        stf = stf + (stf_rate);
    }

    //stop shrinking if target shape is reached
    if (region <= target_region){
        region = target_region;
    }

    if (r0 <= target_r0){
        r0 = target_r0;
    }

    if (srf >= target_srf){
        srf = target_srf;
    }

    if (stf >= target_stf){
        stf = target_stf;
    }

    /* Since the swarm information is used to identify the neighbors rather than a local sensor, 
    we use the swarm data to determine the position of the miniboat and all the fleet */
    // boat current position
    pose(0) = swarm_.state_[idx_][0]; //pose in x
    pose(1) = swarm_.state_[idx_][1]; //pose in y
    psi = swarm_.state_[idx_][2]; //yaw
    //detected neighbors position
    robots_detected.clear();
    number_of_robots = 0;
    double curr_time = ros::Time::now().toSec();
    for (int i=0; i<swarm_size_; i++)
    {
        //if self or neighbor is not updating its position properly, skipped
        if (i == idx_ || curr_time - swarm_.last_beat_[i] > detection_max_lapse_) {
            continue;
        }
        det_robot_pose << swarm_.state_[i][0], swarm_.state_[idx_][1];
        robots_detected.push_back(det_robot_pose);
        number_of_robots++;
    }

    //update shape and position
    shape_vertexes = updateShape(des_shape);
    _shape.segs = {};
    _shape.AddPolygon(shape_vertexes);
    position = { pose(0), pose(1) };
    regf = _shape.RegionForce(position); //compute regional force
    attractive_force << regf.x, regf.y;

    //publishes the message with the shape info, so it can be plotted in rviz
    shape_msg_.header.stamp = ros::Time::now();
    shape_msg_.poses.resize(shape_vertexes.size());
    for (int i=0; i<shape_vertexes.size(); i++ ) {
        shape_msg_.poses[i].header = shape_msg_.header;
        shape_msg_.poses[i].pose.position.x = shape_vertexes[i].x;
        shape_msg_.poses[i].pose.position.y = -shape_vertexes[i].y; //back to odom frame
        shape_msg_.poses[i].pose.position.z = 0.0;
    }
    if (!shape_msg_.poses.empty()) {
        shape_msg_.poses.push_back(shape_msg_.poses[0]);
    }
    shape_pub_.publish(shape_msg_);


    //compute repulsive force for each respective neighbor
    repulsive_force << 0.0, 0.0;
    Fr << 0.0, 0.0;
    Ftheta << 0.0, 0.0;
    for (int i=0; i<number_of_robots; i++){
        current_det_pose << robots_detected[i][0], robots_detected[i][1];
        r = pow((pow(pose(0)-current_det_pose(0),2) + pow(pose(1)-current_det_pose(1),2)),0.5);
        pose_difference = current_det_pose - pose;
        Fr = ((pose_difference)/(r)) * (r0 / r*(1 - (r0 / r))) + Fr;
        theta_dir = atan2(pose_difference(1),pose_difference(0));
        inverted_pose << -pose_difference(1), pose_difference(0);
        Ftheta = sin(4 * theta_dir) * (inverted_pose / r) / r + Ftheta;
    }
    repulsive_force = srf * Fr + stf * Ftheta;
    //ROS_FATAL_STREAM("rep_f = " << repulsive_force);
    linear_force = attractive_force + repulsive_force; //add attractive and repulsive forces

    //saturate the resulting force
    if (std::abs(linear_force(0)) > 0.05){
        linear_force(0) = copysign(0.05,linear_force(0));
    }

    if (std::abs(linear_force(1)) > 0.05){
        linear_force(1) = copysign(0.05,linear_force(1));
    }

    //ROS_INFO("fx %f, fy %f",linear_force(0), linear_force(1));

    //Transformation matrix
    rotation << cos(psi), -sin(psi),
        sin(psi), cos(psi);
    //set the force in the body frame
    body_force = rotation.transpose()*linear_force;

    tau_nu << body_force(0), body_force(1), control_effort;

    tau.x = tau_nu(0);
    tau.y = tau_nu(1);
    tau.theta = control_effort;
    



    //Needs to allocate force to all thrusters to yield the required total tau
    allocated_force = allocateForce(tau_nu);
    std::copy(allocated_force.begin(), allocated_force.end(), &force_msg.data[0]);
    
    //Data publishing
    tau_pub_.publish(tau); //send the force to the miniboat
    force_pub_.publish(force_msg);
    counter += 1;
}


std::vector<double> PotentialField::allocateForce(Vector3f tau)
{
    //conversion to split the force between two thrusters placed at 45 degrees
    double tau2thruster = 0.5/sqrt(2.0); //
    double mom2thruster = 3.5355; //1/(0.2*sqrt(2)) since the miniboat is an square of 0.2m
    
    //initializes output
    std::vector<double> force(4,0.0);
    
    //forward force
    if(tau(0)>0) {    
        force[0] = tau(0)*tau2thruster;
        force[1] = tau(0)*tau2thruster;
        force[2] = 0.0;
        force[3] = 0.0;
    }
    else {
        force[0] = 0.0;
        force[1] = 0.0;
        force[2] = - tau(0)*tau2thruster;
        force[3] = - tau(0)*tau2thruster;
    }
    //sideways force (assume force is bounded so thrusters can provide all as required)
    if(tau(1)>0)
    {    
        force[0] += tau(1)*tau2thruster;
        force[2] += tau(1)*tau2thruster;
    }
    else {
        force[1] += - tau(1)*tau2thruster;
        force[3] += - tau(1)*tau2thruster;
    }
    //moment
    if (tau(2)>0) {
        //force[0] += tau(2)*mom2thruster;
       // force[3] += tau(2)*mom2thruster;
        force[0] += std::min(double(tau(2)), 1.0);
  	    force[3] += std::min(double(tau(2)), 1.0);

    }
    else {
        //force[1] += -tau(2)*mom2thruster;
        //force[2] += -tau(2)*mom2thruster;
        force[1] += std::min(-double(tau(2)), 1.0);
  	    force[2] += std::min(-double(tau(2)), 1.0);
    }
    return force;
}

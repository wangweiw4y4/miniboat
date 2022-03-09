/** ----------------------------------------------------------------------------
 * @file:     roboat_pf.cpp
 * @date:     July 18, 2021
 * @datemod:  Nov 12, 2021
 * @author:   Alejandro Gonzalez-Garcia
 * @email:    alexglzg97@gmail.com
 * 
 * @brief: Potential fields algortithm for distributed formation control. 
 * ---------------------------------------------------------------------------*/

#include <iostream>
#include <math.h>
#include "ros/ros.h"
#include "geometry_msgs/Pose2D.h"
#include "geometry_msgs/Vector3.h"
#include <nav_msgs/Path.h>
#include <eigen3/Eigen/Dense>
#include <tf2/LinearMath/Quaternion.h>
#include "VFF/VirtualForceField.h"
#include <roboat_core/swarm.h>
#include <roboat_core/Force.h>
#include <roboat_msgs/Shape.h>

using namespace Eigen;

class PotentialField
{
public:

    Vector3f tau_nu; //vector of forces
    std::vector<double> allocated_force; //arrawy of allocated forces
    
    float r; //distance between robots
    float theta_dir; //angle between robots
    float psi; //miniboat heading

    float r0; //desired distance between robots
    float stf; //coefficient for lattice force
    float srf; //coefficient for repulsive force
    float region; //region shortest side lenght
    float counter; //counter to get inside and shrink the shape
    float counter_thresh; //switch from getting inside to shrinking
    float region_rate; //rate to shrink region
    float r0_rate; //rate to decrease desired distance between robots
    float srf_rate; //rate to increase repulsive coefficient
    float stf_rate; //rate to increase lattice coefficient
    float des_shape; //0 for square, 1 for rectangle, 2 for triangle

    //user parameters
    float x_center; //region center in x
    float y_center; //region center in y
    int shrink_time; //desired time to shrink
    int inside_time; //desired time to get inside
    float target_region; //target region shortest side length
    float target_r0; //target desired distance between robots
    float target_srf; //target repulsive coefficient
    float target_stf; //target lattice coefficient
    float multi_region; //multiplier for initial region shortest side length
    float multi_r0; //multiplier for initial desired distance between robots
    float multi_srf; //multiplier for initial repulsive coefficient
    float multi_stf; //multiplier for initial lattice coefficient
    float attractive_par_1; //attractive force parameter 1
    float attractive_par_2; //attractive force parameter 2

    Vector2f pose; //miniboat pose
    Vector2f attractive_force; //attractive force
    Vector2f repulsive_force; //repulsive force + lattice force
    Vector2f Fr; //repulsive force
    Vector2f Ftheta; //lattice force
    Vector2f linear_force; //control input force in inertial frame
    Vector2f det_robot_pose; //pose of current detected neighbor to create the array
    Vector2f body_force; //control input force in body frame
    Matrix2f rotation; //rotation matrix
    
    Vector2f current_det_pose; //pose of current detected neighbor to compute repulsive force
    Vector2f pose_difference; //pose difference between the miniboat and a neighbor
    Vector2f inverted_pose; //special pose arrangement for lattice force

    std::vector<Vector2f> robots_detected; //poses of every detected neighbor
    int number_of_robots; //number of neighbors

    geometry_msgs::Pose2D tau;      //desired force and moment for publisher
    roboat_core::Force force_msg;   //allocate force to the thrusters

    //position
    point_t position = { 0.0, 0.0 };
    point_t dir = { 0.0, 0.0 };
    point_t regf = { 0.0, 0.0 };
    std::vector<point_t> shape_vertexes;
    
    PotentialField(ros::NodeHandle nh);

    std::vector<point_t> updateShape();
    std::vector<point_t> updateShape(float shape_number_code);
    
    void timeStep(polygon_t _shape);

private:
    ros::NodeHandle nh_;
    ros::Publisher tau_pub_;
    ros::Publisher force_pub_;
    ros::Publisher shape_pub_;
    ros::Subscriber shape_sub_;

    nav_msgs::Path shape_msg_;
    
    Swarm swarm_;
    int swarm_size_, idx_;
    double detection_max_lapse_;

    std::vector<double> allocateForce(Vector3f tau);

    void shapeCallback(const roboat_msgs::Shape &msg);

};

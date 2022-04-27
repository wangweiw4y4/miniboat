/*------------------------------------------------
Thruster Calibration code
M.I.T. April 2022
Just two thrusters for now ...
------------------------------------------------*/

// libraries
// ROS
#include <ros/ros.h>
#include "math.h"
#include <nav_msgs/Odometry.h>
#include <roboat_core/Force.h>


// ROS spin settings
int hz = 10; // time delay, 100 ms


// Compare thruster A and B;
float angular_v; //instantaneous angular velocity value, taken from mini-boat odometry.
float thruster_A = 0.1; // thruster value, A
float thruster_B = 0.1; // thruster value, B
float calibration_factor = 1; // ratio between thrusters A and B. ----> write into .XML/.YAML file.


// calibration settings
int t = 0; // time-step
bool startup = true;
bool calibration = false;
bool test_phase = false;
int startup_time = 50; // 50 * 1/10 (10 hz loop rate) = 5 seconds
int calibration_time = 300; // 300 * 1/10 (10hz loop rate) = 30 seconds
int threshold = 0.05; // angular velocity threshold ... radians/s?
int angular_stability = 0; // start counter when angular_v drops below threshold. To make usre it's stable, not just a blip in the angular_v.


// ------------------------------------------------
// functions


void increment_thruster() {
    if (angular_v > threshold) { // angular velocity is +ive, therefore increase thruster B.
        thruster_B = thruster_B*1.01;
    }
    else if (angular_v < - threshold) { // angular velocity is -ive, therefore decrease thruster B.
        thruster_B = thruster_B*0.99;
    }
    else {
        // do nothing.
    }
}


void calculate_calibration_factor() {
    calibration_factor = thruster_B/thruster_A;
}


void write_to_configfile() {
    // write to main.yaml?
}


void test_if_it_works() {
    // use the new configuration factor to move straight(?)
    test_phase = true;
}


void stateCallback(const nav_msgs::Odometry msg) {
    
    angular_v = -msg.twist.twist.angular.z;
    ROS_INFO("angular velocity: %f", angular_v); 
}



// ------------------------------------------------
// main
int main(int argc, char** argv) {

    ros::init(argc, argv, "thruster_calibration_node");
    ros::NodeHandle nh;
    ros::Subscriber state_sub = nh.subscribe("/miniboat4/odometry/filtered", hz, stateCallback);  // to get the angular velocity, odometry
    ros::Publisher force_pub = nh.advertise<roboat_core::Force>("/miniboat4/mpc_force", hz); // to publish the forces to the thrusters
    ros::Rate loop_rate(hz);

    while(ros::ok){

        t++; // increment time step

        if (startup == true && t > startup_time) {
            startup = false;
            calibration = true;
            t = 0;
        }

        if (calibration){
            ROS_INFO("calibrating ...");

            // -----------------------------------
            // Publish the forces to the thrusters.
            roboat_core::Force force_msg;
            force_msg.data = {thruster_A, thruster_B, 0, 0};
            ROS_INFO("Thruster values: %f and %f", force_msg.data[0], force_msg.data[1]); 
            force_pub.publish(force_msg);
            // -----------------------------------


            if (abs(angular_v) > threshold) {
                increment_thruster(); // either subtract or add to one of the thrusters based on the current angular velocity.
                angular_stability = 0;
            }
            else {
                
                angular_stability ++; // a simple counter; see for how many time-step the angular_velocity remains ~ 0 ...
                ROS_INFO("Angular velocity < threshold :) ... waiting how long it lasts");

                if (angular_stability == 10 || t>calibration_time) { // once it's stable, or too much time has passed, call the calibration finished.
                    calculate_calibration_factor(); // ratio of the two thruster values.
                    ROS_INFO("Finished calibration!");
                    ROS_INFO("calibration finished, calibration factor: %f", calibration_factor);
                    write_to_configfile();
                    calibration = false;
                    // t = 0;
                }
            }
        }
        else if (test_phase) {
            // try out the new calibration factor.
        }

        ros::spinOnce();
        loop_rate.sleep();
    }

    return 0;
}


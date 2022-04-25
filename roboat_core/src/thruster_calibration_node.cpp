/*------------------------------------------------
Thruster Calibration code
M.I.T. April 2022
Just two thrusters for now ...
------------------------------------------------*/

// libraries
// ROS
#include <ros/ros.h>
#include "math.h"
// #include <yaml-cpp/yaml.h>
#include <nav_msgs/Odometry.h>
#include <roboat_core/Force.h>


//roboat_core::Force force_msg;


// ROS spin settings
int hz = 10; // time delay, 100 ms


// Compare thruster A and B;
float angular_v; //instantaneous angular velocity value, taken from mini-boat odometry.
float thruster_A = 0.2; // thruster value, A
float thruster_B = 0.2; // thruster value, B
float calibration_factor = 1; // ratio between thrusters A and B. ----> write into .XML/.YAML file.


// calibration settings
int t = 0;
bool startup = true;
bool calibration = false;
bool test_phase = false;
int startup_time = 50; // 50 * 1/10 (10 hz loop rate) = 5 seconds
int calibration_time = 120; // 120 * 1/10 (10hz loop rate) = 12 seconds
int threshold = 0.05; // angular velocity threshold ... radians/s?


// ------------------------------------------------
// functions


void increment_thruster() {
    if (angular_v > threshold) {
        thruster_B = thruster_B*1.01;
    }
    else if (angular_v < - threshold) {
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

      //subscriber for sensor state from microcontroller
    ros::Subscriber state_sub = nh.subscribe("/miniboat8/odometry/filtered", hz, stateCallback); 
    ros::Publisher force_pub = nh.advertise<roboat_core::Force>("/miniboat8/mpc_force", hz);
    ros::Rate loop_rate(hz);

    while(ros::ok){

        // Publish the forces to the thrusters.
        // new thruster forces.
        roboat_core::Force force_msg;
    	// force_msg.data[0] = thruster_A;
    	// force_msg.data[1] = thruster_B;
    	// force_msg.data[2] = 0;
    	// force_msg.data[3] = 0;
    	force_msg.data = {thruster_A, thruster_B, 0, 0};
        force_pub.publish(force_msg);

        t++; // increment time step

        if (startup == true && t > startup_time) {
            startup = false;
            calibration = true;
            t = 0;
        }

        if (calibration){
            ROS_INFO("calibrating ...");

            if (abs(angular_v) > threshold) {
                increment_thruster(); // either subtract or add to one of the thrusters based on the current angular velocity.
            }
            else if (abs(angular_v) < threshold || t>calibration_time) {
                calculate_calibration_factor(); // ratio of the two thruster values.
                printf("Finished calibration");
                ROS_INFO("calibration finished, calibration factor: %f", calibration_factor);
                write_to_configfile();
                calibration = false;
                t = 0;
                break;
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


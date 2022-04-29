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
float thruster_A = 0.2; // thruster value, A
float calibration_factor = 1; // ratio between thrusters A and B. ----> write into .XML/.YAML file.
float thruster_B = thruster_A*calibration_factor; // thruster value, B - 1.257

// calibration settings
int t = 0; // time-step
bool startup = true;
bool calibration = false;
bool test_phase = false;
int startup_time = 50; // 50 * 1/10 (10 hz loop rate) = 5 seconds
int initial_accel = 50; // time for the boat thrusters to have an effect when you first switch on.
int thruster_waiting_time = 0; // counter so that the thruster only gets updated intermittently.


// calibration settings
int calibration_time = 1200; // 300 * 1/10 (10hz loop rate) = 30 seconds
float threshold = 0.05; // angular velocity threshold ... radians/s?
int angular_stability = 0; // start counter when angular_v drops below threshold.
float delta = 0.01; // factor by which we we change the thruster value when we make an adjustment. i.e. 0.01 = 1%.
int n = 0; // calibration cycles; after each cycle we log the new calibration value and recalculate the thrusters.


// ------------------------------------------------
// functions

void increment_thruster()
{

void increment_thruster() {

    if (angular_v > threshold) { // angular velocity is +ive, therefore increase thruster B.
        thruster_B = thruster_B*(1+delta);
    }
    else if (angular_v < - threshold) { // angular velocity is -ive, therefore decrease thruster B.
        thruster_B = thruster_B*(1-delta);
    }
    else
    {
        // do nothing.
    }
}

void calculate_calibration_factor()
{
    calibration_factor = thruster_B / thruster_A;
}

void write_to_configfile()
{
    // write to main.yaml?
}

void test_if_it_works()
{
    // use the new configuration factor to move straight(?)
    test_phase = true;
}

void stateCallback(const nav_msgs::Odometry msg)
{

    angular_v = -msg.twist.twist.angular.z;
    ROS_INFO("angular velocity: %f  	|	  compared to our calibration threshold: %f", angular_v, threshold);
}





// ------------------------------------------------
// main
int main(int argc, char **argv)
{

    ros::init(argc, argv, "thruster_calibration_node");
    ros::NodeHandle nh;
    ros::Subscriber state_sub = nh.subscribe("/miniboat4/odometry/filtered", hz, stateCallback); // to get the angular velocity, odometry
    ros::Publisher force_pub = nh.advertise<roboat_core::Force>("/miniboat4/mpc_force", hz);     // to publish the forces to the thrusters
    ros::Rate loop_rate(hz);

    while (ros::ok)
    {

        t++; // increment time step

        if (startup == true && t > startup_time)
        {
            ROS_INFO("Startup finished.");
            startup = false;
            calibration = true;
            t = 0;
        }

        if (calibration)
        {
            ROS_INFO("calibrating ...");

            // -----------------------------------
            // Publish the forces to the thrusters.
            roboat_core::Force force_msg;
            force_msg.data = {thruster_A, thruster_B, 0, 0};
            ROS_INFO("Thruster values: %f and %f", force_msg.data[0], force_msg.data[1]);
            force_pub.publish(force_msg);
            // -----------------------------------

			if (t > initial_accel ){
	            if (angular_v > threshold || angular_v < -threshold) {
                    
                	ROS_INFO("angular_v is above threshold!");
                	angular_stability = 0;

                	if (thruster_waiting_time == 10) {
                    	thruster_waiting_time = 0; // reset timer, so that we always wait a bit before updating the thrusters again.
                    	increment_thruster(); // either subtract or add to one of the thrusters based on the current angular velocity.
                    	ROS_INFO("Thruster value has been changed");
                	}
                	thruster_waiting_time ++;
            	}
            	else {
                	angular_stability ++; // a simple counter; see for how many time-step the angular_velocity remains ~ 0 ...
                	ROS_INFO("Angular velocity < threshold :) ... waiting how long it lasts");
                	ROS_INFO("angular stability counter: %d", angular_stability);

                	if (angular_stability == 10 || t>calibration_time) { // once it's stable, or too much time has passed, call the calibration finished.
                        n++;
                        delta = delta/(n+1); // i.e. decrease the adjustment factor every go round ... i.e. the amount by which the thruster value is changed. i.e. it becomes more stable/more damped.
                    	calculate_calibration_factor(); // ratio of the two thruster values.

                        if (n==4) {
                            ROS_INFO("Finished calibration!");
                    	    ROS_INFO("calibration finished, calibration factor: %f", calibration_factor);
                    	    write_to_configfile();
                    	    calibration = false;
                    	    // t = 0;
                    	    //break;
                        }
                	}
            	}
			}

        }
        else if (test_phase)
        {
            // try out the new calibration factor.
        }

        ros::spinOnce();
        loop_rate.sleep();
    }

    return 0;
}

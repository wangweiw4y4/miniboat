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


#define BUFFER_SIZE 20 // buffer size for the moving average.
#define STARTUP_TIME 50 // allow for startup. 50 * 1/10 (10 hz loop rate) = 5 seconds
#define INITIAL_ACCEL 50 // allow for initial acceleration, after startup.
#define HZ 10 // ROS spin setting, 100ms.

// ----------------------------------------------

// Compare thruster A and B; MOVING AVERAGE
float angular_v; //instantaneous angular velocity value, taken from mini-boat odometry.
int b = 0; // index of the buffer array.
float angular_v_buffer[BUFFER_SIZE]; 
float moving_average; // calculated from the values in the buffer array

// ----------------------------------------------

// Initial Thruster values
float thruster_A = 0.22; // thruster value, A
float calibration_factor = 1.25; // 1.27, ratio between thrusters A and B. ----> write into .XML/.YAML file.
float thruster_B = thruster_A*calibration_factor; // thruster value, B - 1.257

// ----------------------------------------------

// calibration settings
int t = 0; // time-step
bool startup = true;
bool calibration = false;
bool test_phase = false;
int thruster_waiting_time = 0; // counter so that the thruster only gets updated intermittently.


// more calibration settings
const int calibration_time = 1500; // 300 * 1/10 (10hz loop rate) = 30 seconds
float threshold = 0.10; // angular velocity threshold ... radians/s?
int angular_stability = 0; // start counter when angular_v drops below threshold.
float delta = 0.01; // factor by which we we change the thruster value when we make an adjustment. i.e. 0.01 = 1%.
int n = 0; // calibration cycles; after each cycle we log the new calibration value and recalculate the thrusters.


// ------------------------------------------------
// functions

void increment_thruster() {
	
	float d; // delta

    if (angular_v > threshold) { // angular velocity is +ive, therefore increase thruster B.
        d = delta;
        ROS_INFO("angular velocity is too +ve");
    }
    else if (angular_v < - threshold) { // angular velocity is -ive, therefore decrease thruster B.
    	d = - delta;
    	ROS_INFO("angular velocity is too -ve");
    }
    
    float q = 1+d;
    ROS_INFO("Multiplying thruster B by: %f", q);
    thruster_B = thruster_B*q;
	ROS_INFO("-------------------------------------");
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

    // always overwrite the next increment in the buffer (b++). When we reach the end, circle back around (b = 0).
    if (b < BUFFER_SIZE) {
        angular_v_buffer[b] = angular_v;
        b ++;
    }
    else {
        b = 0;
    }

    // calculate the sum of the buffer. If at the beginning, e.g. [value, 0, 0, 0] the sum will be small, so the average will look small, which is ok since it won't make any corrections (i.e. it will look like the angular velocity is small)
    float sum;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        sum =+ angular_v_buffer[i];
    }
    
    // calculate the average
    moving_average = sum/BUFFER_SIZE;
}





// ------------------------------------------------
// main
int main(int argc, char **argv)
{


    ros::init(argc, argv, "thruster_calibration_node");
    ros::NodeHandle nh;
    ros::Subscriber state_sub = nh.subscribe("/miniboat4/odometry/filtered", HZ, stateCallback); // to get the angular velocity, odometry
    ros::Publisher force_pub = nh.advertise<roboat_core::Force>("/miniboat4/mpc_force", HZ);     // to publish the forces to the thrusters
    ros::Rate loop_rate(HZ);



    while (ros::ok)
    {

        t++; // increment time step



        if (startup == true && t > STARTUP_TIME)
        {
            ROS_INFO("Startup finished.");
            startup = false;
            calibration = true;
            t = 0;
        }



        else if (calibration)
        {
            // ROS_INFO("calibrating ...");
            ROS_INFO("~ angular velocity (moving average): %f  (vs. threshold: %f ) ~", moving_average, threshold);

            
            // Publish the forces to the thrusters.
            roboat_core::Force force_msg;
            force_msg.data = {thruster_A, thruster_B, 0, 0};
            ROS_INFO("Thruster values: %f and %f", force_msg.data[0], force_msg.data[1]);
            force_pub.publish(force_msg);


			if (t > INITIAL_ACCEL ){

	            if (moving_average > abs(threshold)) {

                    angular_stability = 0; // reset angular stability index, i.e. it's not in the stable region:)
                	ROS_INFO("Above threshold!");

                	if (thruster_waiting_time == 5) {
                    	thruster_waiting_time = 0; // reset timer, so that we always wait a bit before updating the thrusters again.
                    	increment_thruster(); // either subtract or add to one of the thrusters based on the current angular velocity.
                    	ROS_INFO("---------------- Thruster value has been changed ----------------");
                	}
                	thruster_waiting_time ++;
            	}

            	else {

                	angular_stability ++; // stability index: to check how long the angular_velocity remains stable ...

                	ROS_INFO("Angular velocity is BELOW threshold ... waiting how long it lasts");
                	ROS_INFO("Angular stability counter: %d", angular_stability);

                	if (angular_stability == 5 || t>calibration_time) { // once it's stable, or too much time has passed
                        
                        n++; // go through n calibration cycles 
                        
                        if (n==1) {
                            delta = delta*0.6;
                        }
                        if (n==2 || n==3) {
                            delta = delta*0.4;
                        }

                        ROS_INFO("---------------------- Completed calibration cycle, n: %d ---------------------- ", n);
                        ROS_INFO("new delta: %f", delta);
                    	calculate_calibration_factor();
                    	angular_stability = 0;

                        if (n==4) {
                            ROS_INFO("------------------  Finished calibration!  ------------------");
                    	    ROS_INFO("calibration finished, calibration factor: %f", calibration_factor);
                    	    // write_to_configfile();
                    	    calibration = false;
                        }
                	}
            	}
			}
            else {
                // nothing
            }

        }
        else if (test_phase)
        {
            // try out the new calibration factor.
        }
        else {
            // nothing
        }

        ros::spinOnce();
        loop_rate.sleep();
    }

    return 0;
}

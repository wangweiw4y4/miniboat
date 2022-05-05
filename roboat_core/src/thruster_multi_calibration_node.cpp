/*-------------------------
Thruster Calibration code
Roboat / Mini-boats
Niklas Hagemann
MIT Senseable City Lab
May 2022
-------------------------*/


// libraries
#include <ros/ros.h>
#include "math.h"
#include <nav_msgs/Odometry.h>
#include <roboat_core/Force.h>


// ROS spin settings
int hz = 10; // time delay, 100 ms


// Compare thruster A and B;
float angular_v; //instantaneous angular velocity value, taken from mini-boat odometry.
float t_start = 0.2; // starting value
float thruster[4] = {t_start, t_start, 0, 0}; // thruster values.
float calibration_factor[3] = {1, 1, 1}; // ratio between thrusters in the BOW, PORT- and STARBOARD-sides.
int S = 3; // which side we are calibrating! 0 = Forward, 1 = left/port-side, 2=right/star-board.
bool new_S = true;


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
float init_delta = 0.01;
float delta = init_delta; // factor by which we we change the thruster value when we make an adjustment. i.e. 0.01 = 1%.
int n = 0; // calibration cycles; after each cycle we log the new calibration value and recalculate the thrusters.


// functions
void increment_thruster()
{

    float change = delta;

    if (angular_v > threshold) {

        change = delta;
    }
    else if (angular_v < threshold) {

        change = -delta;
    }

    if (S==0) { 

        thruster[1] = thruster[0]*(1+change);
    }
    else if (S==1) {

        thruster[2] = thruster[0]*(1+change);
    }
    else if (S==2) {

        thruster[3] = thruster[1]*(1+change);
    }
}

void calculate_calibration_factor()
{
    if (S==0) { 

        calibration_factor[0] = thruster[1]/thruster[0];
    }
    else if (S==1) {

        calibration_factor[1] = thruster[2]/thruster[0];
    }
    else if (S==2) {

        calibration_factor[2] = thruster[3]/thruster[1];
    }
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


void reset_thrusters() 
{
    if (S==0) { 

        ROS_INFO("Calibrating: BOW -- moving forward!");
        thruster[0] = t_start;
        thruster[1] = t_start;
        thruster[2] = 0;
        thruster[3] = 0;
    }
    else if (S==1) {

        ROS_INFO("Calibrating: PORTSIDE thruster -- moving right!");
        thruster[0] = t_start;
        thruster[1] = 0;
        thruster[2] = t_start;
        thruster[3] = 0;
    }
    else if (S==2) {

        ROS_INFO("Calibrating: STARBOARD thruster -- moving left!");
        thruster[0] = 0;
        thruster[1] = t_start;
        thruster[2] = 0;
        thruster[3] = t_start;
    }
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

        // increment time step
        t++;


        // At the beginning, wait a bit
        if (startup && t > startup_time)
        {
            ROS_INFO("Startup finished.");
            startup = false;
            calibration = true;
            t = 0;
        }


        // main calibration sequence
        else if (calibration)
        {
            ROS_INFO("calibrating ...");

            // If we are moving onto a new side, give a delay and reset the thrusters!
            if (new_S) 
            {

                ROS_INFO("New side: %d", S);

                if (t<startup_time) 
                {
                    ROS_INFO("Stopping the boat for a few seconds");
                    thruster[0] = 0;
                    thruster[1] = 0;
                    thruster[2] = 0;
                    thruster[3] = 0;
                }

                else 
                {
                    reset_thrusters();
                    delta = init_delta;                         // reset the delta
                    t = 0;                                      // reset timer so that we can account for the initial acceleration phase.
                    new_S = false;                              // finished resetting
                }

            }


            // Publish the forces to the thrusters.
            roboat_core::Force force_msg;
            force_msg.data = {thruster[0], thruster[1], thruster[2], thruster[3]};
            ROS_INFO("Thruster values: %f and %f", force_msg.data[0], force_msg.data[1]);
            force_pub.publish(force_msg);



			if (t > initial_accel )
            {                            // when starting a new calibration/side, wait until the boat has accelerated a little bit.

	            if (abs(angular_v) > threshold) 
                {

                    angular_stability = 0;                      // reset angular stability index, i.e. it's not in the stable region:)
                    thruster_waiting_time ++;

                	ROS_INFO("angular_v is above threshold!");

                	if (thruster_waiting_time == 10) 
                    {
                    	thruster_waiting_time = 0;              // reset timer, so that we always wait a bit before updating the thrusters again.
                    	increment_thruster();                   // either subtract or add to one of the thrusters based on the current angular velocity.
                    	ROS_INFO("Thruster value has been changed");
                	}
            	}

            	else
                {

                	angular_stability ++;                       // stability index: to check how long the angular_velocity remains stable ...
                	ROS_INFO("Angular velocity < threshold :) ... waiting how long it lasts");
                	ROS_INFO("Angular stability counter: %d", angular_stability);

                	if (angular_stability == 10 || t > calibration_time) {      // once it's stable, or too much time has passed, call the calibration finished.
                        
                        n++;                                    // go through n calibration cycles (with each cycle the sensitivity of the thruster adjustments is reduced)
                        delta = delta/(n+1);                    // i.e. decrease the adjustment factor every go round ... i.e. the amount by which the thruster value is changed. i.e. it becomes more stable/more damped.
                    	calculate_calibration_factor();         // ratio of the two thruster values.

                        
                        if (n==4) {                             // After n calibration cycles

                            ROS_INFO("Finished calibration!");
                    	    ROS_INFO("calibration finished, calibration factor for side %d: %f", S, calibration_factor[S-1]);
                    	    write_to_configfile();


                    	    S++;                                // move onto next side of the boat
                            t = 0;                              // reset timer for next calibration.

                            
                            if (S == 3) {                       // once we have calibrated all sides                   

                                calibration = false;
                                ROS_INFO("Finished calibration!");
                                ROS_INFO("All the calibration factors: %f, %f, %f", calibration_factor[0], calibration_factor[1], calibration_factor[2]);
                            
                            }
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

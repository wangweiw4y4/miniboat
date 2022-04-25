/*
----------------------------------------------------------------------------
Capacitive sensor node
Niklas Hagemann
MIT mini-boats 
2022.
----------------------------------------------------------------------------
*/


#include <ros/ros.h>
#include "std_msgs/Int16MultiArray.h"
#include <math.h>



int td = 10; // time delay, 100 ms
const int N = 50;  // N, the number of values we want to sample for the calibration.
int counter;
int cap_temp[4];
int cap_buffer[N][4];
int cap_baseline[4];
int threshold = 20;



void arrayCallback(const std_msgs::Int16MultiArray::ConstPtr& array);
bool allCalibrated(bool temp_c[4]);
bool calibration_sequence = true;
int latched(int i);



int main(int argc, char** argv)
{


  ros::init(argc, argv, "capacitive_node");
  ros::NodeHandle nh; 


  ros::Subscriber cap_sub = nh.subscribe("capacitive", td, arrayCallback);
  ros::Publisher cap_pub = nh.advertise<std_msgs::Int16MultiArray>("latch_state", td);


  ros::Rate loop_rate(td);



  // initialise values used for calibration
  int sensorMax[4] = {0};
  int sensorMin[4] = {1024,1024,1024,1024};
  bool calibrate[4] = {true, true, true, true};
  counter = 0;


  ROS_INFO("reached main function");

  // loop
  while(ros::ok()){

        ros::spinOnce();
        // ROS_INFO("reached while loop");

        // --------------------------------------------------------------------------------------------------------------------------
        if(calibration_sequence){  

            ROS_INFO("reached if calibration sequence");  

            // ----------------------------------
            // Once we have collected enough values
            if (counter == N) {
                for (int i=0; i<4; i++) {

                    // still need to calibrate? 
                    if (calibrate[i] == true) {

                        // calculate spread
                        for (int n=0; n<N; n++){
                
                            if (cap_buffer[n][i] > sensorMax[i]){
                                sensorMax[i] = cap_buffer[n][i];
                            }
                            else if (cap_buffer[n][i] < sensorMin[i]) {
                                sensorMin[i] = cap_buffer[n][i];
                            }
                    
                        }

                        // spead good enough?
                        if (abs(sensorMax[i] - sensorMin[i]) < 10) {

                            int sum = 0;

                            for (int n=0; n<N; n++) {
                                sum += cap_buffer[n][i];
                            }

                            cap_baseline[i] = round(sum/N);
                            calibrate[i] = false; // calibration finished
                            ROS_INFO("Sensor #%d stable, finished calibration; baseline value: %d ", i, cap_baseline[i]);
                        }
                        else {
                            ROS_INFO("Too many fluctuations, still waiting for sensor #%d to settle ...", i);
                        }


                    } 
                    else {
                        ROS_INFO("*Sensor #%d already calibrated, skipping to next", i);
                    }

                }

                // Calculate baseline if the values are stable
            }
            // ----------------------------------

            // Check if all are calibrated ...
            if (allCalibrated(calibrate) == true) { 
                calibration_sequence = false;
                ROS_INFO("-------------- Calibration done --------------");
            }
            else {
                ROS_INFO("... Still Waiting ...");
            }
        }
        // --------------------------------------------------------------------------------------------------------------------------

        else {
        
            //ROS_INFO("Debug: Entered publish part of loop ... ");
            std_msgs::Int16MultiArray latch_Msg;
            latch_Msg.data.resize(4);


            short int state[4];

            for (int i=0; i<4; i++) {
            	state[i] = latched(i);
            }
            
            latch_Msg.data = {state[0], state[1], state[2], state[3]};

      
            cap_pub.publish(latch_Msg); // publish as a ROS message
            ROS_INFO("Sensor 1: %d vs baseline %d", cap_temp[0], cap_baseline[0]);
            ROS_INFO("Sensor 2: %d vs baseline %d", cap_temp[1], cap_baseline[1]);
            ROS_INFO("Sensor 3: %d vs baseline %d", cap_temp[2], cap_baseline[2]);
            ROS_INFO("Sensor 4: %d vs baseline %d", cap_temp[3], cap_baseline[3]);
            ROS_INFO("Latch states: %d, %d, %d,%d", latch_Msg.data[0], latch_Msg.data[1], latch_Msg.data[2], latch_Msg.data[3]); // feedback for terminal
        }

        ros::spinOnce();
        loop_rate.sleep();
    }
  
    return 0;

}
  





void arrayCallback(const std_msgs::Int16MultiArray::ConstPtr& array)
{
	int i = 0;
    // ROS_INFO("Entered arrayCallback!");

    if (calibration_sequence == true) {
        if (counter == N+1) { // wrap around.
            counter = 0;
            ROS_INFO("wrap around, counter = %d", counter);
        }

        // read values from ros_topic into cap_buffer array[N][i]; i.e. keep N samples.
	    for(auto it = array->data.begin(); it != array->data.end(); ++it)
	    {
		    cap_buffer[counter][i] = *it;
		    i++;

            ROS_INFO("addded to calibration buffer, sensor # %d and counter = %d", i, counter);
	    }
        counter++;
	    return;
    }
    else {
        // read values from ros_topic into cap_temp array, only need the instantaneous values, no more buffer.
        for(auto it = array->data.begin(); it != array->data.end(); ++it)
	    {
		    cap_temp[i] = *it;
		    i++;

          // ROS_INFO("addded to temporary capacitive value array, sensor # %d with value %d and baseline %d", i, cap_temp[i], cap_baseline[i]);
	    }
    }
}




bool allCalibrated(bool temp_c[4]) { // function to check if all the sensors have been calibrated.

  for (int i=0; i<4; i++) 
  {
    if (temp_c[i] == true) { // i.e. if any are still calibrating (true), break and return false (i.e. we haven't calibrated all of them).
      return false;
      break;
    }
  }
  return true;
}


int latched(int i) { // function to check if we are latched.
  int l = 0;
  if (cap_baseline[i] - cap_temp[i] > threshold) {   // compare against the calibrated baseline
    l = 1;
  }
  else{
    l = 0;
  }
  return l;
}






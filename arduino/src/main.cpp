// Use the following line if you have a Leonardo or MKR1000
//#define USE_USBCON

/*The goal of the MC is to:
  - Control the motors by interfacing with the ESC and send the correct 
  - Initialize and calibrate the ESC when needed (by default on initialization?)
  - read the proximity sensor and publish its data
  - latch and unlatch 
*/

// #define IS_TEST true     //activates the flag to print in the serial port rather than in the ROS network

/* Compilation flags are specified here, update accordingly based on the hardware 
to which the MCU is connected to*/
#include "mc_node.h" 
//rest of local headers
#include "miniboat.h"

//ROS FRAMEWORK HEADERS
#include <ros.h>
#include <ros/time.h>
#include "std_msgs/Header.h"
#include "roboat_msgs/MicrocontrollerCommand.h"
#include "roboat_msgs/MicrocontrollerFeedback.h"

//-----------------------------
// Printing / publishing rates
#ifdef IS_TEST
  static const unsigned long SERIAL_REFRESH_INTERVAL = 250;  // 250 ms
  unsigned long serialLastRefreshTime = 0;
#else
  static const unsigned long ROS_REFRESH = 100;    // 100 ms (10 Hz)
  unsigned long rosLastRefreshTime = 0;
  unsigned long ros_refresh_interval;
#endif  


// global variables in setup and loop
Miniboat mb;


// ----------------------------
// ROS PUBLISHERS / SUBSCRIBERS
//----------------------------
ros::NodeHandle nh;
bool latch_cmd;
bool delatch_cmd;

void commandCallback(const roboat_msgs::MicrocontrollerCommand& msg)
{
  for (int i=0; i<ESC_MOTORS::NUMBER_OF_MOTORS; i++) {
    mb.frc_cmd[i] = msg.force[i];
  }
  latch_cmd = msg.latch;
  return;
}
void escCalibrateCallback(const std_msgs::Header& msg) {
  mb.calibrate();
}
ros::Subscriber<roboat_msgs::MicrocontrollerCommand> command_sub("microcontroller_command", commandCallback);
ros::Subscriber<std_msgs::Header> calibrate_sub("esc_calibrate", escCalibrateCallback);
roboat_msgs::MicrocontrollerFeedback mc_fb_msg;
ros::Publisher feedback_pub("microcontroller_feedback", &mc_fb_msg);




// ----------------------------
// MAIN PROGRAM
//-----------------------------
void setup()
{ 
  // INITIALIZE OUTPUT CHANNELS
#ifdef IS_TEST
  //Opens serial port to display results
  Serial.begin(9600);
  Serial.println("Waste Test - SETUP START");
#else
  /* Load ROS publishers and subscribers
  Needs to be done early in the setup, as otherwise may lead to synchronization error 
  when intializing the rosserial node in the host computer */
  nh.getHardware()->setBaud(57600); //57600 default, 9600, 115200
  nh.initNode();
  nh.subscribe(command_sub);
  nh.advertise(feedback_pub);
#endif

  //Initialize the gate mechanism of each controlled bin
  mb.initialize();
  
#ifdef IS_TEST
  Serial.println("Miniboat Test - SETUP DONE");
#endif
}

//-----------------------------
void loop() // put your main code here, to run repeatedly:
{
  // ----------------------------
  // CHECK BUTTONS
  // activate/deactivate autonomous mode
  // if (digitalRead(mb.SWITCH_AUTO_MANUAL) == LOW && millis() - mb.last_debounce_time > 1000) 
  // {
  //   // Serial.println("change mode");
  //   mb.last_debounce_time = millis();
  //   mb.autonomy_mode = !mb.autonomy_mode;
  //   digitalWrite(mb.AUTONOMY_LED, mb.autonomy_mode ? HIGH : LOW);
  // }
  // for now, there is no buttons connected so let's go blindly and ignore errors
  mb.autonomy_mode = true;
  mb.err_status = 0; 
  //I disable for now also the calibration check, let's just trust the switches
  mb.max_calibrated = false;
  mb.min_calibrated = false;


  // =======================================================================================================
  // FORWARD MANUAL OVERRIDE (open gates)
  if (digitalRead(mb.LATCH)==LOW)  
  {
    // Serial.print("latch_bttn "), Serial.println(k);
    if (!mb.is_open()) {
      mb.servo_open();
    }
    //disables autonomy mode since we are manually overriding the system
    mb.autonomy_mode = false;
    digitalWrite(mb.AUTONOMY_LED, LOW);
  }
  // =======================================================================================================
  // BACKWARD MANUAL OVERRIDE (retrieve gates)
  else if (digitalRead(mb.DELATCH)==LOW) 
  {
    // Serial.print("back_bttn "), Serial.println(k);
    if (mb.is_open()) {
      mb.servo_close();
    }
    //disables autonomy mode since we are manually overriding the system
    mb.autonomy_mode = false;
    digitalWrite(mb.AUTONOMY_LED, LOW);
  }
  // =======================================================================================================
  // STOP (either manual override or error found)
  else if (!mb.autonomy_mode || mb.err_status !=0) 
  {
    // Serial.println("stopped");
  }
  // =======================================================================================================
  // AUTONOMOUS MODE
  else 
  {
    // Serial.println("autonomy");
    for (int i=0; i<ESC_MOTORS::NUMBER_OF_MOTORS; i++) {
      mb.motor_speed((ESC_MOTORS)i, mb.frc_cmd[i]);
    }
  }
  

  // ----------------------------
  // PUBLISH STATUS
#ifndef IS_TEST
  /* Arduino loop runs with different delay to avoid missing enconder data, but we do not need
  to publish information at that rate */  
  if (millis() - rosLastRefreshTime >= ROS_REFRESH)
  {
    //fills feedback info message
    mc_fb_msg.header.stamp = nh.now(); //ros::Time::now(); 
    mc_fb_msg.rpm[0] = 0; //no feedback from now, need to figure out how to get it from ESC
    mc_fb_msg.rpm[0] = 0; 
    mc_fb_msg.rpm[0] = 0; 
    mc_fb_msg.rpm[0] = 0; 
    mc_fb_msg.latch = false;
    mc_fb_msg.latched[0] = false; //once the capacitive sensors working,edit
    mc_fb_msg.latched[1] = false; 
    mc_fb_msg.latched[2] = false; 
    mc_fb_msg.latched[3] = false; 
    mc_fb_msg.err_code = mb.err_status;
    feedback_pub.publish( &mc_fb_msg );
    //ROS update to recieve and publish
    rosLastRefreshTime += ROS_REFRESH;
    nh.spinOnce();
    //ROS_REFRESH_BASE already sets the frequency, no need for an extra delay()
  }
#else
  if (millis() - serialLastRefreshTime >= SERIAL_REFRESH_INTERVAL)
  {
    Serial.print(mb.rpm[0]);
    Serial.print(mb.rpm[1]);
    Serial.print(mb.rpm[2]);
    Serial.print(mb.rpm[3]);
    //
    Serial.println();
    serialLastRefreshTime += SERIAL_REFRESH_INTERVAL;
  }
#endif 
    
}

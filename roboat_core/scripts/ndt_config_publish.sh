#!/bin/bash

rostopic pub /config/ndt autoware_msgs/ConfigNdt "header:
  seq: 0
  stamp: {secs: 0, nsecs: 0}
  frame_id: ''
init_pos_gnss: 0
x: 0.0
y: 0.0
z: 0.0
roll: 0.0
pitch: 0.0
yaw: 0.0
use_predict_pose: 0
error_threshold: 1.0
resolution: 1.0
step_size: 0.1
trans_epsilon: 0.01
max_iterations: 30" 

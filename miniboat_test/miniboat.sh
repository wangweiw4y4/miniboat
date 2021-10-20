#!/bin/bash

# HOSTNAME is computer name, i.e., miniboat1, miniboat2
export HOSTNAME

# source workspace
source /opt/ros/melodic/setup.bash
source ~/catkin_ws/devel/setup.bash

# setup ip
# master ip address (the laptop ip address)
export ROS_MASTER_URI=http://192.168.31.3:11311
# miniboat ip address (the raspberry ip address)
export ROS_IP=192.168.31.2
#export ROS_IP=192.168.31.102

# launch ROS packages
roslaunch miniboat_test run.launch

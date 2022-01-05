#!/bin/bash

# NOTE: make sure correct workspace is sourced before calling this script

# HOSTNAME is computer name, i.e., miniboat1, miniboat2
export HOSTNAME

# if no multimaster node is running, link directly to setup ip
export ROS_MASTER_URI=http://192.168.31.102:11311   # user ip address or name (if properly defined in /etc/hosts file)
export ROS_IP=192.168.31.11                         # miniboat ip address (the raspberry ip address)

# launch ROS packages
roslaunch roboat_launch miniboat.launch use_ekf:=true
#!/bin/bash

# NOTE: make sure correct workspace is sourced before calling this script

# HOSTNAME is computer name, i.e., miniboat1, miniboat2
export HOSTNAME

# to specify directly the master URI and IP, otherwise will use http://<$HOSTNAME>:11311
export ROS_MASTER_URI=http://192.168.31.102:11311
export ROS_IP=192.168.1.102

# starts roscore
roscore
## ----------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.3)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)


## ----------------------------------------------------------------------------
project(roboat_msgs)

## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-std=c++14)


## ----------------------------------------------------------------------------
## Catkin macros and libraries
find_package(catkin REQUIRED COMPONENTS
        message_generation
        std_msgs
        geometry_msgs
        nav_msgs
        )


## ----------------------------------------------------------------------------
## Generate messages in the 'msg' folder
add_message_files(
        FILES
        Force.msg
        Heartbeat.msg
        ThrusterCommand.msg
	ThrusterFeedback.msg
        ThrusterForceFeedback.msg
        ThrustState.msg
)

## Generate added messages and services with any dependencies listed here
generate_messages(
        DEPENDENCIES
        std_msgs
        geometry_msgs
        nav_msgs
)


## ----------------------------------------------------------------------------
catkin_package(
        CATKIN_DEPENDS
        message_runtime
)


## ----------------------------------------------------------------------------
## Specify additional locations of header files
include_directories(
        ${catkin_INCLUDE_DIRS}
)

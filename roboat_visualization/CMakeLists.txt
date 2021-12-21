cmake_minimum_required(VERSION 2.8.3)
project(roboat_visualization)

###########
## Build ##
###########

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS "-std=c++14")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -g -pthread")

#############
## Package ##
#############

find_package(catkin REQUIRED COMPONENTS
        tf
        roscpp
        roslib
        rospy
        xacro
        cv_bridge
        message_filters
        # pcl library
        pcl_conversions
        # msgs
        std_msgs
        sensor_msgs
        geometry_msgs
        nav_msgs
        visualization_msgs
        # roboat
        roboat_core
        roboat_msgs
        )

find_package(PCL REQUIRED)
find_package(OpenMP REQUIRED)
find_package(OpenCV REQUIRED)

############
## Catkin ##
############

catkin_package(
        INCLUDE_DIRS
        include
        DEPENDS PCL GTSAM
        CATKIN_DEPENDS
        std_msgs
        nav_msgs
        geometry_msgs
        sensor_msgs
        visualization_msgs
)

# include directories
include_directories(
        include
        ${catkin_INCLUDE_DIRS}
        ${PCL_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
        ${catkin_INCLUDE_DIRS}
)

# link directories
link_directories(
        include
        ${PCL_LIBRARY_DIRS}
        ${OpenCV_LIBRARY_DIRS}
)

###########
## Build ##
###########

# Data Modify
add_executable(visualization_node src/roboat_visualization.cpp)
add_dependencies(visualization_node ${catkin_EXPORTED_TARGETS})
target_link_libraries(visualization_node ${catkin_LIBRARIES})

# Mark cpp header files for installation
install(DIRECTORY include/${PROJECT_NAME}/
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
        FILES_MATCHING PATTERN "*.h"
        )

install(DIRECTORY launch meshes
        DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
        )

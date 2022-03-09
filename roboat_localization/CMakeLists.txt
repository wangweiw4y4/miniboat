cmake_minimum_required(VERSION 2.8.3)
project(roboat_localization)

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
        # msgs
        std_msgs
        sensor_msgs
        geometry_msgs
        nav_msgs
        visualization_msgs
        # roboat
        roboat_core
        roboat_msgs
        # sensors
        marvelmind_nav
        )

## System dependencies are found with CMake's conventions
find_package (Eigen3 REQUIRED)
find_package(OpenMP REQUIRED)
find_package(OpenCV REQUIRED)

############
## Catkin ##
############

catkin_package(
        INCLUDE_DIRS
        include
        # DEPENDS 
        CATKIN_DEPENDS
        std_msgs
        nav_msgs
        geometry_msgs
        sensor_msgs
        visualization_msgs
        roboat_core
        marvelmind_nav
)

# include directories
include_directories(
        include
        ${catkin_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIRS}
)

# link directories
link_directories(
        include
        ${OpenCV_LIBRARY_DIRS}
        ${EIGEN3_INCLUDE_DIRS}
)

###########
## Build ##
###########

# Data Modify
add_executable(data_filter_node  src/data_filter_node.cxx  src/data_filter.cpp)
add_dependencies(data_filter_node  ${catkin_EXPORTED_TARGETS})
target_link_libraries(data_filter_node  ${catkin_LIBRARIES})

add_executable(sensor_node  src/sensor_node.cxx)
add_dependencies(sensor_node  ${catkin_EXPORTED_TARGETS})
target_link_libraries(sensor_node  ${catkin_LIBRARIES})

add_executable(test_node  src/test_node.cxx)
add_dependencies(test_node  ${catkin_EXPORTED_TARGETS})
target_link_libraries(test_node  ${catkin_LIBRARIES})

## Mark executables and/or libraries for installation
install(
  TARGETS
  data_filter_node
  sensor_node
  test_node
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

# Mark cpp header files for installation
install(DIRECTORY include/${PROJECT_NAME}/
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
        FILES_MATCHING PATTERN "*.h"
        )

install(DIRECTORY include/${PROJECT_NAME}/
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
        FILES_MATCHING PATTERN "*.hpp"
        )

install(DIRECTORY config launch urdf
        DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
        )

cmake_minimum_required(VERSION 2.8.3)
project(miniboat_test)

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -g -pthread")

find_package(catkin REQUIRED COMPONENTS
  tf
  roscpp
  rospy
  cv_bridge
)

catkin_package(
  INCLUDE_DIRS include
  CATKIN_DEPENDS 
  std_msgs
)

# include directories
include_directories(
	include
	${catkin_INCLUDE_DIRS}
)

# link directories
link_directories(
	include
)

###########
## Build ##
###########

add_executable(miniboat_test src/main.cpp)
target_link_libraries(miniboat_test ${catkin_LIBRARIES})
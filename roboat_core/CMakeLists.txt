cmake_minimum_required(VERSION 3.0.2)
set( CMAKE_EXPORT_COMPILE_COMMANDS 1 )
project(roboat_core)

## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-std=c++11)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS
  message_generation
  message_runtime
  roscpp
  roslib
  roslint
  serial
  std_msgs
  sensor_msgs
  geometry_msgs
  roboat_msgs
  tf
)

find_package (Eigen3 REQUIRED)

set(CASADI_DIR "/usr/local")
set(CMAKE_PREFIX_PATH ${CASADI_DIR} ${CMAKE_PREFIX_PATH})
set(PKG_CONFIG_PATH ${CASADI_DIR}/lib/pkgconfig ${PKG_CONFIG_PATH})
find_package(PkgConfig REQUIRED)
pkg_search_module(CASADI casadi REQUIRED)

message("Libraries: ${ACADO_INCLUDE_DIRS}")

## System dependencies are found with CMake's conventions
# find_package(Boost REQUIRED COMPONENTS system)


## Uncomment this if the package has a setup.py. This macro ensures
## modules and global scripts declared therein get installed
## See http://ros.org/doc/api/catkin/html/user_guide/setup_dot_py.html
# catkin_python_setup()

################################################
## Declare ROS messages, services and actions ##
################################################

## To declare and build messages, services or actions from within this
## package, follow these steps:
## * Let MSG_DEP_SET be the set of packages whose message types you use in
##   your messages/services/actions (e.g. std_msgs, actionlib_msgs, ...).
## * In the file package.xml:
##   * add a build_depend tag for "message_generation"
##   * add a build_depend and a run_depend tag for each package in MSG_DEP_SET
##   * If MSG_DEP_SET isn't empty the following dependency has been pulled in
##     but can be declared for certainty nonetheless:
##     * add a run_depend tag for "message_runtime"
## * In this file (CMakeLists.txt):
##   * add "message_generation" and every package in MSG_DEP_SET to
##     find_package(catkin REQUIRED COMPONENTS ...)
##   * add "message_runtime" and every package in MSG_DEP_SET to
##     catkin_package(CATKIN_DEPENDS ...)
##   * uncomment the add_*_files sections below as needed
##     and list every .msg/.srv/.action file to be processed
##   * uncomment the generate_messages entry below
##   * add every package in MSG_DEP_SET to generate_messages(DEPENDENCIES ...)

## Generate messages in the 'msg' folder
add_message_files(
  FILES
  Force.msg
  State.msg
  CommandToMicroController.msg
  SensorFromMicroController.msg
  FloatArray.msg  
)

## Generate services in the 'srv' folder
# add_service_files(
#   FILES
#   Service1.srv
#   Service2.srv
# )

## Generate actions in the 'action' folder
# add_action_files(
#   FILES
#   Action1.action
#   Action2.action
# )

## Generate added messages and services with any dependencies listed here
generate_messages(
 #DEPENDENCIES
 #std_msgs
)

################################################
## Declare ROS dynamic reconfigure parameters ##
################################################

## To declare and build dynamic reconfigure parameters within this
## package, follow these steps:
## * In the file package.xml:
##   * add a build_depend and a run_depend tag for "dynamic_reconfigure"
## * In this file (CMakeLists.txt):
##   * add "dynamic_reconfigure" to
##     find_package(catkin REQUIRED COMPONENTS ...)
##   * uncomment the "generate_dynamic_reconfigure_options" section below
##     and list every .cfg file to be processed

## Generate dynamic reconfigure parameters in the 'cfg' folder
# generate_dynamic_reconfigure_options(
#   cfg/DynReconf1.cfg
#   cfg/DynReconf2.cfg
# )

###################################
## catkin specific configuration ##
###################################
## The catkin_package macro generates cmake config files for your package
## Declare things to be passed to dependent projects
## INCLUDE_DIRS: uncomment this if you package contains header files
## LIBRARIES: libraries you create in this project that dependent projects also need
## CATKIN_DEPENDS: catkin_packages dependent projects also need
## DEPENDS: system dependencies of this project that dependent projects also need
catkin_package(
  INCLUDE_DIRS include
  LIBRARIES lib_VFF
#  LIBRARIES roboat_core
  CATKIN_DEPENDS 
  geometry_msgs
  message_runtime
  roscpp
  sensor_msgs
  std_msgs
)

###########
## Build ##
###########

## Specify additional locations of header files
## Your package locations should be listed before other locations


include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIRS}
  codegen_solver
  qpoases
  qpoases/INCLUDE
  qpoases/SRC
)

## Declare a C++ library
add_library(
  acado_mpc
  codegen_solver/acado_auxiliary_functions.c
  codegen_solver/acado_integrator.c
  codegen_solver/acado_qpoases_interface.cpp
  codegen_solver/acado_qpoases_interface.hpp
  codegen_solver/acado_solver.c
  codegen_solver/acado_solver_sfunction.c
  qpoases/SRC/Bounds.cpp
  qpoases/SRC/Constraints.cpp
  qpoases/SRC/CyclingManager.cpp
  qpoases/SRC/Indexlist.cpp
  qpoases/SRC/MessageHandling.cpp
  qpoases/SRC/QProblem.cpp
  qpoases/SRC/QProblemB.cpp
  qpoases/SRC/SubjectTo.cpp
  qpoases/SRC/Utils.cpp
  qpoases/SRC/EXTRAS/SolutionAnalysis.cpp
)

add_library(lib_VFF
  include/VFF/VirtualForceField.cpp
  include/VFF/VirtualForceField.h
)
target_link_libraries(lib_VFF
  ${catkin_LIBRARIES}
)

## Add cmake target dependencies of the library
## as an example, code may need to be generated before libraries
## either from message generation or dynamic reconfigure
# add_dependencies(${PROJECT_NAME} ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## Declare a C++ executable
## With catkin_make all packages are built within a single CMake context
## The recommended prefix ensures that target names across packages don't collide
add_executable(command_node             src/command_node.cpp               src/Command.cpp)
add_executable(csv_path_node            src/csv_path_node.cpp              src/CsvPath.cpp      src/Path.cpp)
add_executable(pose_path_node           src/pose_path_node.cpp             src/PosePath.cpp     src/Path.cpp)
add_executable(mpc_node                 src/mpc_node.cpp                   src/MPC.cpp)
add_executable(pid_node                 src/pid_node.cpp)
add_executable(state_node               src/state_node.cpp)
add_executable(pf_node                  src/potential_field_node.cxx       src/potential_field.cpp   src/swarm.cpp)       
add_executable(serial_node              src/serial_node.cpp)            
add_executable(sim_node                 src/sim_node.cpp                   src/Sim.cpp)       
add_executable(swarm_node               src/swarm_node.cxx                 src/swarm.cpp)        
add_executable(capacitive_node          src/capacitive_node.cpp)
add_executable(thruster_calibration_node                                   src/thruster_calibration_node.cpp)
add_executable(thruster_multi_calibration_node                             src/thruster_multi_calibration_node.cpp)
add_executable(pf_vel_node                  src/potential_field_vel_node.cxx       src/potential_field_vel.cpp   src/swarm.cpp)       


## Rename C++ executable without prefix
## The above recommended prefix causes long target names, the following renames the
## target back to the shorter version for ease of user use
## e.g. "rosrun someones_pkg node" instead of "rosrun someones_pkg someones_pkg_node"
# set_target_properties(${PROJECT_NAME}_node PROPERTIES OUTPUT_NAME node PREFIX "")

## Add cmake target dependencies of the executable
## same as for the library above
add_dependencies(serial_node                 ${PROJECT_NAME}_generate_messages_cpp)
add_dependencies(mpc_node                    ${PROJECT_NAME}_generate_messages_cpp acado_mpc)
add_dependencies(pid_node                    ${PROJECT_NAME}_generate_messages_cpp)
add_dependencies(state_node                  ${PROJECT_NAME}_generate_messages_cpp)
add_dependencies(pf_node                     ${PROJECT_NAME}_generate_messages_cpp)
add_dependencies(command_node                ${PROJECT_NAME}_generate_messages_cpp)
add_dependencies(sim_node                    ${PROJECT_NAME}_generate_messages_cpp)
add_dependencies(swarm_node                  ${PROJECT_NAME}_generate_messages_cpp)
add_dependencies(capacitive_node             ${PROJECT_NAME}_generate_messages_cpp)
add_dependencies(thruster_calibration_node   ${PROJECT_NAME}_generate_messages_cpp)
add_dependencies(thruster_multi_calibration_node   ${PROJECT_NAME}_generate_messages_cpp)
add_dependencies(pf_vel_node                     ${PROJECT_NAME}_generate_messages_cpp)


## Specify libraries to link a library or executable target against
target_link_libraries(serial_node                ${catkin_LIBRARIES})
target_link_libraries(mpc_node                   ${catkin_LIBRARIES} acado_mpc)
target_link_libraries(pid_node                   ${catkin_LIBRARIES})
target_link_libraries(state_node                 ${catkin_LIBRARIES})
target_link_libraries(pf_node                    ${catkin_LIBRARIES} lib_VFF)
target_link_libraries(sim_node                   ${catkin_LIBRARIES})
target_link_libraries(command_node               ${catkin_LIBRARIES})
target_link_libraries(csv_path_node              ${catkin_LIBRARIES})
target_link_libraries(pose_path_node             ${catkin_LIBRARIES})
target_link_libraries(swarm_node                 ${catkin_LIBRARIES})
target_link_libraries(capacitive_node            ${catkin_LIBRARIES})
target_link_libraries(thruster_calibration_node  ${catkin_LIBRARIES})
target_link_libraries(thruster_multi_calibration_node  ${catkin_LIBRARIES})
target_link_libraries(pf_vel_node                    ${catkin_LIBRARIES} lib_VFF)

#############
## Install ##
#############

# all install targets should use catkin DESTINATION variables
# See http://ros.org/doc/api/catkin/html/adv_user_guide/variables.html

## Mark executable scripts (Python etc.) for installation
## in contrast to setup.py, you can choose the destination
install(PROGRAMS
  scripts/rostopic2csv.sh
  scripts/ndt_config_publish.sh
  DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

## Mark executables and/or libraries for installation
install(
  TARGETS
  acado_mpc
  command_node
  pid_node
  state_node
  csv_path_node
  mpc_node
  pf_node
  pf_vel_node  
  serial_node
  sim_node
  swarm_node
  capacitive_node
  thruster_calibration_node
  thruster_multi_calibration_node
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

#Mark cpp header files for installation
install(
  DIRECTORY include/${PROJECT_NAME}/ include/VFF
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
)

## Mark other files for installation (e.g. launch and bag files, etc.)
install(
  # FILES
  DIRECTORY config/ launch/
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
)

#############
## Testing ##
#############

## Add gtest based cpp test target and link libraries
# catkin_add_gtest(${PROJECT_NAME}-test test/test_my_serial_node.cpp)
# if(TARGET ${PROJECT_NAME}-test)
#   target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})
# endif()

## Add folders to be run by python nosetests
# catkin_add_nosetests(test)

roslint_cpp()

add_executable(position_pid src/position_pid.cpp)
target_link_libraries(position_pid ${catkin_LIBRARIES})
add_dependencies(position_pid      ${PROJECT_NAME}_generate_messages_cpp)

add_executable(velocity_pid src/velocity_pid.cpp)
target_link_libraries(velocity_pid ${catkin_LIBRARIES})
add_dependencies(velocity_pid      ${PROJECT_NAME}_generate_messages_cpp)


add_executable(velocity_smc src/velocity_smc.cpp)
target_link_libraries(velocity_smc ${catkin_LIBRARIES})
add_dependencies(velocity_smc      ${PROJECT_NAME}_generate_messages_cpp)


add_executable(velocity_st src/velocity_st.cpp)
target_link_libraries(velocity_st ${catkin_LIBRARIES})
add_dependencies(velocity_st      ${PROJECT_NAME}_generate_messages_cpp)


add_executable(velocity_asmc src/velocity_asmc.cpp)
target_link_libraries(velocity_asmc ${catkin_LIBRARIES})
add_dependencies(velocity_asmc      ${PROJECT_NAME}_generate_messages_cpp)




add_executable(velocity_cascade_pid src/velocity_cascade_pid.cpp)
target_link_libraries(velocity_cascade_pid ${catkin_LIBRARIES})
add_dependencies(velocity_cascade_pid      ${PROJECT_NAME}_generate_messages_cpp)



add_executable(velocity_cascade_smc src/velocity_cascade_smc.cpp)
target_link_libraries(velocity_cascade_smc ${catkin_LIBRARIES})
add_dependencies(velocity_cascade_smc      ${PROJECT_NAME}_generate_messages_cpp)


add_executable(velocity_cascade_asmc src/velocity_cascade_asmc.cpp)
target_link_libraries(velocity_cascade_asmc ${catkin_LIBRARIES})
add_dependencies(velocity_cascade_asmc      ${PROJECT_NAME}_generate_messages_cpp)

link_directories(${CASADI_LIBRARY_DIRS})
add_executable(miniboat_admm src/miniboat_admm.cpp src/swarm_admm.cpp)
target_link_libraries(miniboat_admm ${catkin_LIBRARIES} ipopt ${CASADI_LIBRARIES})
target_include_directories(miniboat_admm PUBLIC ${CASADI_INCLUDEDIR} ${EIGEN_INC})
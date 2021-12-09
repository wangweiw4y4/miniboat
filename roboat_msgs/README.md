# roboat_msgs
The code repository for the ROS messages defnitions used to communicate between roboat software packages

## Generating header defitions for lower level code
Roboat msgs contains all the msgs used in roboat control. As we are interacting with embedded devices we also need to generate header files usable for these devices. This is done using the rosserial_client.

  rosrun rosserial_client make_libraries src/roboat/roboat_common/roboat_msgs/built_headers/

The lower level devices such as the esp32 and arduino should then reference these header files in their code. Ideally this step should be a post build process in cmake.
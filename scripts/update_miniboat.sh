#! /bin/bash
trap "exit" INT # make it killable

# MINIBOAT SOFTWARE UPDATER
YELLOW='\033[1;33m'
NC='\033[0m' # No Color
echo -e "${YELLOW}MINIBOAT SOFTWARE UPDATER${NC}"

## ------------------------------------------------------------------------------------------
#NOTE: alternatively, call the installation script directly to make sure that 
#linux installation is up to date:
# SCRIPT_PATH="$(pwd + dirname "$0")"
# cd SCRIPT_PATH
# ./install_ros-miniboat.sh 

## CATKIN & MINIBOAT WORKSPACE ---------------------
# it is assume that the software is properly installed (see install_ros-miniboat.sh script)
SCRIPT_PATH="$(pwd + dirname "$0")"
# echo $SCRIPT_PATH
MINIBOAT_SRC_PATH="$SCRIPT_PATH/../"
cd $MINIBOAT_SRC_PATH ; 
if [ `basename $PWD` = 'src' ]; then
    #miniboat packages directly in src folder
    CATKIN_WS="$SCRIPT_PATH/../../"
else
    #miniboat packages in dedicated /src/miniboat folder
    CATKIN_WS="$SCRIPT_PATH/../../../"
fi
# echo $CATKIN_WS
# echo $MINIBOAT_SRC_PATH

# ###################################################################################
# pulls from repository the latest version
cd $MINIBOAT_SRC_PATH ;
git pull

# ###################################################################################
#Build, compile, and install catkin workspace
cd $CATKIN_WS 
rosdep install --from-paths src --ignore-src -r -y  # Install any missing packages
catkin_make 
# catkin_make install

# recompile message headers for MCU uploads
source devel/setup.bash
rm -r $MINIBOAT_SRC_PATH/roboat_msgs/built_headers/ros_lib
rosrun rosserial_client make_libraries $MINIBOAT_SRC_PATH/roboat_msgs/built_headers/

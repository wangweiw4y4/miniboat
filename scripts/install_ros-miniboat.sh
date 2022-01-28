#!/bin/bash
trap "exit" INT # make it killable

# MINIBOAT SOFTWARE INSTALLER
# When installing in the Raspberry Pi, needs load to install after Ubuntu 20.04 Server.
YELLOW='\033[1;33m'
NC='\033[0m' # No Color
echo -e "${YELLOW}MINIBOAT SOFTWARE INSTALLATION${NC}"

# PROCESS INPUTS #####################################################################
# default values
ROS_DISTRO=noetic 

## CATKIN WORKSPACE ---------------------
SCRIPT_PATH="$(pwd + dirname "$0")"
if [ -d "$SCRIPT_PATH/../.git" ]; then  
  #script part of the repo
  MINIBOAT_SRC_PATH="$SCRIPT_PATH/../"
  cd $MINIBOAT_SRC_PATH ;
  if [ `basename $PWD` = 'src' ]; then
    #miniboat packages directly in src folder
    CATKIN_WS="$SCRIPT_PATH/../../"
  else
    #miniboat packages in dedicated /src/miniboat folder
    CATKIN_WS="$SCRIPT_PATH/../../../"
  fi
else 
  CATKIN_WS=~/catkin_ws
  MINIBOAT_SRC_PATH=$CATKIN_WS/src/miniboat
fi

CONFIGURE_NETWORK=1
INSTALL_PI=0
INSTALL_LIBRASPBERRYPI=1
INSTALL_OPENCV=1
INSTALL_DESKTOP=1
UPLOAD_ARDUINO=0

# process possible inputs
while :; do
    case $1 in
        # -h|-\?|--help)
        #     show_help    # Display a usage synopsis.
        #     exit
        #     ;;
        -r|--ros-distro)
            if [ "$2" ]; then
                ROS_DISTRO=$2
                shift
            else
                echo "empty --ros-distro argument, using default ${ROS_DISTRO}"
            fi
            ;;
        --ros-distro=?*)
            ROS_DISTRO=${1#*=} # Delete everything up to "=" and assign the remainder.
            ;;
        --ros-distro=) # empty --image=
            echo "empty --ros-distro argument, using default ${ROS_DISTRO}"
            ;;
        -p|--path)
            if [ "$2" ]; then
                CATKIN_WS=$2
                MINIBOAT_SRC_PATH=$CATKIN_WS/src/miniboat
                shift
            else
                echo "empty --path argument, using default ${CATKIN_WS}"
            fi
            ;;
        --path=?*)
            CATKIN_WS=${1#*=} # Delete everything up to "=" and assign the remainder.
            MINIBOAT_SRC_PATH=$CATKIN_WS/src/miniboat
            ;;
        --path=) # empty --image=
            echo "empty --path argument, using default  ${CATKIN_WS}"
            ;;
        --pi) # 
            INSTALL_PI=1  
            ;;
        -l|--libraspberrypi)
            INSTALL_LIBRASPBERRYPI=1
            ;;
        -o|--open-cv) # 
            INSTALL_OPENCV=1  
            ;;
        -d|--desktop)
            INSTALL_DESKTOP=1
            ;;
        -n|--network)
            CONFIGURE_NETWORK=1
            ;;
        -a|--arduino)
            UPLOAD_ARDUINO=1
            ;;
        --)  # End of all options.
            shift
            break
            ;;
        -?*)
            printf 'WARN: Unknown option (ignored): %s\n' "$1" >&2
            ;;
        *)               # Default case: No more options, so break out of the loop.
            break
    esac 
    shift
done

# Makes sure that CATKIN_WS is an absolute path from here on
mkdir -p $CATKIN_WS | exit
cd $CATKIN_WS ; CATKIN_WS=$PWD
# echo $CATKIN_WS

# Checks that it has access to github, since this requires user input so better do it first
git ls-remote -h -q https://github.mit.edu/SenseableCityLab/miniboat.git
# it will prompt a message asking for username and password(token), and print the refs/heads

# UBUNTU CONFIGURATION TOOLS ##############################################################
sudo apt-get update
sudo apt-get upgrade -y

if [[ INSTALL_PI -eq 1 ]]; then
    #raspi-config
    mkdir -p ~/downloads
    cd ~/downloads
    wget http://mirrors.ustc.edu.cn/archive.raspberrypi.org/debian/pool/main/r/raspi-config/raspi-config_20201108_all.deb  
    #NOTE: there might be newer versions in the website, check web
    sudo apt install -y lua5.1  libatopology2 libfftw3-single3 libsamplerate0 alsa-utils
    sudo dpkg -i raspi-config_20201108_all.deb
    # NOTE: To provide access to the camera, need to open the raspi-config tools and activate it. 
    #       Done at the end of the installation script. Read install_pi.md file for further instructions.

    sudo apt-get install -y v4l-utils

    #To use the camera using raspivid to take short videos and other pi utilities
    if [[ INSTALL_LIBRASPBERRYPI -eq 1 ]]; then
        echo "INSTALLING LIBRASPBERRYPI"
        sudo apt install -y libraspberrypi-bin
    fi
    # NOTE: This library is only available for ARM and aarhf architectures, not x86.


    if [[ INSTALL_OPENCV -eq 1 ]]; then
        echo "INSTALLING OPENCV"
        #Explore here a way to automatically assign the GPU memory, spceified 
        #in the gpu_mem variable at /boot/firmware/config.txt
        sudo apt install -y libopencv-dev python3-opencv
    fi
    # NOTE: For more control over the installation options, OpenCV can be instaled directly from
    #       from source . Check install_pi.md for more info.


    #to have X11 forwarding so we can check the screen in the remote computer
    sudo apt-get install -y x11-apps 


    if [[ INSTALL_DESKTOP -eq 1 ]]; then
        echo "INSTALLING DESKTOP"
        sudo apt install -y lubuntu-desktop
        sudo echo "dtoverlay=vc4-fkms-v3d" >> /boot/firmware/syscfg.txt
        #once desktop install, load VSCode and Terminator to edit and run the code 
    else 
        # base desktop is very limited, starts by default in terminal mode:
        sudo systemctl set-default multi-user.target
    fi
fi


# INSTALL AND CONFIGURE ROS ##############################################################
#We check if ROS is already installed by evaluating if the ros distro is already installed
if [ ! -f "/opt/ros/$ROS_DISTRO/setup.bash" ]; then
    echo "INSTALLING ROS"
    sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
    sudo apt install -y curl 
    curl -s https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | sudo apt-key add -
    sudo apt update
    if [[ INSTALL_DESKTOP -eq 1 ]]; then
        sudo apt install -y ros-$ROS_DISTRO-desktop
    else 
        sudo apt install -y ros-$ROS_DISTRO-ros-base
        sudo apt install -y ros-$ROS_DISTRO-rosserial //not included in ros-base
    fi
    sudo apt-get install -y ros-$ROS_DISTRO-xacro
    source /opt/ros/$ROS_DISTRO/setup.bash
    echo "source /opt/ros/$ROS_DISTRO/setup.bash" >> ~/.bashrc
    source ~/.bashrc
    sudo apt install -y python3-rosdep python3-rosinstall python3-rosinstall-generator python3-wstool build-essential
    sudo rosdep init
else
    #ROS already installed, just makes sure it is up to date and sourced
    if [[ INSTALL_DESKTOP -eq 1 ]]; then
        sudo apt-get install -y ros-$ROS_DISTRO-desktop
    else 
        sudo apt-get install -y ros-$ROS_DISTRO-ros-base
    fi
    sudo apt-get install -y ros-$ROS_DISTRO-xacro
    grep -qxF "source /opt/ros/$ROS_DISTRO/setup.bash" ~/.bashrc ||  echo "source /opt/ros/$ROS_DISTRO/setup.bash" >> ~/.bashrc
    source ~/.bashrc 
    sudo apt install -y python3-rosdep python3-rosinstall python3-rosinstall-generator python3-wstool build-essential
fi
rosdep update  

#install multimaster 
if [[ ${ROS_DISTRO} == "noetic" ]]; then
    sudo apt-get install -y ros-noetic-fkie-multimaster
else
    sudo apt-get install -y ros-$ROS_DISTRO-multimaster-fkie
fi


# Create main directories ----------
mkdir -p $CATKIN_WS/src
mkdir -p $CATKIN_WS/bags
mkdir -p $CATKIN_WS/logs
mkdir -p $CATKIN_WS/repos


# LOAD MINIBOATS #####################################################################
if [ ! -d "$MINIBOAT_SRC_PATH" ]; then
    cd $CATKIN_WS/src
    git clone https://github.mit.edu/SenseableCityLab/miniboat.git
else
    cd $MINIBOAT_SRC_PATH
    git pull
fi

# Install external package dependencies not included in the miniboat repo ----------
#
# ACADO TOOLKIT 
if ! grep -qxF "ACADOtoolkit/build/acado_env.sh" ~/.bashrc; then
    echo "Installing Acado Toolkit"
    cd $CATKIN_WS/src
    sudo apt-get install gcc g++ cmake git gnuplot doxygen graphviz
    if [ ! -d "$CATKIN_WS/src/ACADOtoolkit/" ]; then        
        git clone https://github.com/acado/acado.git -b stable ACADOtoolkit
    fi
    cd ACADOtoolkit
    mkdir build
    cd build || exit
    cmake ..
    make
    sudo make install -j4
    sudo ldconfig
    SETUP_PATH=$CATKIN_WS/src/ACADOtoolkit/build/acado_env.sh
    grep -qxF "source $SETUP_PATH" ~/.bashrc || echo "source $SETUP_PATH" >> ~/.bashrc
fi

# (serial package already included as part of miniboat packages)
# if [ ! -d "$CATKIN_WS/src/serial/" ]; then
#     cd $CATKIN_WS/src
#     git clone https://github.com/wjwwood/serial.git  
# fi



# CONFIGURE NETWORK #################################################################
if [[ CONFIGURE_NETWORK -eq 1 ]]; then
    cd $SCRIPT_PATH
    sudo ./setup_network.sh
fi


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

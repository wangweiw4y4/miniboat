#!/bin/bash
trap "exit" INT # make it killable

# Installs the roboat latching software in the Raspberry Pi, after Ubuntu 20.04 Server has been installed 
# and SSH keys to access the Roboat repositories at the SCL Github have been loaded.


# PROCESS INPUTS #####################################################################
# default values
ROS_DISTRO=noetic 
CATKIN_WS=~/catkin_ws 

CONFIGURE_NETWORK=0
INSTALL_LIBRASPBERRYPI=1
INSTALL_OPENCV=1
INSTALL_DESKTOP=0
UPLOAD_ARDUINO=0
# process possible inputs
while :; do
    case $1 in
        # -h|-\?|--help)
        #     show_help    # Display a usage synopsis.
        #     exit
        #     ;;
        -o|--open-cv) # creates ROS framework, make sure it does not exist already as the script will fail otherwise
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
        -l|--libraspberrypi)
            INSTALL_LIBRASPBERRYPI=1
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
mkdir -p $CATKIN_WS
cd $CATKIN_WS
CATKIN_WS=$PWD
echo $CATKIN_WS

# Checks that it has access to github, since this requires user input so better do it first
git ls-remote -h -q https://github.mit.edu/SenseableCityLab/roboat
# it will prompt a message asking for username and password(token), and print the refs/heads

# UBUNTU CONFIGURATION TOOLS ##############################################################
sudo apt-get update
sudo apt-get upgrade -y

mkdir -p ~/downloads
cd ~/downloads

#raspi-config
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
else 
    # base desktop is very limited, starts by default in terminal mode:
    sudo systemctl set-default multi-user.target
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


# Install base Roboat packages ----------
mkdir -p $CATKIN_WS/src
mkdir -p $CATKIN_WS/bags
mkdir -p $CATKIN_WS/logs
mkdir -p $CATKIN_WS/repos


# rosserial seems to be finally added to the noetic repository, but we still need to clone serial
if [ ! -d "$CATKIN_WS/src/serial/" ]; then
    cd $CATKIN_WS/src
    #git clone https://github.com/ros-drivers/rosserial.git -b noetic-devel
    git clone https://github.com/wjwwood/serial.git  #used by roboat_control to compile from source the serial package
fi



# CONFIGURE NETWORK #################################################################
if [[ CONFIGURE_NETWORK -eq 1 ]]; then
    echo "network configuration pending to implement"
fi


# LOAD MINIBOATS #####################################################################


#Build, compile, and install catkin workspace
cd $CATKIN_WS 
rosdep install --from-paths src --ignore-src -r -y  # Install any missing packages
catkin_make_isolated 
catkin_make_isolated --install

# recompile message headers for MCU uploads
source devel_isolated/setup.bash
rm -r src/roboat_common/roboat_msgs/built_headers/ros_lib
rosrun rosserial_client make_libraries src/roboat_common/roboat_msgs/built_headers/

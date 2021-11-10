# roboat_core

The older version of documentation can be found here: http://www.mit.edu/~roboat (authentication required)

## Package introduction

This package mainly includes following C/C++ function files:
- ```MPC```: it provides trajectory tracking ability of roboat
- ```Command```: it priortites multiple command sources (MPC, latching, and joystick, etc.) and choose the highest one to send to the thrusters.  
- ```pid```: it provides latching actions
- ```serial```: it convert the force commands into bytes and send it to thruster driver (STM32) through serial 232 
- ```pressure_sensor```: it reads data from the pressure sensor units


## 1. Install dependencies

Several external non-ROS packages need to be installed first before compiling this package. Please follow the instructions below to install these dependecies. 

**1.1 Install ROS**
```
sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
sudo apt-key adv --keyserver hkp://ha.pool.sks-keyservers.net:80 --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654
sudo apt-get update
sudo apt-get install -y ros-kinetic-desktop-full
sudo rosdep init
rosdep update
sudo apt-get install -y python-rosinstall python-rosinstall-generator python-wstool build-essential
```

**1.2 Install rosserial packages**
```
cd ~/catkin_ws/src
git clone https://github.com/ros-drivers/rosserial.git
```
Note that in ros moledic, you need to use catkin_make_isolated to compile. I do not know why it has this bug, and it can compile directly in ros kinetic before.

**1.3 Install serial package**
```
cd ~/catkin_ws/src
git clone https://github.com/wjwwood/serial.git
```
**1.4 Install Eigen**
```
wget -O ~/Downloads/eigen.deb "http://ftp.br.debian.org/debian/pool/main/e/eigen3/libeigen3-dev_3.3.7-1_all.deb"
cd ~/Downloads && sudo gdebi eigen.deb
```
## 2. Install ACADO

**2.1 Install the prerequisites**
```
sudo apt-get install gcc g++ cmake git gnuplot doxygen graphviz
```
**2.2 Download ACADOtoolkit**
```
Git cloning it from the ACADO Github repository or finding it at another source. For the purposes of this tutorial we placed it in our home directory although you should be able to place it elsewhere.
 cd ~
 git clone https://github.com/acado/acado.git -b stable ACADOtoolkit
```
**2.3 create a build directory in the ACADOtoolkit directory**
 ```
 cd ACADOtoolkit
 mkdir build        
 cd build
```
**2.4. make and install ACADO**
 ```
cmake ..
make
sudo make install
sudo ldconfig
```
**2.5 Source ACADOtoolkit/build/acado_env.sh in your .bashrc** 
If ACADOtoolkit was placed somewhere other than your home directory accordingly modify the bellow statement.
```
echo source '$HOME'/ACADOtoolkit/build/acado_env.sh' >> ~/.bashrc
```
## 3 Include ACADO in ROS

**3.1 copy FindACADO.cmake to the relevant ROS package directory from ACADOtoolkit/Cmake**
```
cp ACADOtoolkit/cmake/FindACADO.cmake ROS_PACKAGE_PACK/FindACADO.cmake
```
**3.2 Help catkin use FindACADO.cmake by adding the following line under other find_package commands in the relevant CMakeLists.txt file**
```
find_package(ACADO REQUIRED)
```
**3.3 Add ${ACADO_INCLUDE_DIRS} to the include directories statement in your CMakeLists.txt file.** 
The resulting include_directories statement should look similar to the following
```
include_directories(include

        ${ACADO_INCLUDE_DIRS}

        ${catkin_INCLUDE_DIRS}

        )
```
**3.4 For each of the ROS nodes that include ACADO add ACADO to its target_link_libraries.** 

The resulting target_link_libraries statements should look similar to the following
```       
target_link_libraries(NODE_NAME ${ACADO_SHARED_LIBRARIES} ${catkin_LIBRARIES})
```

**3.5 Include ACADO in the relevant ROS .cpp files by adding the following near the top**
```     
#include <acado_optimal_control.hpp>
```
**3.6. Make your catkin workspace at its root and verify that it builds successfully**
```
catkin_make
```
As a note, the ACADO Examples can be replicated in the ROS environment by directly copying them into a .cpp file in your ROS package source, making them executable from your CMakeLists.txt, following the above amendments in the CMakeLists.txt file, and starting the node using rosrun.

 

**3.7 references**
```
1. https://acado.github.io/install_linux.html

2. https://sourceforge.net/p/acado/wiki/Using%20CMake%20-%20UNIX%20-%20Common/

3. http://wiki.ros.org/catkin/CMakeLists.txt

4. http://acado.sourceforge.net/doc/html/d9/d65/example_001.html
 
```

## 4. How to compile

Download the repository to your ROS workspace and compile using the following commands:
```
cd ~/catkin_ws/src
git clone https://github.mit.edu/SenseableCityLab/roboat_core.git
cd ..
catkin_make
```



## Copyright
```
Copyright 2020 Roboat, all rights reserved.
Private software. No distribution allowed.


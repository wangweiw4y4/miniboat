# Miniboat: DEVELOPMENT NOTES

## SETUP MINIBOAT ROS FRAMEWORK
### Fixes to the image so it can update/upgrade:

1. Go to 
```
cd /etc/apt/sources.list.d
``` 
and  change in `gnome-terminator-ubuntu-ppa-focal.list` and `gnome-terminator-ubuntu-ppa-focal.list.save` the distribution from `focal` to `xenial`

2. Remove sublime package distribution, if it is there
```
sudo apt remove sublime-text
```

3. Load the keys for ROS, as explained in [ROS documentation](http://wiki.ros.org/noetic/Installation/Ubuntu). They have changed them recently, so need to be reload):
```
curl -s https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | sudo apt-key add -
```
4. At this point, you should be able to run ```sudo apt-get update``` with no errors. It is in fact convenient to update the Pi, but this step takes a lof of time and can be skipped. If you want to do so, 
```
sudo apt-get update
sudo agt-get upgrade 
```

5. Install chrony
```
sudo apt install chrony
```

6. Add the server as a time source, so Pi clock is synchronized even if there is no internet connection. To do so, add in the chrony configuration file `/etc/chrony/chrony.conf`, the following line
```
server 192.168.31.102  iburst
``` 
where the different sources are list (I think the first source is `pool ntp.ubuntu.com    iburst maxsources 4`). 
Regarding the server itself, it is already configured but if you want to add an aditional laptop to act as a time server, you need to add at the end of that same `chrony.conf` file:
```
# Step the system clock instead of slewing it if the adjustment is larger than
# one second, but only in the first three clock updates.
makestep 1 3

# Allow NTP client access from local network.
allow 192.168.31.0/24

# This directive enables the server to act as if it synchronized to a real-time
# clock even if not
local stratum 10
```

7. Enable ip_forwarding and multicast for multimaster node, for which needs to edit the file `/etc/sysctl.conf' by uncommenting the line 
```
net.ipv4.ip_forward = 1
```
and adding below it
```
net.ipv4.icmp_echo_ignore_broadcasts=0
```

8. Edit the `/etc/hosts` file adding the IP and name of the different components of the miniboat swarm (server, miniboat3, ...). You can write them manually or run the script in `~/catkin_ws/src/miniboat/scripts/setup_network.sh` to do it automatically. If running the script, double check that it is up to date and has written the ips and names of all miniboats in the swarm.

9. The existing miniboat image already has ROS install. If for whatever reason it doesn't, follow the instructions in the (ROS documentation)[http://wiki.ros.org/noetic/Installation/Ubuntu] to do so. In any case, make sure that the rosdep installation packages are installed, so you can update rosdep:
```
sudo apt install python3-rosdep python3-rosinstall python3-rosinstall-generator python3-wstool build-essential
rosdep update
```

10. Install xacro and multimaster packages, which are not specified as dependencies in miniboat packages
```
sudo apt-get install -y ros-noetic-xacro
sudo apt-get install -y ros-noetic-fkie-multimaster
```

10. If ACADO toolkit has not been installed in the Pi, do so. Check the instructions to do so in the README file of the miniboat project, or in the installation script.

11. Create the catkin workspace and clone the project.
```
cd ~
mkdir -p catkin_ws/src
cd catkin_ws/src
git clone https://github.mit.edu/SenseableCityLab/miniboat.git
```
For future updates, if you want to only use your login credentials once, go to the repo folder and instruct git to store your credentials
```
cd ~/catkin_ws/src/miniboat
git config --global credential.helper store
```

12. Go the catkin workspace and update all the package dependencies needed by the miniboat
```
cd ~/catkin_ws
rosdep install --from-paths src --ignore-src -r -y 
```

13. At this point, you should be able to compile the project :)
```
catkin_make
```
If you are using an arduino and you need to use customized messages, you'll need to build the message headers explicitly to be linked by arduino. 
```
cd ~/catkin_ws
rm -r ~/catkin_ws/src/miniboat/roboat_msgs/built_headers/ros_lib
rosrun rosserial_client make_libraries ~/catkin_ws/src/miniboat/roboat_msgs/built_headers/
```
Note also that you will need to do this every time you add and/or change a message definition that is used by the arduino.

14. Add to the end of `~/.bashrc` file the sourcing commands, to avoid requiring to do it everytime we open a terminal:
```
source ~/catkin_ws/build/setup.bash
export HOSTNAME
```
Note that it should already have the instructions to load ROS and ACADOtoolkit (assumed in the instruction below that is installed in the `catkin_ws/src`, but can be in any other directory)
```
source /opt/ros/noetic/setup.bash
source ~/catkin_ws/roboat_ws/src/miniboat/ACADOtoolkit/build/acado_env.sh
```


### Add a new miniboat to the swarm
In order to add a new miniboat to the swarm so it can communicate with the rest of the miniboats, you need to do in **all the miniboats** of the swarm:
1. Add in the `/etc/hosts` file the miniboat ip and name
2. Add in the swarm configuration file, located in `~/catkin_ws/src/miniboat/roboat_core/config/swarm.yaml` the new miniboat name



### Sensor fusion
Tixiao used a ekf_localization_node from existing robot_localization toolkit to do the ekf filtering, so no filter really on our side. Check [link](http://docs.ros.org/en/melodic/api/robot_localization/html/state_estimation_nodes.html). The only thing that the data filter does is remapping the proper info to the right topic names. The kalman publishes the transform of odom to base_link, then the dataFilter publishes info from map to odom, so all is linked in a way. 


## INSTALL SCRIPT
There is an installation script in `miniboat/scripts/install_ros-miniboat.sh` that contains all the steps needed to configure the raspberry Pi from scratch and load the miniboat software. The steps are very similar to the ones described in detail in the `install_pi.md` file provided in the **roboat_latching** repo. I haven't tried the script for the miniboats, so it will probably require to fix some bugs, but the instructions contained in it are useful to see the instructions needed to install the miniboat software in a new Pi from scratch.


## CONFIGURE ESC
Since we don't have the flight controller, we can use an arduino for the FC passthrough. The BLHeli_32 suite provides a tool to deploy the interface to the arduino. Check:
- You can download the BLHeli_32 suite at [https://www.mediafire.com/folder/dx6kfaasyo24l/BLHeliSuite](https://www.mediafire.com/folder/dx6kfaasyo24l/BLHeliSuite)
- Multiple blogs comment on it, google "connect blheli_32 to ESC through arduino" and can give you a list of blogs discussing about it. The best one that we used to follow the instructions is [https://www.rcgroups.com/forums/showthread.php?3073656-BLHeli-32-ESC-setup-with-Arduino-UNO-R3-%28Racerstar-Tattoo-35A%29](https://www.rcgroups.com/forums/showthread.php?3073656-BLHeli-32-ESC-setup-with-Arduino-UNO-R3-%28Racerstar-Tattoo-35A%29)

Other useful links I've checked:
- [https://forum.arduino.cc/t/controlling-an-esc/261062](https://forum.arduino.cc/t/controlling-an-esc/261062)
- [https://github.com/bitdump/BLHeli/tree/master/BLHeli_32%20ARM](https://github.com/bitdump/BLHeli/tree/master/BLHeli_32%20ARM)
- [https://www.heli-nation.com/tiny-talon-16x16-blheli-32-esc](https://www.heli-nation.com/tiny-talon-16x16-blheli-32-esc) (ESC model used, runs blheli_32 as a controller)
      
Once you connet the BLHeli_32 to the ESC, you can turn off the Beacon_beep, and also adjust the PWM for the thrusters.


## SUBMODULES
When including external packages in the repo, it might be useful to load them as submodules rather than copy/paste them. Check the README in **roboat** repository to see how to do it.

For the miniboat, external packages that can be loaded as references are:
- `dynamic_reconfigure` (to change dynamically parameters in nodes, where is it used?)
- `marvelmind_nav`
- `ros_imu_bno055`
- `serial`


## NETWORK SETUP
### Direct link
To communicate over the network, just make sure we point at the right IP where the roscore is running, both in the master and all the connected ones
```
export ROS_MASTER_URI=http://10.0.0.21:11311
export ROS_IP=10.0.0.5 #(when machine is not reachable by name)
export ROS_HOSTNAME=10.0.0.5 #(when machine doesn't recognize its own name, i.e. running hostname returns localhost rather than the name)
```

### Multimaster (done by default)
Install multimaster:
- from repo:
  - Noetic  `sudo apt-get install ros-noetic-fkie-multimaster`
  - Melodic `sudo apt-get install ros-melodic-multimaster-fkie`
- from source, check [website](https://github.com/fkie/multimaster_fkie/tree/master)
  

To run the multimaster and synchronize the Pi with the central computer, 
```
rosrun master_discovery_fkie master_discovery
rosrun master_sync_fkie master_sync
```

Finally, in order to link properly, the network IP and hostnames of the different machines running ROS needed to be properly saved.


## JOYSTICK
Following the [ROS tutorial](http://wiki.ros.org/joy/Tutorials/ConfiguringALinuxJoystick), to check joystick buttons 
```
sudo jstest /dev/input/jsX
```
where X is the #assigned to the thruster. 

We might get a warning about the joystick force feedback missing, but this is a matter of drivers and still works fine (provides -1 to 1, while `jsdev` gives larger values).


## PI STARTUP
To avoid the need to ssh into each Pi, it is easier if we load ROS automatically on startup, for which we can use the robot_upstart package to set up all files. Check in the **roboat_latching** repository the `install_pi.md` file for more details about how to configure it.

Useful links I read
- [roverrobotics](https://blog.roverrobotics.com/how-to-run-ros-on-startup-bootup/)
- [dexterindustries](https://www.dexterindustries.com/howto/run-a-program-on-your-raspberry-pi-at-startup/)
- [systemd](https://www.freedesktop.org/software/systemd/man/systemd.service.html)


## ARDUINO
To load the ROS library in the arduino for testing without setting up the whole ROS environment, check [link](http://wiki.ros.org/rosserial_arduino/Tutorials/Arduino%20IDE%20Setup)
*Note: Currently you can install the Arduino libaries directly in the Arduino IDE. Just open the Library Manager from the IDE menu in Sketch -> Include Library -> Manage Library. Then search for "rosserial". This is useful if you need to work on an Arduino sketch but don't want to setup a full ROS workstation.

**Arduino does not support float64**, so do not use it for custom messages! Check the Arduino limitations explained by [ROS documentation](http://wiki.ros.org/rosserial/Overview/Limitations)

To load compile and upload code using PlatformIO directly from the Pi, we can install it to run from the terminal.
```
sudo apt-get install python3 python3-pip
sudo python3 -m pip install -U platformio
```

And from the PlatformIO project folder,
- `platformio run` compiles the code
- `platformio run -t upload` compiles and uploads the code
- `platformio device monitor` starts a serial monitor in the terminal


## Other useful commands:
| Command         | Description |
| --------------- | ----------- |
| `ip a`          | check the IP |
| `free -m`       | to check the memory avaiable |
| `sudo reboot`   | whenever needed to reboot the Pi, better than powering off and on |
| `sudo poweroff` | to power off software running, better to do it before unplugging the pi |
| `git submodule update --remote <submodule_id> | to update submodule in metapackage 



## Copyright
```
Copyright 2022 Roboat, all rights reserved.
Private software. No distribution allowed.
```



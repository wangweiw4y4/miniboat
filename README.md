# Project Description
To validate our coordination algorithms in building dynamic infrastructure in large scale, we aim at developing a group of miniboats, which can coordinate to form infrastructures like floating bridges and stages to facilitate temporary activities on the water. 


## Install ACADO
 If one download the miniboat code from github, you need to install the ACADO toolkit. With ACADO toolkit installed, you can tune the MPC parameters as needed. Each time, after you change the MPC parameters, you need to run the following four commands to make the MPC parameters to take effects:
```
catkin_make
roslaunch roboat_acado export_mpc.launch
catkin_make
catkin_make
```
Here are the steps to install ACADO:

1. Install the prerequisites
    .. code-block:: bash

        sudo apt-get install gcc g++ cmake git gnuplot doxygen graphviz

2. Download ACADOtoolkit by git cloning it from the ACADO Github repository or finding it at another source. For the purposes of this tutorial we placed it in our home directory although you should be able to place it elsewhere.
    .. code-block:: bash

        cd ~
        git clone https://github.com/acado/acado.git -b stable ACADOtoolkit

3. create a build directory in the ACADOtoolkit directory
    .. code-block:: bash

        cd ACADOtoolkit
        mkdir build         
        cd build

4. make and install ACADO
    .. code-block:: bash

        cmake ..
        make
        sudo make install
        sudo ldconfig

5. Source ACADOtoolkit/build/acado_env.sh in your .bashrc. If ACADOtoolkit was placed somewhere other than your home directory accordingly modify the bellow statement.
    .. code-block:: bash

        echo 'source '$HOME'/ACADOtoolkit/build/acado_env.sh' >> ~/.bashrc

## Clone and restore an PI image
If we need to produce multiple new miniboats, it is easier to use the image method to make the code ready. Here are the steps to clone and restore a codebase image for PI 4.


**1.1 Clone an Ubuntu Syste to an Image**   

Creating image files only requires one command:
```
sudo dd if=/dev/sdX of=path/to/your-backup.img bs=4K
```

```dd``` is the command, ```if``` is the input, as in the location you want to copy. ```of``` is the output, or the location you're replacing with your copy. ```bs``` is the block size for copy (the default is 512 bytes but doesn’t work well for larger drives).
sdX refers to the drive you are interacting with. Drives are often given a name such as /dev/sda, /dev/sdb, or /dev/sdc. 

**1.2 Resize the image to adapt to smaller size SD card** 

Sometimes the image size is very large, depending on the source of the hard drive or sd card. But most of the space are unused. And also, sometimes we want to copy the image to a smaller sized drive. Here we use PiShrink to automatically shrink a pi image that will then resize to the max size of the SD card on boot. This will make putting the image back onto the SD card faster and the shrunk images will compress better. In addition the shrinked image can be compressed with gzip and xz to create an even smaller image.

PiShrink can be found here: https://github.com/Drewsif/PiShrink

**1.3 Restore an image to SD card**

When restoring from an image file, the same concept applies:
```
dd if=path/to/your-backup.img of=/dev/sdX
```
**1.4 Resize your SD card to full size**

Finally, we can insert the sd card and start the cloned system. As the size of image could be much smaller than the sd card. If one wants to store more data later, we can use the disks tool on ubuntu to resize the partion of the sd card.


## INSTALL & CONFIGURE CSSH

ClusterSSH is a tool that allows to connect simultaneously to a group of machines through SSH, sending the commands typed to all of them at the same time. This is convenient to operate all the miniboats at the same time, when all need to receive the same command. 

To install, simply
```
sudo apt update  
sudo apt install clusterssh
```

To connect to all the miniboats, rather than typing all their addresses, it is simpler to save the cluster definition in the file `/etc/clusters`. If the file doesn't exist, you can create it it with `touch /etc/clusters`. Then, to load the miniboat info, 
```
sudo echo "miniboats ubuntu@192.168.31.14 ubuntu@192.168.31.16 <rest miniboats IPs >" >> /etc/clusters
```

At this point, to launch cssh and connect to the miniboat fleet, simply type `cssh miniboats`. 

CSSH uses Xterm windows, rather simple. In order to visualize it better in screens with high definition, we need to modify the default configuration options as otherwise the text is too small. Two files need to be edited:

1. To adjust the font size, modify the Xterm default configuration options by editing/creating the file `~/.Xresources` so it contains
```
Xft.dpi: 141
xterm*faceName: xft:monospace:pixelsize=30
```
To apply the changes without re-starting the computer, simply type in a terminal `xrdb ~/.Xresources`

2. To adjust the size/position of the terminals opened by CSSH, modify the cssh configuration file located at `~/.clusterssh/config`. All lines are commented with the default values. To adjust 
```
terminal_allow_send_events=-xrm '*.VT100.allowSendEvents:true'
terminal_reserve_bottom=25
terminal_reserve_left=50
terminal_reserve_right=50
terminal_reserve_top=50
terminal_size=300x124
```
Depending on each screen, the values shown above might change. Just play with them until you find a good fit.

For more information, check 
- [linux.die.net](https://linux.die.net/man/1/cssh)
- [linuxquestions.org](https://www.linuxquestions.org/questions/slackware-14/dpi-issues-on-high-resolution-screen-mainly-xterm-4175470553/)
- [unix.stackexchange.com](https://unix.stackexchange.com/questions/219370/larger-xterm-fonts-on-hidpi-displays)
- [linuxreviews.org](https://linuxreviews.org/HOWTO_set_DPI_in_Xorg)


## RUN MINIBOATS

Each of the miniboat runs locally the main software, providing independent and full autonomous control to each unit. In addition, a server laptop acts as a central hub providing the commands that trigger the different control schemes, can configure each unit, and visualizes the miniboat fleet in RVIZ. 

First, after turning on all the miniboats, it is recommended to ensure that their clocks are synchronized. This can be done with
```
cd ~/catkin_ws/src/miniboat/scripts
./sync_clocks.sh
```

Next, to initiate the control software in each miniboat, can be done simultaneously in all the units connecting to all of of them in a separate terminal with `cssh --fillscreen miniboats`. Once logged in, we can initiate the control software within the CSSH environment with
```
cd ~/catkin_ws
source devel/setup.bash
export HOSTNAME
roslaunch roboat_launch miniboat.launch use_ekf:=true use_multimaster:=true
./run_miniboat.sh
```
At this point, all miniboats should be ready to receive instructions and move autonomously. Note that we activate the use of the EKF filter for sensor data fusion, and also use multimaster to establish the communication channels between the miniboats. The latter requires that the file `/etc/hosts` in each miniboat and in the server contains the IP addresses of all the miniboats and the server. If not, they can be added manually in each machine editing that file, or using the provided script `scripts/setup_network.sh`. 

To initiate the fleet visualization, in a new terminal window in the server:
```
cd ~/catkin_ws
source devel/setup.bash
roslaunch roboat_launch run_server.launch use_multimaster:=true
```
This should open a new RVIZ window, and visualize the miniboat fleet positions. 

For manual control of an individual miniboat, we can do it connecting a joystick to the server and launching in a separate window a dedicated joy node
```
cd ~/catkin_ws
source devel/setup.bash
roslaunch roboat_launch run_joy.launch id:=<miniboat_id>
```
where you need to substitute `<miniboat_id>` with the miniboat hostname that want to be controlled.

Similarly, to follow a predefined path loaded as a CSV file, 
```
cd ~/catkin_ws
source devel/setup.bash
roslaunch roboat_launch run_cvs_path.launch id:=<miniboat_id>
```
By default, it launches `miniboat_shapeshift_miniboat4.csv`. For a different path, add as launch parameter `file:=<file_name>`.

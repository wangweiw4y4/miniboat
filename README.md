

#Install ACADO
-------------- 
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


# Miniboat Swarm

## 1. Clone and restore a system image for Raspberry Pi


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



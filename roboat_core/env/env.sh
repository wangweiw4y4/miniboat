use_pool=1
master=1

pool='192.168.31.104'
lab='18.111.9.60'
remote='192.168.31.246'

if [ $use_pool -eq 1 ]; then
    setup=$pool;
else
    setup=$lab;
fi

if [ $master -eq 1 ]; then
    ROS_IP=$setup;
else
    ROS_IP=$remote;
fi

export ROS_IP=$ROS_IP

export ROS_MASTER_URI="http://"$setup":11311"

#!/bin/sh

#We use the server as the time server, so it

# From within each PI, it can be edited from the server directly by 
# using -u is better for standardizing date output and timezones
# sudo date --set="$(ssh ubuntu@server date -u)"


#Loads the date from the server in each of the PIs composing the cluster
# We can get all username@IP from the cluster file by:
# CLUSTER=`sed -n '/^miniboats/ s/miniboats//p' /etc/clusters`  
# echo $CLUSTER
#
# or specify it directly
CLUSTER="ubuntu@192.168.31.14 ubuntu@192.168.31.16 ubuntu@192.168.31.17 ubuntu@192.168.31.18"

# updates date in each Pi through ssh, will ask for ssh password if ssh keys are not set for quick access
for address in $CLUSTER
do
    # ssh $USER@<pi adress> "sudo date --set \"$(date)\""
    echo $address
    # adds 5s timeout to avoid getting stuck too long if PI is not online
    ssh -o ConnectTimeout=5 $address 'sudo date --set="$(date -u)"'
done

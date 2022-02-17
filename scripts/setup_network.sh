#!/bin/sh

# adds/replaces the fixed IPs information assigned to the different ROS hardware

HEADER="### MINIBOAT ###"
FOOTER="#-- MINIBOAT ---"

NET=192.168.31 # ONBOARD NETWORK: 192.168.31.0/24

HOSTS=\
"${NET}.102  server\n\
\n\
${NET}.14  miniboat4\n\
${NET}.16  miniboat6\n\
${NET}.17  miniboat7\n\
${NET}.18  miniboat8\n\
\n\
${NET}.3  david-XPS"

# edits the /etc/hosts file with the ip adresses
if grep -qxF "$HEADER" /etc/hosts; then
    #updates them
    sudo sed -i -n '/'"$HEADER"'/{p;:a;N;/'"$FOOTER"'/!ba;s/.*\n/'"$HOSTS"'\n/};p' /etc/hosts

else
    #adds the hosts
    echo "\n"${HEADER} >> /etc/hosts
    echo ${HOSTS} >> /etc/hosts
    echo ${FOOTER} >> /etc/hosts
fi

# Another alternative is to go hardware by hardware and replace them manually. 
# Needs also to edit the hostname, or is that something that we leave to the user? 
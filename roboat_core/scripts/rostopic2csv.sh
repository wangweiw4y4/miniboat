#!/bin/bash

fileName=$1/$(date +"auto_%Y-%m-%d-%H-%M-%S")
topic=$2
rostopic echo -p ${topic} > ${fileName}_${topic}.csv

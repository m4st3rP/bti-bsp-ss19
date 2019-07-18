#!/bin/sh
bufsize=$1
shift=$2



if [ -z "$1" ]; then
    bufsize=40
fi

if [ -z "$2" ]; then
    shift=3
fi

# This script installs a new version of the trans
sudo rm /dev/trans0
sudo rm /dev/trans1
sudo rmmod translate
make clean
make 
sudo insmod translate.ko TRANSLATE_BUFSIZE=$bufsize TRANSLATE_SHIFT=$shift
echo Major-Nummer:
cat /proc/devices | grep trans0 | cut -c1-3 
sudo chmod 666 /dev/trans0
sudo chmod 666 /dev/trans1
# EOF

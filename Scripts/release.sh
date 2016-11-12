#!/bin/bash
# run with "bash release.sh"

#turn on echo
set -x
#stop the zen service or the destination files will be locked
sudo service zen stop

#copy rockbus to the rockbus user directory
sudo cp ~/Documents/Rockbus/Rockbus/main /home/rockbus/rockbus

#set the file ownership to rockbus
sudo chown rockbus:rockbus /home/rockbus/rockbus

#copy zen to the rockbus user directory
sudo cp ~/Documents/Rockbus/Zen/main /home/rockbus/zen

#set the file ownership to rockbus
sudo chown rockbus:rockbus /home/rockbus/zen

#start the zen service with the new release
sudo service zen start
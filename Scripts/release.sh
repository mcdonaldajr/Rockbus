#!/bin/bash
# run with "bash release.sh"

#turn on echo
set -x

#copy rockbus to the rockbus user directory
sudo cp "~/Documents/Rockbus/main" /home/rockbus/rockbus

#set the file ownership to rockbus
sudo chown rockbus:rockbus /home/rockbus/rockbus

#copy zen to the rockbus user directory
sudo cp "~/Documents/Geany Projects/Zen/main" /home/rockbus/zen

#set the file ownership to rockbus
sudo chown rockbus:rockbus /home/rockbus/zen
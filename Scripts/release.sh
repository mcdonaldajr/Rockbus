#!/bin/bash -vx
# run with "bash release.sh"
sudo cp "/home/pi/Documents/Geany Projects/Rockbus V2/main" /home/rockbus/rockbus
sudo chown rockbus:rockbus /home/rockbus/rockbus
sudo cp "/home/pi/Documents/Geany Projects/Zen/main" /home/rockbus/zen
sudo chown rockbus:rockbus /home/rockbus/zen
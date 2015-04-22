#!/bin/sh

#export relevant pins
echo 14 > /sys/class/gpio/export
echo 214 > /sys/class/gpio/export
echo 236 > /sys/class/gpio/export
echo 212 > /sys/class/gpio/export


echo "exports done"

#set up muxing
echo low > /sys/class/gpio/gpio214/direction
echo low > /sys/class/gpio/gpio236/direction
echo high > /sys/class/gpio/gpio212/direction
echo in > /sys/class/gpio/gpio14/direction
echo high > /sys/class/gpio/gpio214/direction

echo "muxing setup"
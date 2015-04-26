#!/bin/sh
 /usr/sbin/rfkill unblock bluetooth
 /usr/bin/hciconfig hci0 up
 /usr/bin/hciconfig hci0 piscan
 /usr/bin/hciconfig hic0 sspmode 0

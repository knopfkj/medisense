To use Bluetooth Low Energy on the Intel Edison you must first turn on Bluetooth. 

To do this automatically I would suggest adding the script btup.sh as explained at http://rwx.io/blog/2015/02/18/seting-up-an-edison/

place the script in /home/root/ and perform the command below to Make the script executable

chmod +x /home/root/btup.sh

also create a new service file /lib/systemd/system/btup.service

then enable that service using the following command:

 systemctl enable /lib/systemd/system/btup.service
 
 
 finally reboot the Edison

# medisense
MediSense Code Repository
------------------------------------------------------
Adventures in making the Intel Edison play with ThingWorxs

Use this tutorial as a basis:
http://www.thingworx.com/academic_content/galileo-weather-app/

The code from this tutorial is in the "IntelGalileoWeatherProject" folder.
IntelGalileoWeatherProject/TWX2/twLibrary contains all the necessary code that works the magic so the edison speaks to Thingworx.

IntelGalileoWeatherProject/TemperatureAndHumidityThingworxWifi contains the .ino Arduino file I used as a basis for the final code.

See /DOFinal for the completed code and configuration changes needed for eclipse.

-------------------------------------------------------
9DOF Sensor Work

DOF acceleration values are in mG.

Followed this guide, except used a sample C project instead of C++
https://learn.sparkfun.com/tutorials/programming-the-intel-edison-beyond-the-arduino-ide?_ga=1.211029523.740336311.1428326006

Here is the guy's code I used:
https://github.com/jku/LSM9DS0

NOTE: After you build the project in eclipse, and run it using the remote C/C++ Application run configuration, I was able to get it to run through the terminal on the Edison. Running the chmod 755 ./FILENAME and then ./FILENAME in the Edison terminal made it work.
----------------------------------------------------------
Pushbutton

This looks like a good guide. I'm going to try it out once we get the header pins soldered to the GPIO block:

https://learn.sparkfun.com/tutorials/sparkfun-blocks-for-intel-edison---gpio-block?_ga=1.209527316.740336311.1428326006

----------------------------------------------------------
Heart Rate

1) install https://github.com/intel-iot-devkit/mraa on edison board
2) code must be compiled via terminal on edison to it can reference the mraa librar (dont' forget -lmraa)



This is looking promising:
http://iotdk.intel.com/docs/master/mraa/

----------------------------------------------------------

Pulse Sensor Code
https://github.com/WorldFamousElectronics
Code Walkthrough:
http://pulsesensor.com/pages/pulse-sensor-amped-arduino-v1dot1

----------------------------------------------------------
(I used the download from the sparkfun guide at the top)
Download Intel's Eclipse C/C++ development environment for Edison at:
https://software.intel.com/sites/landingpage/iotdk/windows-development-kit.html

----------------------------------------------------------
Automatic Scripting at Boot-Up
----------------------------------------------------------
there are a few different ways to do this, but on Edison I found that the easiest way is using the /etc/init.d directory. There are many references out there for why and how this works, a good one can be found here. If this is your first time using init.d for start-up scripts, you’re going go to the directory /etc/init.d:


root@edison:/etc/init.d# update-rc.d rc.sh defaults

----------------------------------------------------------
Tricks / Good to know:
----------------------------------------------------------
configure_edison –setup (/etc/hostapd/hostapd.conf takes the root pw)

rfkill unblock wifi 

rfkill unblock bluetooth

opkg list – list available package to be installed

opkg update

opkg upgrade

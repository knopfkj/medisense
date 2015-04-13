# medisense
MediSense Code Repository

If you can't find a library when building in eclipse: "To fix it I went to Project -> Properties -> C/C++ Build -> Settings, then under "Cross G++ Compiler" I clicked on "Includes" and manually add all the directories."

Working on getting this to work on the edison:
http://www.thingworx.com/wp-content/uploads/2014/12/2.-How-to-read-Temperature-and-Humidity-from-HTU21D-sensor-using-Thingworx-C-SKD-for-I.pdf


-------------------------------------------------------
9DOF Sensor Work

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

----------------------------------------------------------

Pulse Sensor Code
https://github.com/WorldFamousElectronics
Code Walkthrough:
http://pulsesensor.com/pages/pulse-sensor-amped-arduino-v1dot1

----------------------------------------------------------
(I used the download from the sparkfun guide at the top)
Download Intel's Eclipse C/C++ development environment for Edison at:
https://software.intel.com/sites/landingpage/iotdk/windows-development-kit.html

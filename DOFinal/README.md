You will need to be sure to include the mraa-master folder in the
library search path, and also to include -lmraa in compilation (you can
add it as mraa in eclipse

The "1_c_helloworld.c" file is a combination of the 9DOF code, pushbutton code, and the ThingWorx code used in the sample arduino code for the Galileo. It is commented fairly well, and the ThingWorx code should have comments.


Access properties by right clicking on the project folder (after you imported the /src folder from here as a new project) and selecting properties.

Change the settings to be the same as the setup.png (minus the path, you shouldn't need that)

The run configurations are the same as testDOF, find the sparkfun tutorial link from there.
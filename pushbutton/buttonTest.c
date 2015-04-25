 
 #include "mraa/gpio.h"
 main(){
 mraa_gpio_context gpio;
    gpio = mraa_gpio_init(36);
    mraa_gpio_dir(gpio, MRAA_GPIO_IN);
    for (;;) {
        fprintf(stdout, "Gpio is %d\n", mraa_gpio_read(gpio));
        sleep(1);
    }
    mraa_gpio_close(gpio);
	}
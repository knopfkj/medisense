/*
#include "mraa.h"
int
main(int argc, char** argv)
{
    char* board_name = mraa_get_platform_name();
    fprintf(stdout, "hello mraa\n Version: %s\n Running on %s\n", mraa_get_version(), board_name);
    mraa_deinit();
    return MRAA_SUCCESS;



}
*/


#include "mraa/aio.h"
int
main()
{

	char * name = "";
	int pin = 0;
	int i = 0;
    mraa_aio_context adc_a0;
    uint16_t adc_value = 0;
    float adc_value_float = 0.0;
	

    char* board_name = mraa_get_platform_name();
        fprintf(stdout, "hello mraa\n Version: %s\n Running on %s\n", mraa_get_version(), board_name);
    //unsigned int pinCount = mraa_get_pin_count();
    //fprintf("%u", pinCount);


	//set pin input
//	for (i=0; i<300; i++) {
//	pin = i;
    pin = 36; //GP14

	printf("Pin#:%d\n", pin);

    adc_a0 = mraa_aio_init(pin);
	
	
	name = mraa_get_pin_name(pin);
	printf("Name:");
	printf("%s\n", name);
        //mraa_deinit();
        //return MRAA_SUCCESS;

		if (adc_a0 == NULL) {
			printf("NULL input\n");
			return 1;
		}
//		else {
//			printf("GOOD");
//			break;
//		}
//	} //end for loop
//return 1;
    for (;;) {
        adc_value = mraa_aio_read(adc_a0);
        adc_value_float = mraa_aio_read_float(adc_a0);
        fprintf(stdout, "ADC A0 read %X - %d\n", adc_value, adc_value);
        fprintf(stdout, "ADC A0 read float - %.5f\n", adc_value_float);
    }
    mraa_aio_close(adc_a0);
    return MRAA_SUCCESS;
}

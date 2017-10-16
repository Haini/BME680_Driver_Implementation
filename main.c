#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>

#include <unistd.h>				//Needed for I2C port
#include <fcntl.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port
#include <time.h>

#include "bme680.h"
#include "bsec_interface.h"


#define MODE_READ 0
#define MODE_WRITE 1
#define MAX_LEN 32


int file_i2c;
int length;
unsigned char buffer[60] = {0};
int notTerminated = 1;

/* Function Constructors. */
int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
void user_delay_ms(uint32_t period);
void intHandler(int tmp);

/* Function Implementations. */
int main (int argc, const char** argv)
{

	signal(SIGINT, intHandler);
	printf("Running ... \n");
    
	//----- OPEN THE I2C BUS -----
	char *filename = (char*)"/dev/i2c-1";
	if ((file_i2c = open(filename, O_RDWR)) < 0)
	{
		//ERROR HANDLING: you can check errno to see what went wrong
		printf("Failed to open the i2c bus");
		return 1;
	}    
    
    //---- AQUIRE BUS ACCESS AT ADDRESS ----
	int addr = BME680_I2C_ADDR_SECONDARY;          //<<<<<The I2C address of the slave
	if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
	{
		printf("Failed to acquire bus access and/or talk to slave.\n");
		//ERROR HANDLING; you can check errno to see what went wrong
		return 1;
	}
	
    /** Configure the sensor and assign function pointers for I2C Communication **/
	struct bme680_dev gas_sensor;

	gas_sensor.dev_id = BME680_I2C_ADDR_PRIMARY;
	gas_sensor.intf = BME680_I2C_INTF;
	gas_sensor.read = user_i2c_read;
	gas_sensor.write = user_i2c_write;
	gas_sensor.delay_ms = user_delay_ms;

	int8_t rslt = BME680_OK;
	rslt = bme680_init(&gas_sensor);

    /* Configure the sensor in forced sensor mode. */

    uint8_t set_required_settings;

	/* Set the temperature, pressure and humidity settings */
	gas_sensor.tph_sett.os_hum = BME680_OS_2X;
	gas_sensor.tph_sett.os_pres = BME680_OS_4X;
	gas_sensor.tph_sett.os_temp = BME680_OS_8X;
	gas_sensor.tph_sett.filter = BME680_FILTER_SIZE_3;

	/* Set the remaining gas sensor settings and link the heating profile */
	gas_sensor.gas_sett.run_gas = BME680_ENABLE_GAS_MEAS;
	/* Create a ramp heat waveform in 3 steps */
	gas_sensor.gas_sett.heatr_temp = 320; /* degree Celsius */
	gas_sensor.gas_sett.heatr_dur = 150; /* milliseconds */

	/* Select the power mode */
	/* Must be set before writing the sensor configuration */
	gas_sensor.power_mode = BME680_FORCED_MODE; 

	/* Set the required sensor settings needed */
	set_required_settings = BME680_OST_SEL | BME680_OSP_SEL | BME680_OSH_SEL | BME680_FILTER_SEL 
		| BME680_GAS_SENSOR_SEL;
		
	/* Set the desired sensor configuration */
	rslt = bme680_set_sensor_settings(set_required_settings,&gas_sensor);

	/* Set the power mode */
	rslt = bme680_set_sensor_mode(&gas_sensor);

	/* Get the total measurement duration so as to sleep or wait till the
	 * measurement is complete */
	uint16_t meas_period;
	bme680_get_profile_dur(&meas_period, &gas_sensor);
	user_delay_ms(meas_period); /* Delay till the measurement is ready */
    
    /* Read the sensor data. */
    struct bme680_field_data data;
	
	/* File writing */	
	FILE *f = fopen("data.txt", "a");
	time_t now;
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    exit(1);
	}

	while(notTerminated) 
	{
		rslt = bme680_get_sensor_data(&data, &gas_sensor);
		now = time(NULL);
		struct tm tm = *localtime(&now);	
		printf("Time: %d:%d:%d, T: %.2f degC, P: %.2f hPa, H %.2f %%rH ", tm.tm_hour, tm.tm_min, tm.tm_sec, data.temperature / 100.0f,
			data.pressure / 100.0f, data.humidity / 1000.0f );

		//fprintf(f, "T: %.2f degC, P: %.2f hPa, H %.2f %%rH \r\n", data.temperature / 100.0f,
	        //				data.pressure / 100.0f, data.humidity / 1000.0f );
		fprintf(f, "%d:%d:%d;%.2f;%.2f;%.2f;", tm.tm_hour, tm.tm_min, tm.tm_sec, data.temperature / 100.0f, data.pressure / 100.0f, 
					data.humidity / 1000.0f);
		/* Avoid using measurements from an unstable heating setup */
		if(data.status & BME680_HEAT_STAB_MSK) {
			printf(", G: %d ohms", data.gas_resistance);
			fprintf(f, "%d", data.gas_resistance);
		}
		
		printf("\r\n");
		fprintf(f, "\r\n");
		fflush(f);
		sleep(5);
		rslt = bme680_set_sensor_mode(&gas_sensor);
		user_delay_ms(meas_period + 5*1000);
	}
	fclose(f);
    return 0;
}

void intHandler(int tmp)
{
	notTerminated = 0;
}

int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
    
    /* Step 1: Write Register Address to read to slave. */
	uint8_t tmp[1];
	tmp[0] = reg_addr;
	length = 1;			//<<< Number of bytes to write 
	if (write(file_i2c, tmp, length) != length)		
	{
        	rslt = 1;
		//ERROR HANDLING: i2c transaction failed
		perror("Failed to write register number to the i2c bus.\n");
	}

    /* Step 2: Read Register Data for len */
    if (read(file_i2c, reg_data, len) != len)
    {
        rslt = 1;
        perror("Failed to write data to i2c bus.\n");
    }
    /*
     * The parameter dev_id can be used as a variable to store the I2C address of the device
     */

    /*
     * Data on the bus should be like
     * |------------+---------------------|
     * | I2C action | Data                |
     * |------------+---------------------|
     * | Start      | -                   |
     * | Write      | (reg_addr)          |
     * | Stop       | -                   |
     * | Start      | -                   |
     * | Read       | (reg_data[0])       |
     * | Read       | (....)              |
     * | Read       | (reg_data[len - 1]) |
     * | Stop       | -                   |
     * |------------+---------------------|
     */

    return rslt;
}

int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    	int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
    	length = 1;
	uint8_t i = 1;
	uint8_t tmp[16];
	tmp[0] = reg_addr;

	for (i = 1; i <= len; i++) {
		tmp[i] = reg_data[i-1];	
	}

	if (write(file_i2c, tmp, len+1) != (len+1)) {
		perror("Could not write data to i2c BUS");
		rslt = 1;
		exit(1);
	}

//    /* Step 1: Write the register address. */
//	if (write(file_i2c, &reg_addr, length) != length)		
//	{
//        rslt = 1;
//		//ERROR HANDLING: i2c transaction failed
//		printf("Failed to read from the i2c bus.\n");
//	}
//    
//    /* Step 2: Write the command to the slave. */
//	if (write(file_i2c, reg_data, len) != len)		
//	{
//        rslt = 1;
//		//ERROR HANDLING: i2c transaction failed
//		printf("Failed to read from the i2c bus.\n");
//	}

    /*
     * The parameter dev_id can be used as a variable to store the I2C address of the device
     */

    /*
     * Data on the bus should be like
     * |------------+---------------------|
     * | I2C action | Data                |
     * |------------+---------------------|
     * | Start      | -                   |
     * | Write      | (reg_addr)          |
     * | Write      | (reg_data[0])       |
     * | Write      | (....)              |
     * | Write      | (reg_data[len - 1]) |
     * | Stop       | -                   |
     * |------------+---------------------|
     */

    return rslt;
}

void user_delay_ms(uint32_t period)
{
    /* sleep waits for seconds, so we need to convert
	ms to s. */
    sleep(period/1000);
    /*
     * Return control or wait,
     * for a period amount of milliseconds
     */
}


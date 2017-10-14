#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <unistd.h>				//Needed for I2C port
#include <fcntl.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port


#include "bme680.h"


#define MODE_READ 0
#define MODE_WRITE 1
#define MAX_LEN 32


int file_i2c;
int length;
unsigned char buffer[60] = {0};


/* Function Constructors. */
int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
void user_delay_ms(uint32_t period);


/* Function Implementations. */
int main (int argc, const char** argv)
{
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
	int addr = BME680_I2C_ADDR_PRIMARY;          //<<<<<The I2C address of the slave
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
	
	while(1) 
	{
		rslt = bme680_get_sensor_data(&data, &gas_sensor);

		printf("T: %.2f degC, P: %.2f hPa, H %.2f %%rH ", data.temperature / 100.0f,
			data.pressure / 100.0f, data.humidity / 1000.0f );
		/* Avoid using measurements from an unstable heating setup */
		if(data.status & BME680_HEAT_STAB_MSK)
			printf(", G: %d ohms", data.gas_resistance);
		
		printf("\r\n");
	}
    return 0;
}

int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
    
    /* Step 1: Write Register Address to read to slave. */
	length = 1;			//<<< Number of bytes to write 
	if (write(file_i2c, &reg_addr, length) != length)		
	{
        rslt = 1;
		//ERROR HANDLING: i2c transaction failed
		printf("Failed to read from the i2c bus.\n");
	}

    /* Step 2: Read Register Data for len */
    if (read(file_i2c, reg_data, len) != len)
    {
        rslt = 1;
        printf("Failed to write to i2c bus.\n");
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

    /* Step 1: Write the register address. */
	if (write(file_i2c, &reg_addr, length) != length)		
	{
        rslt = 1;
		//ERROR HANDLING: i2c transaction failed
		printf("Failed to read from the i2c bus.\n");
	}
    
    /* Step 2: Write the command to the slave. */
	if (write(file_i2c, reg_data, len) != len)		
	{
        rslt = 1;
		//ERROR HANDLING: i2c transaction failed
		printf("Failed to read from the i2c bus.\n");
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
    /* Standard I2C clock is 1kHz, T=1/f. */
    sleep(period/1000);
    /*
     * Return control or wait,
     * for a period amount of milliseconds
     */
}


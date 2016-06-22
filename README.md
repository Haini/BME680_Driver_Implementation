CONTENTS OF THIS FILE
======================
	* Introduction
	* Version
	* Integration details
	* Driver files information
	* Supported sensor interface
	* Simple Integration Example

INTRODUCTION
=============
	- This package contains the Bosch Sensortec MEMS BME680 sensor driver (sensor API)
	- The sensor driver package includes below files
		* bme680.c
		* bme680.h
		* bme680_calculations.c
		* bme680_calculations.h
		* bme680_internal.h
		* sensor_api_common_types.h
		* BME680_SensorAPI_Optimization_Example_Guide_External.pdf		

VERSION
========
	- Version of bme680 sensor driver is:
		* bme680.c                              - 2.0.0
		* bme680.h                              - 2.0.0
		* bme680_calculations.c                 - 2.0.0
		* bme680_calculations.h                 - 2.0.0
		* bme680_internal.h                     - 2.0.0
		* sensor_api_common_types.h             - 2.0.0
		* BME680_SensorAPI_Example_Guide.pdf    - 2.0.0

INTEGRATION DETAILS
====================
	- Integrate files bme680.c, bme680.h, bme680_calculations.c, bme680_calculations.h, bme680_internal.h,
		and sensor_api_common_types.h into your project.
		
	- User has to refer bme680.h to refer the API calls for the integration.
		
	- The BME680_SensorAPI_Example_Guide.pdf contains examples for API use cases.

DRIVER FILES INFORMATION
===========================
	bme680.h
	---------
		* This header file has the constant definitions, user data types and supported sensor driver calls declarations which is required by the user.

	bme680.c
	---------
		* This file contains the implementation for the sensor driver APIs.

	bme680_calculations.h
	----------------------
		* This header file has the internal function declaration for the sensor calculation.

	bme680_calculations.c
	----------------------
		* This file contains the implementation of the sensor calculations for sensor driver APIs.
		
	bme680_internal.h
	------------------
		* This header file has the register address definition, internal constant definitions.

	sensor_api_common_types.h
	--------------------------
		* This header file has the data type definition for different compiler platform.


SUPPORTED SENSOR INTERFACE
===========================
	- This BME680 sensor driver supports SPI and I2C interfaces
	
Simple Integration Example
===========================
	- A simple example for BME680 is given below. 
	- Example meant for Single BME680 sensor in Force Mode with Temperature 
		Pressure, Humidity and Gas Enabled
	- For further examples and details refer BME680_SensorAPI_Example_Guide.pdf 
	- Please refer bme680.h to refer the API calls for the integration.

	/* include bme680 main header */
	#include "bme680.h"
	/*!
	* BME680_MAX_NO_OF_SENSOR = 2; defined in bme680.h file
	* In order to interface only one sensor over SPI, user must change the value of
	* BME680_MAX_NO_OF_SENSOR = 1
	* Test setup: It has been assumed that “BME680 sensor_0” interfaced over SPI with
	* Native chip select line
	*/
	/* BME680 sensor structure instance */
	struct bme680_t bme680_sensor_no[BME680_MAX_NO_OF_SENSOR];
	/* BME680 sensor’s compensated data structure instance */
	struct bme680_comp_field_data compensate_data_sensor[BME680_MAX_NO_OF_SENSOR][3];
	/* BME680 sensor’s uncompensated data structure instance */
	struct bme680_uncomp_field_data uncompensated_data_of_sensor[BME680_MAX_NO_OF_SENSOR][3];
	/* BME680 sensor’s configuration structure instance */
	struct bme680_sens_conf set_conf_sensor[BME680_MAX_NO_OF_SENSOR];
	/* BME680 sensor’s heater configuration structure instance */
	struct bme680_heater_conf set_heatr_conf_sensor[BME680_MAX_NO_OF_SENSOR];
	
	void main(void)
	{
		unsigned int i = BME680_INIT_VALUE;
		enum bme680_return_type com_rslt = BME680_COMM_RES_ERROR;
		
		/* Do BME680 sensor structure instance initialization*/
		/* Sensor_0 interface over SPI with native chip select line */
		/* USER defined SPI bus read function */
		bme680_sensor_no[0].bme680_bus_read = BME680_SPI_bus_read_user;
		/* USER defined SPI bus write function */
		bme680_sensor_no[0].bme680_bus_write = BME680_SPI_bus_write_user;
		/* USER defined SPI burst read function */
		bme680_sensor_no[0].bme680_burst_read = BME680_SPI_bus_read_user;
		/* USER defined delay function */
		bme680_sensor_no[0].delay_msec = BME680_delay_msec_user;
		/* Mention communication interface */
		bme680_sensor_no[0].interface = BME680_SPI_INTERFACE;
		
		/* get chip id and calibration parameter */
		com_rslt = bme680_init(&bme680_sensor_no[0]);
		
		/* Do Sensor initialization */
		for (i=0;i<BME680_MAX_NO_OF_SENSOR;i++) {
			/* Check Device-ID before next steps of sensor operations */
			if (BME680_CHIP_ID == bme680_sensor_no[i].chip_id) {
			/* Select sensor configuration parameters */
				set_conf_sensor[i].heatr_ctrl = BME680_HEATR_CTRL_ENABLE;
				set_conf_sensor[i].run_gas = BME680_RUN_GAS_ENABLE;
				set_conf_sensor[i].nb_conv = 0x00;
				set_conf_sensor[i].osrs_hum = BME680_OSRS_1X;
				set_conf_sensor[i].osrs_pres = BME680_OSRS_1X;
				set_conf_sensor[i].osrs_temp = BME680_OSRS_1X;
				
				/* activate sensor configuration */
				com_rslt += bme680_set_sensor_config(&set_conf_sensor[i],
				&bme680_sensor_no[i]);
				
				/* Select Heater configuration parameters */
				set_heatr_conf_sensor[i].heater_temp[0] = 300;
				set_heatr_conf_sensor[i].heatr_idacv[0] = 1;
				set_heatr_conf_sensor[i].heatr_dur[0] = 137;
				set_heatr_conf_sensor[i].profile_cnt = 1;
				
				/* activate heater configuration */
				com_rslt += bme680_set_gas_heater_config(&set_heatr_conf_sensor[i],
				&bme680_sensor_no[i]);
				
				/* Set power mode as forced mode */
				com_rslt += bme680_set_power_mode(BME680_FORCED_MODE,&bme680_sensor_no[i]);
				
					if (BME680_COMM_RES_OK == com_rslt) {
						/*Get the uncompensated T+P+G+H data*/
						bme680_get_uncomp_data(uncompensated_data_of_sensor[i], 1, BME680_ALL,
						&bme680_sensor_no[i]);
						
						/*Get the compensated T+P+G+H data*/
						bme680_compensate_data(uncompensated_data_of_sensor[i],
						compensate_data_sensor[i], 1,
						BME680_ALL, &bme680_sensor_no[i]);
						
						/* put sensor into sleep mode explicitly */
						bme680_set_power_mode(BME680_SLEEP_MODE, &bme680_sensor_no[i]);
						
						/* call user define delay function(duration millisecond) */
						User_define_delay(100);
					}
				}
			}
	}
/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>

#include "ADC.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define ADC_CHANNEL_RAIN    ADC_CHANNEL_7
#define ADC_CHANNEL_SOIL    ADC_CHANNEL_6
#define ADC_CHANNEL_WIND    ADC_CHANNEL_4
#define ADC_CHANNEL_BATT    ADC_CHANNEL_5
#define ADC_ATTEN           ADC_ATTEN_DB_12
#define ENABLE_PIN  		GPIO_NUM_4

ADC::ADC()
{
	handle = NULL;

	init_config1 = {
	.unit_id = ADC_UNIT_1,
	.ulp_mode = ADC_ULP_MODE_DISABLE,
	};
	
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &handle));
	
	config = {
		.atten = ADC_ATTEN,
		.bitwidth = ADC_BITWIDTH_DEFAULT
	};
	
	ESP_ERROR_CHECK(adc_oneshot_config_channel(handle, ADC_CHANNEL_RAIN, &config));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(handle, ADC_CHANNEL_SOIL, &config));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(handle, ADC_CHANNEL_WIND, &config));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(handle, ADC_CHANNEL_BATT, &config));
	
	cali_handle = NULL;
	
	
	cali_config = {
	.unit_id = ADC_UNIT_1,
	.atten = ADC_ATTEN_DB_12,
	.bitwidth = ADC_BITWIDTH_DEFAULT,
	};
	
	
	ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle));
	gpio_set_direction(ENABLE_PIN, GPIO_MODE_OUTPUT);
}

ADC::~ADC()
{

}

int ADC::measure_rain()
{   
	ESP_ERROR_CHECK(adc_oneshot_read(handle, ADC_CHANNEL_RAIN, &rain));
	adc_cali_raw_to_voltage(cali_handle, rain, &mv_output);
	vTaskDelay(20 / portTICK_PERIOD_MS);
	return mv_output;
}

int ADC::measure_soil()
{   
	ESP_ERROR_CHECK(adc_oneshot_read(handle, ADC_CHANNEL_SOIL, &soil));
	adc_cali_raw_to_voltage(cali_handle, soil, &mv_output);
	vTaskDelay(20 / portTICK_PERIOD_MS);
	return mv_output;
}

int ADC::measure_wind()
{
	ESP_ERROR_CHECK(adc_oneshot_read(handle, ADC_CHANNEL_WIND, &wind));
	adc_cali_raw_to_voltage(cali_handle, wind, &mv_output);
	vTaskDelay(20 / portTICK_PERIOD_MS);
	return mv_output -142;
}

int ADC::measure_batt()
{
	gpio_set_level(ENABLE_PIN, 1); 
	ESP_ERROR_CHECK(adc_oneshot_read(handle, ADC_CHANNEL_BATT, &batt));
	adc_cali_raw_to_voltage(cali_handle, batt, &mv_output);
	vTaskDelay(20 / portTICK_PERIOD_MS);
	gpio_set_level(ENABLE_PIN, 0); 
	return mv_output;
}
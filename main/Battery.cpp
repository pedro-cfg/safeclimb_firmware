#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "Battery.h"

#define ADC1_EXAMPLE_CHAN0          ADC1_CHANNEL_6
#define ADC_EXAMPLE_ATTEN           ADC_ATTEN_DB_12
#define ENABLE_PIN  				GPIO_NUM_27

Battery::Battery()
{
	ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_EXAMPLE_CHAN0, ADC_EXAMPLE_ATTEN));
    gpio_set_direction(ENABLE_PIN, GPIO_MODE_OUTPUT);
}

Battery::~Battery()
{
	
}

int Battery::measure()
{   
	gpio_set_level(ENABLE_PIN, 1); 
	vTaskDelay(20 / portTICK_PERIOD_MS);
	int raw = 0;
	for(int i = 0; i<100; i++)
	{
		adc_raw[0][i] = adc1_get_raw(ADC1_EXAMPLE_CHAN0);
		raw += adc_raw[0][i];
	}
    raw /= 100;
    int voltage = (int)(raw*3600.0/4096.0);
    gpio_set_level(ENABLE_PIN, 0); 
	return voltage;
}
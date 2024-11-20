#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "Soil.h"

#define ADC1_EXAMPLE_CHAN1          ADC1_CHANNEL_6
#define ADC_EXAMPLE_ATTEN           ADC_ATTEN_DB_12
#define ENABLE_PIN  				GPIO_NUM_34

Soil::Soil()
{
	ESP_ERROR_CHECK(adc1_config_width((adc_bits_width_t)ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_EXAMPLE_CHAN1, ADC_EXAMPLE_ATTEN));
    gpio_set_direction(ENABLE_PIN, GPIO_MODE_OUTPUT);
}

Soil::~Soil()
{
	
}

int Soil::measure()
{   
	gpio_set_level(ENABLE_PIN, 1); 
	vTaskDelay(20 / portTICK_PERIOD_MS);
	int raw = 0;
	for(int i = 0; i<100; i++)
	{
		adc_raw[0][i] = adc1_get_raw(ADC1_EXAMPLE_CHAN1);
		raw += adc_raw[0][i];
	}
    raw /= 100;
    int voltage = (int)(raw*3300.0/4096.0);
    gpio_set_level(ENABLE_PIN, 0); 
	return voltage;
}

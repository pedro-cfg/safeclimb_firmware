#include "esp_adc/adc_oneshot.h"

#ifndef ADC_H
#define ADC_H  

class ADC {
private:
	adc_oneshot_unit_handle_t handle;
	adc_oneshot_unit_init_cfg_t init_config1;
	adc_oneshot_chan_cfg_t config;
	adc_cali_handle_t cali_handle;
	adc_cali_line_fitting_config_t cali_config;
	int rain;
	int soil;
	int wind;
	int batt;
	
	int mv_output;
public:
	ADC();
	~ADC();
	int measure_rain();
	int measure_soil();
	int measure_wind();
	int measure_batt();
};

#endif

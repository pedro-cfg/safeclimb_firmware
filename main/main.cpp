#include <cstring>
#include <stdint.h>
#include <stdio.h>
#include <sys/_stdint.h>
#include <time.h>
#include <sys/time.h>
#include <string>

#include "esp_private/esp_clk.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "led_strip.h"
#include "driver/spi_master.h"
#include "rom/gpio.h"
#include "sdkconfig.h"
#include "esp32-dht11.h"

#include "parameters.h"
#include "BluetoothManager.h"
#include "Temperature.h"
#include "LoraManager.h"
#include "ADC.h"

#define CONFIG_DHT11_PIN GPIO_NUM_17
#define CONFIG_CONNECTION_TIMEOUT 5

#define BUTTON_PIN GPIO_NUM_27
#define LED GPIO_NUM_26

BluetoothManager btManager;
LoraManager loraManager;
ADC adc;
WIFI wifi;
uint8_t buf[256];

TaskHandle_t xHandleBluetooth= NULL;
TaskHandle_t xHandleLoRa = NULL;
TaskHandle_t xHandleWifi = NULL;

bool main_tower = MAIN_TOWER;
bool server_tower = SERVER_TOWER;

bool led_status = false;
bool bluetooth_comm = false;
bool bluetooth_flag = false;

static void IRAM_ATTR gpio_interrupt_handler(void *args)
{	
	bluetooth_comm = true;
	//loraManager.setTransmitting(false);
	xTaskNotifyGive(xHandleBluetooth);
}

static void deep_sleep_register_rtc_timer_wakeup(void)
{
    const int wakeup_time_sec = main_tower?SLEEP_TIME:SLEEP_TIME-5;
    printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
}

void deep_sleep_register_ext0_wakeup(void)
{
    printf("Enabling EXT0 wakeup on pin GPIO%d\n", BUTTON_PIN);
    ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0));

    // Configure pullup/downs via RTCIO to tie wakeup pins to inactive level during deepsleep.
    // EXT0 resides in the same power domain (RTC_PERIPH) as the RTC IO pullup/downs.
    // No need to keep that power domain explicitly, unlike EXT1.
    ESP_ERROR_CHECK(gpio_pullup_en((gpio_num_t)BUTTON_PIN));
    ESP_ERROR_CHECK(gpio_pulldown_dis((gpio_num_t)BUTTON_PIN));
}

void blink_led()
{
	led_status = !led_status;
	gpio_set_level(LED, led_status);
}

void thread_LoRa(void *pvParameters)
{
	int temp = 0;
	int air_humidity = 0;
	int soil_humidity = 0;
	int wind_speed = 0;
	int rain = 0;
	int batt1 = 0;
	int batt2 = 0;
	dht11_t dht11_sensor;
	dht11_sensor.dht11_pin = CONFIG_DHT11_PIN;
	
	loraManager.init(&wifi);
	struct timeval now;
    gettimeofday(&now, NULL);
    
	if(main_tower && !bluetooth_comm)
    {
//		while(1){
		temp = temperature();
		printf("\n\nTemperature: %.1f C \n", (float)temp/10.0);
		vTaskDelay(2000/portTICK_PERIOD_MS);
		printf("\nDTH11 sensor:\n");
		if(!dht11_read(&dht11_sensor, CONFIG_CONNECTION_TIMEOUT))
		{  
//		printf("[Temperature]> %.2f \n",dht11_sensor.temperature);
//		printf("[Humidity]> %.2f \n",dht11_sensor.humidity);
			air_humidity = (int)dht11_sensor.humidity;
			printf("Air Humidity: %d%% \n", air_humidity);
		}
		vTaskDelay(100/portTICK_PERIOD_MS);
		printf("\nSoil_sensor read...\n");
		soil_humidity = adc.measure_soil();
		soil_humidity = (int)(-100.0f*((float)soil_humidity - 2172.0f)/1183.0f);
		if(soil_humidity > 100)
			soil_humidity = 100;
		else if(soil_humidity < 0)
			soil_humidity = 0;
		printf("Percentage: %d%% \n", soil_humidity);
		vTaskDelay(100/portTICK_PERIOD_MS);
		printf("\nRain_sensor read...\n");
		rain = adc.measure_rain();
		rain = (int)(-100.0f*((float)rain - 3165.0f)/1668.0f);
		if(rain > 100)
			rain = 100;
		else if(rain < 0)
			rain = 0;
		printf("Percentage: %d%% \n", rain);
		vTaskDelay(100/portTICK_PERIOD_MS);
		printf("\nWind sensor:\n");
		wind_speed = adc.measure_wind();
		wind_speed /=10;
		printf("Voltage: %d km/h \n", wind_speed);
		vTaskDelay(100/portTICK_PERIOD_MS);
		printf("\nBatteries:\n");
		batt1 = adc.measure_batt1();
		printf("Voltage: %.3f V \n\n", (float)batt1/1000.0);
		vTaskDelay(100/portTICK_PERIOD_MS);
//		}
		int send_len = sprintf((char *)buf,"T");
		vTaskDelay(500 / portTICK_PERIOD_MS);
		loraManager.sendPackage(buf, send_len, 0,false,false,temp,air_humidity,soil_humidity,wind_speed,rain,batt1,batt2);
//		loraManager.sendPackage(buf, send_len, 0,true);
	}
	else
	{
		if(!main_tower && !server_tower)
		{
			batt2 = adc.measure_batt2();
			printf("Voltage: %.3f V \n\n", (float)batt2/1000.0);
			vTaskDelay(100/portTICK_PERIOD_MS);
			loraManager.setRepeaterBattery(batt2);
		}
		loraManager.receivePackage();
	}
	
	
    loraManager.setInitialTime(now);
    
	while(1) {
		vTaskDelay(1);		
		loraManager.exec();
	} 
}

void thread_Bluetooth(void *pvParameters)
{
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	loraManager.receivePackage();
	//loraManager.setHigherOrder(true);
	printf("Initializing Bluetooth\n");
	btManager.turnOn();
	loraManager.setKeepAlive(true);
	//loraManager.setTransmitting(false);
	
	while(1) {
//		if(bluetooth_flag && !connected)
//		{
//			btManager.setTelephoneBool(false);
//			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//			loraManager.receivePackage();
//			loraManager.setHigherOrder(true);
//			bluetooth_flag = false;
//		}
		
		btManager.receiveData();
		if(btManager.dataReceived())
		{
			printf("Frase: %s",btManager.getData().c_str());
			int send_len = sprintf((char *)buf,"%s", btManager.getData().c_str());
			struct timeval now;
    		gettimeofday(&now, NULL);
    		loraManager.setInitialTime(now);
			if(btManager.getTelephoneBool())
    		{
				loraManager.sendPackage(buf, send_len, 0,2,true);
				//loraManager.setHigherOrder(true);
				btManager.setTelephoneBool(false);
			}
			else
			{
				if (send_len > LORA_BYTES)
				{
				    size_t ind = 0;
				    while (ind < send_len)
				    {
				        size_t diff = (send_len - ind) > LORA_BYTES ? LORA_BYTES : (send_len - ind);
				        uint8_t bufSec[LORA_BYTES]; 
				        memcpy(bufSec, buf + ind, diff);
				        if(diff < LORA_BYTES)
				        	loraManager.sendPackage(bufSec, diff, 0, 3, true);
				        else
 							loraManager.sendPackage(bufSec, diff, 0, 1, true);
				        ind += diff; 
				    }
				}
				else
				{
				    loraManager.sendPackage(buf, send_len, 0, 3, true);
				}
				//loraManager.setHigherOrder(true);
			}
		}
		
//		if(loraManager.getMessageBluetoothReady() && !btManager.getDataSent())
//		{
//			btManager.setDataSent(true);
//	        btManager.sendData(reinterpret_cast<const uint8_t*>(loraManager.getMessageBluetooth()), loraManager.getMessageBluetoothSize());
//		}

		if(!connected)
			blink_led();
		else
		{
			bluetooth_flag = true;
			if(!led_status)
			{
				blink_led();
			}
		}

		vTaskDelay(500 / portTICK_PERIOD_MS); 
		
	} 
	
}

void thread_Wifi(void *pvParameters)
{
	while(1) {	
		wifi.MQTTsubscribe();
		wifi.MQTTreceive();
	} 
	
}

extern "C" void app_main()
{	    
    //Configure DeepSleep
    deep_sleep_register_rtc_timer_wakeup();
    deep_sleep_register_ext0_wakeup();

	gpio_pad_select_gpio((gpio_num_t)BUTTON_PIN);
    gpio_set_direction((gpio_num_t)BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en((gpio_num_t)BUTTON_PIN);
    gpio_pulldown_dis((gpio_num_t)BUTTON_PIN);
    gpio_set_intr_type((gpio_num_t)BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
    
    gpio_reset_pin(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_set_level(LED, led_status);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_interrupt_handler, (void *)BUTTON_PIN);
    
    //btManager.turnOn();
    
    if(!SERVER_TOWER)
    {
		loraManager.setBluetoothManager(&btManager);
	}
		
  	int paramBluetooth = 1;
 	xTaskCreate( thread_Bluetooth, "THREAD_BLUETOOTH", STACK_SIZE, &paramBluetooth, tskIDLE_PRIORITY, &xHandleBluetooth );
 	configASSERT( xHandleBluetooth );
 	
 	if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0)
 	{
		 xTaskNotifyGive(xHandleBluetooth);
		 bluetooth_comm = true;
	}
		
	int paramLoRa = 2;
    xTaskCreate( thread_LoRa, "THREAD_LORA", STACK_SIZE, &paramLoRa, tskIDLE_PRIORITY, &xHandleLoRa );
    configASSERT( xHandleLoRa );
    
    
    if(SERVER_TOWER)
    {
		wifi.init();
		wifi.setLoraManager(&loraManager);
		int paramWifi = 3;
	    xTaskCreate( thread_Wifi, "THREAD_WIFI", STACK_SIZE, &paramWifi, tskIDLE_PRIORITY, &xHandleWifi );
	    configASSERT( xHandleWifi );
	}
   
}

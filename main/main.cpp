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

bool bluetooth_comm = false;

static void IRAM_ATTR gpio_interrupt_handler(void *args)
{	
	bluetooth_comm = true;
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

void thread_LoRa(void *pvParameters)
{
	int temp = 0;
	int air_humidity = 0;
	int soil_humidity = 0;
	int wind_speed = 0;
	int rain = 0;
	int batt = 0;
	dht11_t dht11_sensor;
	dht11_sensor.dht11_pin = CONFIG_DHT11_PIN;
	
	loraManager.init(&wifi);
	struct timeval now;
    gettimeofday(&now, NULL);
    
	if(main_tower && !bluetooth_comm)
    {
		temp = temperature();
		printf("\n\nTemperature: %.1f C \n", (float)temp/10.0);
		vTaskDelay(100/portTICK_PERIOD_MS);
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
		printf("Voltage: %.3f V \n", (float)soil_humidity/1000.0);
		vTaskDelay(100/portTICK_PERIOD_MS);
		printf("\nRain_sensor read...\n");
		rain = adc.measure_rain();
		printf("Voltage: %.3f V \n", (float)rain/1000.0);
		vTaskDelay(100/portTICK_PERIOD_MS);
		printf("\nWind sensor:\n");
		wind_speed = adc.measure_wind();
		printf("Voltage: %.3f V \n", (float)wind_speed/1000.0);
		vTaskDelay(100/portTICK_PERIOD_MS);
		printf("\nBatteries:\n");
		batt = adc.measure_batt();
		printf("Voltage: %.3f V \n\n", (float)batt/1000.0);
		vTaskDelay(100/portTICK_PERIOD_MS);
		int send_len = sprintf((char *)buf,"BATT %d",batt);
		vTaskDelay(500 / portTICK_PERIOD_MS);
		loraManager.sendPackage(buf, send_len, 0,false,false,temp,air_humidity,soil_humidity,wind_speed,rain);
//		loraManager.sendPackage(buf, send_len, 0,true);
	}
	else
	{
		loraManager.receivePackage();
	}
	
	
    loraManager.setInitialTime(now);
    
	while(1) {
		vTaskDelay(1);
		if(loraManager.teste)
		{
			loraManager.teste = false;
		}
		
		loraManager.exec();
	} 
}

void thread_Bluetooth(void *pvParameters)
{
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	printf("Initializing Bluetooth\n");
	btManager.turnOn();
	loraManager.setKeepAlive(true);
	
	while(1) {
		btManager.receiveData();
		if(btManager.dataReceived())
		{
			printf("Frase: %s",btManager.getData().c_str());
			int send_len = sprintf((char *)buf,"%s", btManager.getData().c_str());
			loraManager.sendPackage(buf, send_len, 0,true,true);
		}
		
		if(loraManager.getMessageBluetoothReady())
		{
	        btManager.sendData(reinterpret_cast<const uint8_t*>(loraManager.getMessageBluetooth()), loraManager.getMessageBluetoothSize());
		}

		vTaskDelay(100 / portTICK_PERIOD_MS); 
	} 
	
}

void thread_Wifi(void *pvParameters)
{
	wifi.init();
	while(1) {
//		wifi.MQTTpublish(20,50,100,200,300);
		vTaskDelay(10000 / portTICK_PERIOD_MS); 
	} 
	
}

extern "C" void app_main()
{	    
    //Configure DeepSleep
    deep_sleep_register_rtc_timer_wakeup();
    deep_sleep_register_ext0_wakeup();
    
    if(SERVER_TOWER)
    {
		wifi.init();
	}

	gpio_pad_select_gpio((gpio_num_t)BUTTON_PIN);
    gpio_set_direction((gpio_num_t)BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en((gpio_num_t)BUTTON_PIN);
    gpio_pulldown_dis((gpio_num_t)BUTTON_PIN);
    gpio_set_intr_type((gpio_num_t)BUTTON_PIN, GPIO_INTR_LOW_LEVEL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_interrupt_handler, (void *)BUTTON_PIN);
    
    //btManager.turnOn();
		
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
    
//    int paramWifi = 3;
//    xTaskCreate( thread_Wifi, "THREAD_WIFI", STACK_SIZE, &paramWifi, tskIDLE_PRIORITY, &xHandleWifi );
//    configASSERT( xHandleWifi );

}

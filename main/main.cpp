#include <cstring>
#include <stdint.h>
#include <stdio.h>
#include <sys/_stdint.h>
#include <time.h>
#include <sys/time.h>
#include <string>

#include "esp_private/esp_clk.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "led_strip.h"
#include "driver/spi_master.h"
#include "sdkconfig.h"
#include "esp32-dht11.h"

#include "parameters.h"
#include "BluetoothManager.h"
#include "Temperature.h"
#include "LoraManager.h"
#include "Wind.h"

#define CONFIG_DHT11_PIN GPIO_NUM_4
#define CONFIG_CONNECTION_TIMEOUT 5

BluetoothManager btManager;
LoraManager loraManager;

bool main_tower = MAIN_TOWER;

static void deep_sleep_register_rtc_timer_wakeup(void)
{
    const int wakeup_time_sec = main_tower?SLEEP_TIME:SLEEP_TIME-5;
    printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
}

//void thread_LoRa_Send(void *pvParameters)
//{
//	int button = 1;
//	while(1) {
//		button = gpio_get_level(BUTTON_GPIO);
//		if(button == 0)
//		{
//			vTaskDelay(1);
//			lora_send_packet((uint8_t*)"Hello", 5);
//			printf("package sent\n");
//			vTaskDelay(2000 / portTICK_PERIOD_MS);
//		}
//		vTaskDelay(100 / portTICK_PERIOD_MS);
//	} 
//}
//
//void thread_LoRa_Receive(void *pvParameters)
//{
//	int rssi;
//	int snr;
//	ESP_LOGI(pcTaskGetName(NULL), "Start");
//	uint8_t buf[256]; // Maximum Payload 
//	while(1) {
//		lora_receive(); 
//		if (lora_received()) {
//			int rxLen = lora_receive_packet(buf, sizeof(buf));
//			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
//			rssi = lora_packet_rssi();
//			snr = lora_packet_snr();
//			printf("The RSSI value is: %d\n", rssi);
//			printf("The SNR value is: %d\n\n", snr);
//			vTaskDelay(1); // Avoid WatchDog alerts
//			lora_send_packet((uint8_t*)"Received", 8);
//			printf("response sent\n");
//		}
//		vTaskDelay(1); // Avoid WatchDog alerts
//	} 
//}

void thread_LoRa(void *pvParameters)
{
	
	loraManager.init();
	struct timeval now;
    gettimeofday(&now, NULL);
    
	if(main_tower)
	{
		uint8_t buf[256];
		int send_len = sprintf((char *)buf,"Trasmissão!. O relógio está em %lld",now.tv_sec);
		vTaskDelay(500 / portTICK_PERIOD_MS);
		loraManager.sendPackage(buf, send_len, 0);
	}
	else
	{
		loraManager.receivePackage();
	}
	
    loraManager.setInitialTime(now);
    
	while(1) {
		loraManager.exec();
	} 
}

void thread_Bluetooth(void *pvParameters)
{
	std::string str;
	int i = 0;
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	btManager.turnOn();
	while(1) {
		
//		for(int j = 0; j<10; j++)
//		{
			str = "Olá! Esse é o envio de número " + std::to_string(i); 
	        const uint8_t* data = reinterpret_cast<const uint8_t*>(str.c_str());
	        btManager.sendData(data, str.size());
			printf("New Data!\n");
			btManager.receiveData();
			i++;
			vTaskDelay(3000 / portTICK_PERIOD_MS); 
//		}
//		btManager.turnOff();
//		vTaskDelay(20000 / portTICK_PERIOD_MS); 
	} 
}

//extern "C" {
//    void temperature(); // Function prototype if it's defined in a C source file.
//}

void thread_sensors(void *pvParameters)
{
//	printf("Sensors ON!\n");
//	while(1) {
//		temperature();
//	} 

//    dht11_t dht11_sensor;
//    dht11_sensor.dht11_pin = CONFIG_DHT11_PIN;
//
//    // Read data
//    while(1)
//    {
//      if(!dht11_read(&dht11_sensor, CONFIG_CONNECTION_TIMEOUT))
//      {  
//        printf("[Temperature]> %.2f \n",dht11_sensor.temperature);
//        printf("[Humidity]> %.2f \n",dht11_sensor.humidity);
//      }
//      vTaskDelay(2000/portTICK_PERIOD_MS);
//    } 

	while(1){
		wind_sensor();
	}
}


extern "C" void app_main()
{	
    
    //Configure DeepSleep
    deep_sleep_register_rtc_timer_wakeup();
    
    printf("Teste\n");
    
    //btManager.turnOn();
		
//	TaskHandle_t xHandleLoRa = NULL;
//    int paramLoRa = 2;
//    xTaskCreate( thread_LoRa, "THREAD_LORA", STACK_SIZE, &paramLoRa, tskIDLE_PRIORITY, &xHandleLoRa );
//    configASSERT( xHandleLoRa );

//	TaskHandle_t xHandleBluetooth= NULL;
//  	int paramBluetooth = 2;
// 	xTaskCreate( thread_Bluetooth, "THREAD_BLUETOOTH", STACK_SIZE, &paramBluetooth, tskIDLE_PRIORITY, &xHandleBluetooth );
// 	configASSERT( xHandleBluetooth );

	TaskHandle_t xHandleSensors= NULL;
  	int paramSensors = 2;
 	xTaskCreate( thread_sensors, "THREAD_SENSORS", STACK_SIZE, &paramSensors, tskIDLE_PRIORITY, &xHandleSensors );
 	configASSERT( xHandleSensors );

}

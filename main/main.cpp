#include <cstring>
#include <stdint.h>
#include <stdio.h>
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
#include "lora.h"

#include "BluetoothManager.h"

#define CODING_RATE 7
#define BANDWIDTH 7
#define SPREAD_FACTOR 11
#define STACK_SIZE 200*32
#define SLEEP_TIME 5
#define BUTTON_GPIO GPIO_NUM_2

BluetoothManager btManager;

static void example_deep_sleep_register_rtc_timer_wakeup(void)
{
    const int wakeup_time_sec = SLEEP_TIME;
    printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
}

void thread_LoRa_Send(void *pvParameters)
{
	int button = 1;
	while(1) {
		button = gpio_get_level(BUTTON_GPIO);
		if(button == 0)
		{
			vTaskDelay(1);
			lora_send_packet((uint8_t*)"Hello", 5);
			printf("package sent\n");
			vTaskDelay(2000 / portTICK_PERIOD_MS);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
	} 
}

void thread_LoRa_Receive(void *pvParameters)
{
	int rssi;
	int snr;
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload 
	while(1) {
		lora_receive(); 
		if (lora_received()) {
			int rxLen = lora_receive_packet(buf, sizeof(buf));
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
			rssi = lora_packet_rssi();
			snr = lora_packet_snr();
			printf("The RSSI value is: %d\n", rssi);
			printf("The SNR value is: %d\n\n", snr);
			vTaskDelay(1); // Avoid WatchDog alerts
			lora_send_packet((uint8_t*)"Received", 8);
			printf("response sent\n");
		}
		vTaskDelay(1); // Avoid WatchDog alerts
	} 
}

void thread_Bluetooth(void *pvParameters)
{
	std::string str;
	int i = 0;
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	while(1) {
		btManager.turnOn();
		for(int j = 0; j<10; j++)
		{
			str = "Olá! Esse é o envio de número " + std::to_string(i); 
	        const uint8_t* data = reinterpret_cast<const uint8_t*>(str.c_str());
	        btManager.sendData(data, str.size());
			printf("New Data!\n");
			btManager.receiveData();
			i++;
			vTaskDelay(3000 / portTICK_PERIOD_MS); 
		}
		btManager.turnOff();
		vTaskDelay(20000 / portTICK_PERIOD_MS); 
	} 
}

extern "C" void app_main()
{	
    
    //Configure DeepSleep
    example_deep_sleep_register_rtc_timer_wakeup();
    
    printf("Teste\n");
    
    
    //btManager.turnOn();
//    
//	if (lora_init() == 0) {
//		printf("Does not recognize the module");
//	}
//	else {
//		printf("LoRa initialized!");
//	}
//	lora_set_frequency(915e6);
//	lora_enable_crc();
//	
//	lora_set_coding_rate(CODING_RATE);
//	lora_set_bandwidth(BANDWIDTH);
//	lora_set_spreading_factor(SPREAD_FACTOR);
//	lora_set_tx_power(17);
//
//  TaskHandle_t xHandleLoRa2 = NULL;
//  int paramLoRa2 = 2;
//  xTaskCreate( thread_LoRa_Receive, "THREAD_LORA", STACK_SIZE, &paramLoRa2, tskIDLE_PRIORITY, &xHandleLoRa2 );
//  configASSERT( xHandleLoRa2 );

	TaskHandle_t xHandleBluetooth= NULL;
  	int paramBluetooth = 2;
 	xTaskCreate( thread_Bluetooth, "THREAD_BLUETOOTH", STACK_SIZE, &paramBluetooth, tskIDLE_PRIORITY, &xHandleBluetooth );
 	configASSERT( xHandleBluetooth );

}

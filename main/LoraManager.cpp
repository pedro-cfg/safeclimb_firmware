#include "LoraManager.h"
#include <cstring>

LoraManager::LoraManager()
{
	tower_number = TOWER_NUMBER;
}

LoraManager::~LoraManager()
{
	
}

void LoraManager::init()
{
	if (lora_init() == 0) {
		printf("Does not recognize the module");
	}
	else {
		lora_set_frequency(915e6);
		lora_enable_crc();
		
		lora_set_coding_rate(CODING_RATE);
		lora_set_bandwidth(BANDWIDTH);
		lora_set_spreading_factor(SPREAD_FACTOR);
		lora_set_tx_power(17);
		printf("LoRa initialized!");
	}
}

void LoraManager::exec()
{
	
	struct timeval now;
    gettimeofday(&now, NULL);
    if((int)(now.tv_sec) - (int)(initial_time.tv_sec) >= GLOBAL_TIMEOUT)
    {
		esp_deep_sleep_start();
	}
    
	switch(state)
	{
		case Send1_:
			Send1();
			break;
		case Send2_:
			Send2();
			break;
		case Receive1_:
			Receive1();
			break;
		case Receive2_:
			Receive2();
			break;
		case Receive3_:
			Receive3();
			break;
		case Receive4_:
			Receive4();
			break;
		default:
			break;
	}
}

void LoraManager::Send1()
{
	struct timeval now;
    gettimeofday(&now, NULL);
    transmission_time = now;
	vTaskDelay(100 / portTICK_PERIOD_MS);
	lora_send_packet(package, package_size);
	
	ESP_LOGI("SEND_1:", "%d byte packet sent:[%.*s]", payload_size, payload_size, payload);
	vTaskDelay(1);
	state = Receive2_;
}

void LoraManager::Send2()
{
	struct timeval now;
    gettimeofday(&now, NULL);
    transmission_time = now;
	vTaskDelay(100 / portTICK_PERIOD_MS);
	uint8_t string[13] = "Confirmation";
	lora_send_packet(string, 13);
	ESP_LOGI("SEND_2:", "%d byte packet sent:[%.*s]", 13, 13, string);
	vTaskDelay(1);
	state = Receive4_;
}

void LoraManager::Receive1()
{
	lora_receive(); 
	if (lora_received()) {
		int rxLen = lora_receive_packet(buf, sizeof(buf));
		package = buf;
		package_size = rxLen;
		receivePackage();
		ESP_LOGI("RECEIVE_1:", "%d byte packet received:[%.*s]", payload_size, payload_size, payload);
//		rssi = lora_packet_rssi();
//		snr = lora_packet_snr();
//		printf("The RSSI value is: %d\n", rssi);
//		printf("The SNR value is: %d\n\n", snr);
		vTaskDelay(1);
		state = Send1_;
	}
}

void LoraManager::Receive2()
{
	struct timeval now;
    gettimeofday(&now, NULL);
	lora_receive();
	if (lora_received()) {
		int rxLen = lora_receive_packet(buf, sizeof(buf));
		ESP_LOGI("RECEIVE_2:", "%d byte packet received", rxLen);
		vTaskDelay(1);
		if(destiny_number == tower_number)
		{
			state = Send2_;
			printf("Package received at destiny. Sending confirmation...");
		}
		else
		{
			state = Receive3_;
		}
	}
	else if((int)(now.tv_sec) - (int)(transmission_time.tv_sec) >= LOCAL_TIMEOUT)
	{
		state = Send1_;
	}
}

void LoraManager::Receive3()
{
	lora_receive(); 
	if (lora_received()) {
		int rxLen = lora_receive_packet(buf, sizeof(buf));
		ESP_LOGI("RECEIVE_3:", "%d byte packet received", rxLen);
		vTaskDelay(1);
		state = Send2_;
	}
}

void LoraManager::Receive4()
{
	if(sender)
	{
		printf("Communication Successful");	
		esp_deep_sleep_start();
	}
	struct timeval now;
    gettimeofday(&now, NULL);
	lora_receive();
	if (lora_received()) {
		int rxLen = lora_receive_packet(buf, sizeof(buf));
		ESP_LOGI("RECEIVE_4:", "%d byte packet received", rxLen);
		vTaskDelay(1);
		esp_deep_sleep_start();
	}
	else if((int)(now.tv_sec) - (int)(transmission_time.tv_sec) >= LOCAL_TIMEOUT)
	{
		state = Send2_;
	}
}


void LoraManager::setInitialTime(struct timeval time)
{
	initial_time = time;
}

void LoraManager::sendPackage(uint8_t* pck, int size, int destiny)
{
	payload = pck;
	payload_size = size;
	destiny_number = destiny;
	encapsulate();
	
	sender = true;
	state = Send1_;
}

void LoraManager::receivePackage()
{
	state = Receive1_;
}

void LoraManager::encapsulate()
{
	int header_size = 1;
	int new_len = payload_size + header_size;
	
	uint8_t pack[new_len];
	pack[0] = destiny_number;
	memcpy(pack + header_size, payload, payload_size);	
	
	package = pack;
	package_size = new_len;
}

void LoraManager::getPayload()
{
	int header_size = 1;
	int new_len = package_size - header_size;
	
	uint8_t pay[new_len];
	memcpy(pay, package + header_size, new_len);	
	
	destiny_number = package[0];
	payload = pay;
	payload_size = new_len;
	
}
	

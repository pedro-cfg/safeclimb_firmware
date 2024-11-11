#include "LoraManager.h"
#include <cstring>

LoraManager::LoraManager()
{
	tower_number = TOWER_NUMBER;
	LoraPackage actualPackage;
	LoraPackage receivedPackage;
	LoraPackage newPackage;
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
	
    gettimeofday(&now, NULL);
    if((int)(now.tv_sec) - (int)(initial_time.tv_sec) >= GLOBAL_TIMEOUT)
    {
		Sleep();
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
    gettimeofday(&now, NULL);
    transmission_time = now;
	//vTaskDelay(100 / portTICK_PERIOD_MS);
	lora_send_packet(actualPackage.getPackage(), actualPackage.getPackageSize());
	
	ESP_LOGI("SEND_1:", "%d byte packet sent:[%.*s]", actualPackage.getPayloadSize(), actualPackage.getPayloadSize(), actualPackage.getPayload());
	vTaskDelay(1);
	state = Receive2_;
}

void LoraManager::Send2()
{
    gettimeofday(&now, NULL);
    transmission_time = now;
	//vTaskDelay(100 / portTICK_PERIOD_MS);
	newPackage.setPayload((uint8_t*)"CONFIRMATION",13,actualPackage.getDestinyNumber());
	lora_send_packet(newPackage.getPackage(), newPackage.getPackageSize());
	ESP_LOGI("SEND_2:", "%d byte packet sent:[%.*s]", newPackage.getPayloadSize(),newPackage.getPayloadSize(),newPackage.getPayload());
	vTaskDelay(1);
	state = Receive4_;
}

void LoraManager::Receive1()
{
	lora_receive(); 
	if (lora_received()) {
		int rxLen = lora_receive_packet(buf, sizeof(buf));
		actualPackage.setPackage(buf,rxLen);
		ESP_LOGI("RECEIVE_1:", "%d byte packet received:[%.*s]", actualPackage.getPayloadSize(), actualPackage.getPayloadSize(), actualPackage.getPayload());
//		rssi = lora_packet_rssi();
//		snr = lora_packet_snr();
//		printf("The RSSI value is: %d\n", rssi);
//		printf("The SNR value is: %d\n\n", snr);
		vTaskDelay(500 / portTICK_PERIOD_MS);
		state = Send1_;
	}
}

void LoraManager::Receive2()
{
	if(actualPackage.getDestinyNumber() == tower_number)
	{
		state = Send2_;
		printf("Package received at destiny. Sending confirmation...");
		vTaskDelay(4000 / portTICK_PERIOD_MS);
	}
	else
	{
		lora_receive();
	    gettimeofday(&now, NULL);
		if (lora_received()) {
			int rxLen = lora_receive_packet(buf, sizeof(buf));
			receivedPackage.setPackage(buf,rxLen);
			ESP_LOGI("RECEIVE_2:", "%d byte packet received:[%.*s]", receivedPackage.getPayloadSize(), receivedPackage.getPayloadSize(), receivedPackage.getPayload());
			vTaskDelay(1);
			if(memcmp(receivedPackage.getPayload(), actualPackage.getPayload(), actualPackage.getPayloadSize()) == 0)
			{
				state = Receive3_;
			}
			else if(memcmp((uint8_t*)"CONFIRMATION", receivedPackage.getPayload(), receivedPackage.getPayloadSize()) == 0)
			{
				lora_send_packet((uint8_t*)"", 1);
				vTaskDelay(500 / portTICK_PERIOD_MS);
				state = Send2_;
			}
		
		}
		else if((int)(now.tv_sec) - (int)(transmission_time.tv_sec) >= LOCAL_TIMEOUT)
		{
			state = Send1_;
		}
			
	}
	
}

void LoraManager::Receive3()
{
	lora_receive(); 
	if (lora_received()) {
		int rxLen = lora_receive_packet(buf, sizeof(buf));
		receivedPackage.setPackage(buf,rxLen);
		ESP_LOGI("RECEIVE_3:", "%d byte packet received:[%.*s]", receivedPackage.getPayloadSize(), receivedPackage.getPayloadSize(), receivedPackage.getPayload());
		if(memcmp((uint8_t*)"CONFIRMATION", receivedPackage.getPayload(), receivedPackage.getPayloadSize()) == 0)
		{
			lora_send_packet((uint8_t*)"", 1);
			vTaskDelay(500 / portTICK_PERIOD_MS);
			state = Send2_;
		}
	}
}

void LoraManager::Receive4()
{
	if(sender)
	{
		printf("Communication Successful");	
		Sleep();
	}
	lora_receive();
	
    gettimeofday(&now, NULL);
	if (lora_received()) {
		int rxLen = lora_receive_packet(buf, sizeof(buf));
		ESP_LOGI("RECEIVE_4:", "%d byte packet received", rxLen);
		vTaskDelay(1);
		Sleep();
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
	actualPackage.setPayload(pck,size,destiny);
	sender = true;
	state = Send1_;
}

void LoraManager::receivePackage()
{
	state = Receive1_;
}
	
void LoraManager::Sleep()
{
	ESP_LOGI("LORA:", "Entering Sleep Mode");
	lora_sleep();
	lora_close();
	esp_deep_sleep_start();
//	gettimeofday(&now, NULL);
//	setInitialTime(now);
//	sendPackage((uint8_t*)"Opa",3, 0);
}


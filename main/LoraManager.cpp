#include "LoraManager.h"
#include "driver/rtc_io.h"
#include "esp_task_wdt.h"
#include <cstring>

LoraManager::LoraManager()
{
	tower_number = TOWER_NUMBER;
	LoraPackage actualPackage;
	LoraPackage receivedPackage;
	LoraPackage newPackage;
	keepAlive = false;
	data_bluetooth = false;
	teste = false;
}

LoraManager::~LoraManager()
{
	
}

void LoraManager::init(WIFI* w)
{
	wifi = w;
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
		transmitting = false;
		printf("LoRa initialized!");
	}
}

void LoraManager::exec()
{
    gettimeofday(&now, NULL);
    
    if(keepAlive)
    {
		if((int)(now.tv_sec) - (int)(initial_time.tv_sec) >= BLUETOOTH_TIMEOUT)
	    {
			setKeepAlive(false);
			if(state != Receive_Payload_)
				Sleep();
		}
	}
    else if((int)(now.tv_sec) - (int)(initial_time.tv_sec) >= GLOBAL_TIMEOUT)
    {
		if(state == Receive_Return_Confirm_ || state == Receive_Return_ || state == Send_Return_)
		{
			if(actualPackage.getDestinyNumber() == tower_number)
			{
				consumeInfo();
			}
		}
		if(state != Receive_Payload_)
			Sleep();
	}
    
	switch(state)
	{
		case Send_Payload_:
			Send_Payload();
			break;
		case Send_Confirm_Primary_:
			Send_Confirm_Primary();
			break;
		case Send_Return_:
			Send_Return();
			break;
		case Receive_Payload_:
			Receive_Payload();
			break;
		case Receive_Confirm_Primary_:
			Receive_Confirm_Primary();
			break;
		case Receive_Return_:
			Receive_Return();
			break;
		case Receive_Return_Confirm_:
			Receive_Return_Confirm();
			break;
		default:
			break;
	}
}

void LoraManager::Send_Payload()
{
	lora_send_packet(actualPackage.getPackage(), actualPackage.getPackageSize());
	
	int package_time = actualPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME;
	vTaskDelay(package_time / portTICK_PERIOD_MS);
    
    gettimeofday(&now, NULL);
	transmission_time = now;
	ESP_LOGI("SEND_1:", "%d byte packet sent:[%.*s]", actualPackage.getPayloadSize(), actualPackage.getPayloadSize(), actualPackage.getPayload());
	changeState(Receive_Confirm_Primary_);
}

void LoraManager::Send_Confirm_Primary()
{
	newPackage.setPayload((uint8_t*)"OK",3,actualPackage.getDestinyNumber(),tower_number);
	lora_send_packet(newPackage.getPackage(), newPackage.getPackageSize());
	
	int package_time = newPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME;
	vTaskDelay(package_time / portTICK_PERIOD_MS);
    
    gettimeofday(&now, NULL);
    transmission_time = now;
	ESP_LOGI("Send_Confirm_Primary:", "%d byte packet sent:[%.*s]", newPackage.getPayloadSize(),newPackage.getPayloadSize(),newPackage.getPayload());

	if(actualPackage.getDestinyNumber() == tower_number)
	{
		changeState(Send_Return_);
		printf("Package received at destiny. Sending confirmation...");
		//consumeInfo();
	}
	else
	{
		changeState(Send_Payload_);
	}
}

void LoraManager::Send_Return()
{
	newPackage.setPayload((uint8_t*)"R",2,actualPackage.getDestinyNumber(),tower_number);
	lora_send_packet(newPackage.getPackage(), newPackage.getPackageSize());
	
	int package_time = newPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME;
	vTaskDelay(package_time / portTICK_PERIOD_MS);
	
    gettimeofday(&now, NULL);
    transmission_time = now;
	ESP_LOGI("Send_Return:", "%d byte packet sent:[%.*s]", newPackage.getPayloadSize(),newPackage.getPayloadSize(),newPackage.getPayload());

	changeState(Receive_Return_Confirm_);
}

void LoraManager::Receive_Payload()
{
	lora_receive(); 
	if (lora_received()) {
		int rxLen = lora_receive_packet(buf, sizeof(buf));
		actualPackage.setPackage(buf,rxLen);
		ESP_LOGI("RECEIVE_1:", "%d byte packet received:[%.*s]", actualPackage.getPayloadSize(), actualPackage.getPayloadSize(), actualPackage.getPayload());
		
		int received_sum = actualPackage.getSum();
		actualPackage.calculateSum();
		int calculated_sum = actualPackage.getSum();
		printf("Received: %d, Calculated: %d\n",received_sum,calculated_sum);
		
		if(received_sum == calculated_sum)
		{
			if(actualPackage.getKeepAlive())
				setKeepAlive(actualPackage.getKeepAlive());
			if(actualPackage.getIsInfo())
			{
				int package_time = (actualPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME);
				vTaskDelay(package_time / portTICK_PERIOD_MS);
				
				changeState(Send_Confirm_Primary_);
				actualPackage.setSenderNumber(tower_number);
				
				transmitting = true;
				gettimeofday(&now, NULL);
				setInitialTime(now);	
			}
			
		}
		
		//		rssi = lora_packet_rssi();
//		snr = lora_packet_snr();
//		printf("The RSSI value is: %d\n", rssi);
//		printf("The SNR value is: %d\n\n", snr);
	}
	vTaskDelay(1);
}

void LoraManager::Receive_Confirm_Primary()
{
	lora_receive();
    gettimeofday(&now, NULL);
	if (lora_received()) {
		int rxLen = lora_receive_packet(buf, sizeof(buf));
		receivedPackage.setPackage(buf,rxLen);
		ESP_LOGI("Receive_Confirm_Primary:", "%d byte packet received:[%.*s]", receivedPackage.getPayloadSize(), receivedPackage.getPayloadSize(), receivedPackage.getPayload());
		
		if((receivedPackage.getDestinyNumber() < tower_number && receivedPackage.getSenderNumber() < tower_number) || (receivedPackage.getDestinyNumber() > tower_number && receivedPackage.getSenderNumber() > tower_number))
		{
			int package_time = (receivedPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME)*0.2;
			vTaskDelay(package_time / portTICK_PERIOD_MS);
			
			if(memcmp(receivedPackage.getPayload(), actualPackage.getPayload(), actualPackage.getPayloadSize()) == 0 || memcmp((uint8_t*)"OK", receivedPackage.getPayload(), receivedPackage.getPayloadSize()) == 0)
			{
				changeState(Receive_Return_);
			}
			else if(memcmp((uint8_t*)"R", receivedPackage.getPayload(), receivedPackage.getPayloadSize()) == 0)
			{
				changeState(Send_Return_);
			}
		}
		
	}
	else if((int)(now.tv_sec) - (int)(transmission_time.tv_sec) >= LOCAL_TIMEOUT)
	{
		changeState(Send_Payload_);
	}
	vTaskDelay(1);
}

void LoraManager::Receive_Return()
{
	lora_receive(); 
	if (lora_received()) {
		int rxLen = lora_receive_packet(buf, sizeof(buf));
		receivedPackage.setPackage(buf,rxLen);
		
		if((receivedPackage.getDestinyNumber() < tower_number && receivedPackage.getSenderNumber() < tower_number) || (receivedPackage.getDestinyNumber() > tower_number && receivedPackage.getSenderNumber() > tower_number))
		{
			int package_time = (receivedPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME);
			vTaskDelay(package_time / portTICK_PERIOD_MS);
			
			ESP_LOGI("Receive_Return:", "%d byte packet received:[%.*s]", receivedPackage.getPayloadSize(), receivedPackage.getPayloadSize(), receivedPackage.getPayload());
			
			if(memcmp((uint8_t*)"R", receivedPackage.getPayload(), receivedPackage.getPayloadSize()) == 0)
			{
				changeState(Send_Return_);
			}
		}
	}
	vTaskDelay(1);
}

void LoraManager::Receive_Return_Confirm()
{
	if(sender)
	{
		printf("Communication Successful\n");		
		Sleep();
	}
	
	lora_receive();
    gettimeofday(&now, NULL);
	if (lora_received()) {
		int rxLen = lora_receive_packet(buf, sizeof(buf));
		receivedPackage.setPackage(buf,rxLen);
		
		if((receivedPackage.getDestinyNumber() <= tower_number && receivedPackage.getSenderNumber() > tower_number) || (receivedPackage.getDestinyNumber() >= tower_number && receivedPackage.getSenderNumber() < tower_number))
		{
			ESP_LOGI("Receive_Return_Confirm:", "%d byte packet received", rxLen);
			vTaskDelay(1);
			if(memcmp((uint8_t*)"R", receivedPackage.getPayload(), receivedPackage.getPayloadSize()) == 0)
			{
				consumeInfo();
				Sleep();
			}
		}
	}
	else if((int)(now.tv_sec) - (int)(transmission_time.tv_sec) >= LOCAL_TIMEOUT)
	{
		changeState(Send_Return_);
	}
	vTaskDelay(1);
}


void LoraManager::setInitialTime(struct timeval time)
{
	initial_time = time;
}

void LoraManager::sendPackage(uint8_t* pck, int size, int destiny,bool txt, bool ka, int temp, int ah,int sh,int ws,int rn)
{
	while(transmitting){}
	actualPackage.setPayload(pck,size,destiny,tower_number,ka,true,txt,temp,ah,sh,ws,rn);
	sender = true;
	transmitting = true;
	changeState(Send_Payload_);
}

void LoraManager::receivePackage()
{
	changeState(Receive_Payload_);
}
	
void LoraManager::Sleep()
{
	transmitting = false;
	sender = false;
	if(!keepAlive)
	{
		ESP_LOGI("LORA:", "Entering Sleep Mode");
		lora_sleep();
		lora_close();
		rtc_gpio_isolate(GPIO_NUM_12);
		esp_deep_sleep_start();
	}
	else
	{
		gettimeofday(&now, NULL);
		setInitialTime(now);
		changeState(Receive_Payload_);
	}
	
}

void LoraManager::changeState(State s)
{
	state = s;
    ESP_LOGI("LORA", "Changing state: %s", stateToString(s));
}

const char* LoraManager::stateToString(State s)
{
    switch (s) {
		case Send_Payload_: return "Send_Payload";
		case Send_Confirm_Primary_: return "Send_Confirm_Primary";
		case Send_Return_: return "Send_Return";
		case Receive_Payload_: return "Receive_Payload";
		case Receive_Confirm_Primary_: return "Receive_Confirm_Primary";
		case Receive_Return_: return "Receive_Return";
		case Receive_Return_Confirm_: return "Receive_Return_Confirm";
		default: return "UNKNOWN";
    }
}

bool LoraManager::getTransmitting()
{
	return transmitting;
}

void LoraManager::setKeepAlive(bool ka)
{
	printf("TURNING KEEP ALIVE ");
	if(ka)
	{
		printf("ON!\n");
		gettimeofday(&now, NULL);
		setInitialTime(now);
	}
	else
		printf("OFF!\n");
	keepAlive = ka;
}

void LoraManager::consumeInfo()
{
	if(actualPackage.getIsText())
	{
		printf("Mensagem recebida: [%.*s]\n", actualPackage.getPayloadSize(),actualPackage.getPayload());
		if(SERVER_TOWER)
		{
			bluetooth_tower = actualPackage.getOriginalNumber();
			//printf("Enviando mensagem para a torre %d",bluetooth_tower);
			//Código para mandar para o twilio
			teste = true;
		}
		else
		{
			data_bluetooth = true;
			//Enviar para smartphone
		}
	}
	else
	{
		printf("TEMPERATURE: %.1f\n", ((float)actualPackage.getTemperature())/10.0);
		printf("AIR HUMIDITY: %d\n", actualPackage.getAirHumidity());
		printf("SOIL HUMIDITY: %d\n", actualPackage.getSoilHumidity());
		printf("WIND SPEED: %d\n", actualPackage.getWindSpeed());
		printf("RAIN: %d\n", actualPackage.getRain());	
		printf("Mensagem recebida: [%.*s]\n", actualPackage.getPayloadSize(),actualPackage.getPayload());
		wifi->MQTTpublish(actualPackage.getTemperature(),actualPackage.getAirHumidity(),actualPackage.getSoilHumidity(),actualPackage.getWindSpeed(),actualPackage.getRain());
	}
				
}

bool LoraManager::getMessageBluetoothReady()
{
	if(data_bluetooth)
	{
		data_bluetooth = false;
		return true;
	}
	return false;
}

uint8_t* LoraManager::getMessageBluetooth()
{
	return actualPackage.getPayload();
}

int LoraManager::getMessageBluetoothSize()
{
	return actualPackage.getPayloadSize();
}

int LoraManager::getBluetoothTower()
{
	return bluetooth_tower;
}


#include "LoraManager.h"
#include "esp_random.h"
#include "driver/rtc_io.h"
#include "esp_system.h"
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
	higher_order = false;
	transmission = 0;
	repeaterBattery = 0;
	timeout = (esp_random()%10)+5;
//	teste = false;
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
	higher_order = false;
    gettimeofday(&now, NULL);
    
    if(keepAlive)
    {
		if((int)(now.tv_sec) - (int)(initial_time.tv_sec) >= BLUETOOTH_TIMEOUT)
	    {
			setKeepAlive(false);
			Sleep();
		}
	}
    else if((int)(now.tv_sec) - (int)(initial_time.tv_sec) >= GLOBAL_TIMEOUT)
    {
		if(state == Receive_Return_Confirm_ || state == Receive_Return_ || state == Send_Return_)
		{
//			if(actualPackage.getDestinyNumber() == tower_number)
//			{
//				consumeInfo();
//			}
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
	lora_idle();
	vTaskDelay(100 / portTICK_PERIOD_MS);
	lora_send_packet(actualPackage.getPackage(), actualPackage.getPackageSize());
	
	int package_time = actualPackage.getPackageSize() * BYTE_TIME;
	vTaskDelay(package_time / portTICK_PERIOD_MS);
    
    gettimeofday(&now, NULL);
	transmission_time = now;
	ESP_LOGI("SEND_1:", "%d byte packet sent:[%.*s]", actualPackage.getPayloadSize(), actualPackage.getPayloadSize(), actualPackage.getPayload());
	changeState(Receive_Confirm_Primary_);
}

void LoraManager::Send_Confirm_Primary()
{
	lora_idle();
	vTaskDelay(100 / portTICK_PERIOD_MS);
	newPackage.setPayload((uint8_t*)"OK",3,actualPackage.getDestinyNumber(),tower_number);
	lora_send_packet(newPackage.getPackage(), newPackage.getPackageSize());
	
	int package_time = newPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME;
	vTaskDelay(package_time / portTICK_PERIOD_MS);
    
    gettimeofday(&now, NULL);
    transmission_time = now;
	ESP_LOGI("Send_Confirm_Primary:", "%d byte packet sent:[%.*s]", newPackage.getPayloadSize(),newPackage.getPayloadSize(),newPackage.getPayload());

	if(actualPackage.getDestinyNumber() == tower_number)
	{
		//vTaskDelay(1500 / portTICK_PERIOD_MS);
		changeState(Send_Return_);
		printf("Package received at destiny. Sending confirmation...");
		consumeInfo();
	}
	else
	{
		changeState(Send_Payload_);
	}
}

void LoraManager::Send_Return()
{
	lora_idle();
	vTaskDelay(100 / portTICK_PERIOD_MS);
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
		ESP_LOGI("RECEIVE_1:", "%d byte packet received:[%.*s] from tower %d", actualPackage.getPayloadSize(), actualPackage.getPayloadSize(), actualPackage.getPayload(),actualPackage.getSenderNumber());
		if((!actualPackage.getIsText() && keepAlive) || transmitting || (sender && actualPackage.getSenderNumber() > tower_number && actualPackage.getDestinyNumber() <= tower_number))
		{
			printf("Invalid transmission\n");
		}		
		else if((actualPackage.getDestinyNumber() <= tower_number && actualPackage.getSenderNumber() == tower_number + 1) || (actualPackage.getDestinyNumber() >= tower_number && actualPackage.getSenderNumber() == tower_number - 1))
		{
		
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
					if(!actualPackage.getIsText() && repeaterBattery != 0)
						actualPackage.setBattery2(repeaterBattery);
					int package_time = (actualPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME)*0.8;
					vTaskDelay(package_time / portTICK_PERIOD_MS);
					
					changeState(Send_Confirm_Primary_);
					actualPackage.setSenderNumber(tower_number);
					
					transmitting = true;
					gettimeofday(&now, NULL);
					setInitialTime(now);	
				}
				
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
    
	if (lora_received()) {
		gettimeofday(&now, NULL);
		int rxLen = lora_receive_packet(buf, sizeof(buf));
		receivedPackage.setPackage(buf,rxLen);
		ESP_LOGI("Receive_Confirm_Primary:", "%d byte packet received:[%.*s] from tower %d", receivedPackage.getPayloadSize(), receivedPackage.getPayloadSize(), receivedPackage.getPayload(), receivedPackage.getSenderNumber());
	
		if(receivedPackage.getIsInfo() && receivedPackage.getKeepAlive())
		{
			setKeepAlive(receivedPackage.getKeepAlive());
			if((receivedPackage.getSenderNumber() > tower_number && receivedPackage.getDestinyNumber() > tower_number) || (receivedPackage.getSenderNumber() < tower_number && receivedPackage.getDestinyNumber() < tower_number))
			{
				printf("Invalid transmission\n");
			}
			else {
				int package_time = (receivedPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME)*0.8;
				vTaskDelay(package_time / portTICK_PERIOD_MS);
				changeState(Send_Confirm_Primary_);
				receivedPackage.setSenderNumber(tower_number);
				actualPackage = receivedPackage;
				transmitting = true;
				gettimeofday(&now, NULL);
				setInitialTime(now);	
			}
		}		
		else if((!receivedPackage.getIsText() && keepAlive) || (sender && receivedPackage.getSenderNumber() > tower_number && receivedPackage.getDestinyNumber() <= tower_number))
		{
			printf("Invalid transmission\n");
		}	
		else if((receivedPackage.getDestinyNumber() < tower_number && receivedPackage.getSenderNumber() == tower_number - 1) || (receivedPackage.getDestinyNumber() > tower_number && receivedPackage.getSenderNumber() == tower_number + 1))
		{
			//int package_time = (receivedPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME)*0.2;
			//vTaskDelay(package_time / portTICK_PERIOD_MS);
			
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
	else if((int)(now.tv_sec) - (int)(transmission_time.tv_sec) >= timeout)
	{
		timeout = (esp_random()%10)+5;
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
		
		ESP_LOGI("Receive_Return:", "%d byte packet received:[%.*s] from tower %d", receivedPackage.getPayloadSize(), receivedPackage.getPayloadSize(), receivedPackage.getPayload(), receivedPackage.getSenderNumber());
		
		if(receivedPackage.getIsInfo() && receivedPackage.getKeepAlive())
		{
			setKeepAlive(receivedPackage.getKeepAlive());
			if((receivedPackage.getSenderNumber() > tower_number && receivedPackage.getDestinyNumber() > tower_number) || (receivedPackage.getSenderNumber() < tower_number && receivedPackage.getDestinyNumber() < tower_number))
			{
				printf("Invalid transmission\n");
			}
			else {
				int package_time = (receivedPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME)*0.8;
				vTaskDelay(package_time / portTICK_PERIOD_MS);
				changeState(Send_Confirm_Primary_);
				receivedPackage.setSenderNumber(tower_number);
				actualPackage = receivedPackage;
				transmitting = true;
				gettimeofday(&now, NULL);
				setInitialTime(now);	
			}
		}			
		else if(sender && receivedPackage.getSenderNumber() > tower_number && receivedPackage.getDestinyNumber() <= tower_number)
		{
			printf("Invalid transmission\n");
		}	
		else if((receivedPackage.getDestinyNumber() <= tower_number && receivedPackage.getSenderNumber() == tower_number - 1) || (receivedPackage.getDestinyNumber() >= tower_number && receivedPackage.getSenderNumber() == tower_number + 1))
		{
			int package_time = (receivedPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME)*0.5;
			vTaskDelay(package_time / portTICK_PERIOD_MS);
			
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
		
		ESP_LOGI("Receive_Return_Confirm:", "%d byte packet received:[%.*s] from tower %d", receivedPackage.getPayloadSize(), receivedPackage.getPayloadSize(), receivedPackage.getPayload(), receivedPackage.getSenderNumber());
		
		if(receivedPackage.getIsInfo() && receivedPackage.getKeepAlive())
		{
			setKeepAlive(receivedPackage.getKeepAlive());
			if((receivedPackage.getSenderNumber() > tower_number && receivedPackage.getDestinyNumber() > tower_number) || (receivedPackage.getSenderNumber() < tower_number && receivedPackage.getDestinyNumber() < tower_number))
			{
				printf("Invalid transmission\n");
			}
			else {
				int package_time = (receivedPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME)*0.8;
				vTaskDelay(package_time / portTICK_PERIOD_MS);
				changeState(Send_Confirm_Primary_);
				receivedPackage.setSenderNumber(tower_number);
				actualPackage = receivedPackage;
				transmitting = true;
				gettimeofday(&now, NULL);
				setInitialTime(now);	
			}
		}		
		else if(sender && receivedPackage.getSenderNumber() > tower_number && receivedPackage.getDestinyNumber() <= tower_number)
		{
			printf("Invalid transmission\n");
		}		
		else if((receivedPackage.getDestinyNumber() <= tower_number && receivedPackage.getSenderNumber() == tower_number + 1) || (receivedPackage.getDestinyNumber() >= tower_number && receivedPackage.getSenderNumber() == tower_number - 1))
		{

			int package_time = (receivedPackage.getPackageSize() * BYTE_TIME + EXTRA_TIME)*0.5;
			vTaskDelay(package_time / portTICK_PERIOD_MS);
			
			if(memcmp((uint8_t*)"R", receivedPackage.getPayload(), receivedPackage.getPayloadSize()) == 0)
			{
				//if(actualPackage.getDestinyNumber() == tower_number)
					//consumeInfo();
				Sleep();
			}
		}
	}
	else if((int)(now.tv_sec) - (int)(transmission_time.tv_sec) >= timeout)
	{
		timeout = (esp_random()%10)+5;
		changeState(Send_Return_);
	}
	vTaskDelay(1);
}


void LoraManager::setInitialTime(struct timeval time)
{
	initial_time = time;
}

void LoraManager::sendPackage(uint8_t* pck, int size, int destiny,int txt, bool ka, int temp, int ah,int sh,int ws,int rn,int b1, int b2)
{
	while(transmitting){}
	actualPackage.setPayload(pck,size,destiny,tower_number,ka,true,txt,temp,ah,sh,ws,rn,b1,b2);
	sender = true;
	transmitting = true;
	if(ka)
		state = Send_Payload_;
	else
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
//		if(MAIN_TOWER && !keepAlive)
//		{
//			vTaskDelay(300000 / portTICK_PERIOD_MS);
//			esp_restart();
//		}
//		else
//		{
			changeState(Receive_Payload_);
			Receive_Payload();
//		}
			
	}
	
}

void LoraManager::changeState(State s)
{
	if(!higher_order)
	{
		state = s;
    	ESP_LOGI("LORA", "Changing state: %s", stateToString(s));
	}
	else
		ESP_LOGI("LORA", "State: %s not changed because of higher priority order", stateToString(s));
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
	{
		printf("OFF!\n");
	}
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
			if(actualPackage.getIsText() == 2)
			{
				uint8_t* formatted = formatPhoneNumber(actualPackage.getPayload());
				size_t formattedLength = strlen((char*)formatted);
				memcpy(telephone, formatted, formattedLength + 1);
				printf("Telephone: %s",telephone);
				wifi->setHelloMessage(true);
			}
			else if(actualPackage.getIsText() == 3)
			{
				snprintf(message+transmission,actualPackage.getPayloadSize()+1,"%s",actualPackage.getPayload());
				transmission += actualPackage.getPayloadSize();
				
				wifi->MQTTpublishText(telephone, (uint8_t*)message,sizeof(message));
				
				memset(message, 0, sizeof(message));
            	transmission = 0;
			}
			else if(actualPackage.getIsText() == 1)
			{
				snprintf(message+transmission,actualPackage.getPayloadSize()+1,"%s",actualPackage.getPayload());
				transmission += actualPackage.getPayloadSize();
			}
			//teste = true;
		}
		else
		{
			if (actualPackage.getIsText() == 3) {
			    printf("Última transmissão!\n");
			
		        snprintf(message + transmission, actualPackage.getPayloadSize() + 1, "%s", actualPackage.getPayload());
		        transmission += actualPackage.getPayloadSize();
		
		        bluetooth->sendData((uint8_t*)message, transmission);
		
		        memset(message, 0, sizeof(message));
		        transmission = 0;
			    
			} else if (actualPackage.getIsText() == 1) {
			    printf("Apenas concatenando\n");
		        snprintf(message + transmission, actualPackage.getPayloadSize() + 1, "%s", actualPackage.getPayload());
		        transmission += actualPackage.getPayloadSize();

			}
			
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
		printf("BATTERY MAIN: %d\n", actualPackage.getBatt1());	
		printf("BATTERY REPEATER: %d\n", actualPackage.getBatt2());	
		printf("Mensagem recebida: [%.*s]\n", actualPackage.getPayloadSize(),actualPackage.getPayload());
		wifi->MQTTpublish(actualPackage.getTemperature(),actualPackage.getAirHumidity(),actualPackage.getSoilHumidity(),actualPackage.getWindSpeed(),actualPackage.getRain(), actualPackage.getBatt1(), actualPackage.getBatt2());
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

uint8_t* LoraManager::formatPhoneNumber(const uint8_t* input) {

    uint8_t* result = new uint8_t[15]; 
    std::size_t index = 0;

    result[index++] = '+';
    result[index++] = '5';
    result[index++] = '5';

    for (std::size_t i = 0; input[i] != '\0'; ++i) {
        if (std::isdigit(input[i])) { 
            result[index++] = input[i];
        }
    }

    result[index] = '\0';

    return result;
}

void LoraManager::setBluetoothManager(BluetoothManager* b)
{
	bluetooth = b;
}

void LoraManager::setTransmitting(bool transm)
{
	transmitting = transm;
}

void LoraManager::setHigherOrder(bool higher)
{
	higher_order = higher;
}

void LoraManager::setRepeaterBattery(int batt)
{
	repeaterBattery = batt;
}

void LoraManager::interrupt()
{
	state = Receive_Payload_;
	transmitting = false;
	sender = false;
	higher_order = true;
	//Receive_Payload();
}

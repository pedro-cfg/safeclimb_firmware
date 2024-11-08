#include "BluetoothManager.h"
#include <cstdio>
#include <string.h>
#include <string>

BluetoothManager::BluetoothManager()
{
	initBluetooth();
	telephone_set = false;
}

BluetoothManager::~BluetoothManager()
{
	disableBluetooth();
}

void BluetoothManager::turnOn()
{
	enableBluetooth();
}

void BluetoothManager::turnOff()
{
	disableBluetooth();
}

void BluetoothManager::sendData(const uint8_t* data, int size)
{
	const int max_chunk_size = 20; 
    int offset = 0; 

    while (offset < size) {
        int chunk_size = std::min(max_chunk_size, size - offset);
        uint8_t str[max_chunk_size];
        for(int i = 0;i<chunk_size;i++)
        {
			str[i] = data[offset+i];
		}
        update_and_notify_data(str, chunk_size);
        offset += chunk_size;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

extern "C" void BluetoothManager::receiveData()
{
	if(new_data)
	{
		//new_data = 0;
		if(!telephone_set)
		{
			memcpy(telephone, global_data, 15);
			printf("Telephone: %s",telephone);
			telephone_set = true;
		}
		printf("Recebido: \n%s\n", global_data);	
		//snprintf(global_data, sizeof(global_data), "%s", "");
	}    
	
}
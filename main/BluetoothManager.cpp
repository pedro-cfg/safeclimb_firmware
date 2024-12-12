#include "BluetoothManager.h"
#include <cstdio>
#include <string.h>
#include <string>

BluetoothManager::BluetoothManager()
{
	initBluetooth();
	telephone_set = false;
	data_received = false;
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
        for(int i = 0; i < chunk_size; i++) {
            str[i] = data[offset + i];
        }
        update_and_notify_data(str, chunk_size);
        offset += chunk_size;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    uint8_t null_terminator[1] = { '\0' };
    update_and_notify_data(null_terminator, 1);
}


extern "C" void BluetoothManager::receiveData()
{
	if (new_data)
	{
	    new_data = 0;
	    data_received = true;
	
	    if (global_data[0] == '5' && global_data[1] == '5')
	    {
	        memcpy(telephone, global_data + 2, sizeof(telephone) - 1);
	        telephone[sizeof(telephone) - 1] = '\0';
	        printf("Telephone (sem 55): %s\n", telephone);
	        telephone_set = true;
	        data = (char*)telephone;
	    }
	    else
	    {
	        data = global_data;
	    }
	
	    
	} 
	
}

bool BluetoothManager::dataReceived()
{
	bool aux = data_received;
	data_received = false;
	return aux;
}

std::string BluetoothManager::getData()
{
	return data;
}

bool BluetoothManager::getTelephoneBool()
{
	return telephone_set;
}

void BluetoothManager::setTelephoneBool(bool tel)
{
	telephone_set = tel;
}

    
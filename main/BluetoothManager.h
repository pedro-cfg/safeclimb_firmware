#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string>

#include "bluetooth.h"

class BluetoothManager {
private:
	uint8_t telephone[15];
	bool telephone_set;
	bool data_received;
	std::string data;
public:
    BluetoothManager();
    ~BluetoothManager();
    void turnOn();
    void turnOff();
    void sendData(const uint8_t* data, int size);
    void receiveData();
    bool dataReceived();
    std::string getData();
};

#endif // BLUETOOTH_MANAGER_H

#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <stdio.h>
#include <math.h>

#include "bluetooth.h"

class BluetoothManager {
private:

public:
    BluetoothManager();
    ~BluetoothManager();
    void turnOn();
    void turnOff();
    void sendData(const uint8_t* data, int size);
    void receiveData();
};

#endif // BLUETOOTH_MANAGER_H

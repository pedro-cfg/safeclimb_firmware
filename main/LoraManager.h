#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include <stdio.h>
#include "lora.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>
#include <sys/time.h>
#include "esp_sleep.h"
#include "parameters.h"
#include "LoraPackage.h"
#include "WIFI.h"
#include "BluetoothManager.h"

class LoraManager {
private:
	enum State {
	    Send_Payload_,
	    Send_Confirm_Primary_,
	    Send_Return_,
	    Receive_Payload_,
	    Receive_Confirm_Primary_,
	    Receive_Return_,
	    Receive_Return_Confirm_
	};

	State state;
	uint8_t buf[256];
	struct timeval initial_time;
	struct timeval transmission_time;
	int tower_number;
	bool sender;
	struct timeval now;
	bool keepAlive;
	bool transmitting;
	int bluetooth_tower;
	bool data_bluetooth;
	bool higher_order;
	WIFI* wifi;
	BluetoothManager* bluetooth;
	int timeout;
	
	uint8_t telephone[15];
	
	LoraPackage actualPackage;
	LoraPackage receivedPackage;
	LoraPackage newPackage;
	
	void Send_Payload();
	void Send_Confirm_Primary();
	void Send_Return();
	void Receive_Payload();
	void Receive_Confirm_Primary();
	void Receive_Return();
	void Receive_Return_Confirm();
	void Sleep();
	void changeState(State s);
	const char* stateToString(State s);
	void consumeInfo();
	uint8_t* formatPhoneNumber(const uint8_t* input);
	char message[256];
	size_t transmission;
	int repeaterBattery;

public:
    LoraManager();
    ~LoraManager();
    void init(WIFI* w);
    void setBluetoothManager(BluetoothManager* b);
    void exec();
    void setInitialTime(struct timeval time);
    void sendPackage(uint8_t* pck, int size, int destiny, int txt = 1, bool ka = false, int temp = 0, int ah = 0,int sh = 0,int ws = 0,int rn = 0, int b1 = 0, int b2 = 0);
    void receivePackage();
    bool getTransmitting();
    void setTransmitting(bool transm);
    void setKeepAlive(bool ka);
    bool getMessageBluetoothReady();
    uint8_t* getMessageBluetooth();
    int getMessageBluetoothSize();
    int getBluetoothTower();
    void setHigherOrder(bool higher);
    void setRepeaterBattery(int batt);
    void interrupt();
    
//    bool teste;
};

#endif 
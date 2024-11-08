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

class LoraManager {
private:
	enum State {
	    Send1_,
	    Send2_,
	    Receive1_,
	    Receive2_,
	    Receive3_,
	    Receive4_
	};

	State state;
	uint8_t buf[256];
	struct timeval initial_time;
	struct timeval transmission_time;
	int tower_number;
	bool sender;
	struct timeval now;
	
	LoraPackage actualPackage;
	LoraPackage receivedPackage;
	LoraPackage newPackage;
	
	void Send1();
	void Send2();
	void Receive1();
	void Receive2();
	void Receive3();
	void Receive4();
	void Sleep();

public:
    LoraManager();
    ~LoraManager();
    void init();
    void exec();
    void setInitialTime(struct timeval time);
    void sendPackage(uint8_t* pck, int size, int destiny);
    void receivePackage();
    
};

#endif 
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
	uint8_t* payload;
	int payload_size;
	uint8_t* package;
	int package_size;
	int tower_number;
	int destiny_number;
	bool sender;
	
	void Send1();
	void Send2();
	void Receive1();
	void Receive2();
	void Receive3();
	void Receive4();

public:
    LoraManager();
    ~LoraManager();
    void init();
    void exec();
    void setInitialTime(struct timeval time);
    void sendPackage(uint8_t* pck, int size, int destiny);
    void receivePackage();
    void encapsulate();
	void getPayload();
};

#endif 
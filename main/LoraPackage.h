#ifndef LORA_PACKAGE_H
#define LORA_PACKAGE_H

#include <stdio.h>
#include <cstring>
#include "parameters.h"
#include <memory>
#include "esp_heap_caps.h"


class LoraPackage {
private:
	uint8_t payload[256];
	int payload_size;
	uint8_t package[256];
	int package_size;
	int destiny_number;
	int sender_number;
	int original_number;
	int sum;
	int temperature;
	int air_humidity;
	int soil_humidity;
	int wind_speed;
	int rain;
	uint8_t isText;
	uint8_t isInfo;
	uint8_t iskeepAlive;
	
	void encapsulate();
	void decapsulate();

public:
    LoraPackage();
    ~LoraPackage();
	void setPackage(uint8_t* pck, int size);
	void setPayload(uint8_t* pay, int size, int destiny, int sender, bool ka = false, bool info = false, bool txt = true, int temp = 0, int ah = 0,int sh = 0,int ws = 0,int rn = 0);
	uint8_t* getPackage();
	int getPackageSize();
	uint8_t* getPayload();
	int getPayloadSize();
	int getDestinyNumber();
	void setSenderNumber(int sender);
	int getSenderNumber();
	int getOriginalNumber();
	void calculateSum();
	int getSum();
	int getTemperature();
	int getAirHumidity();
	int getSoilHumidity();
	int getWindSpeed();
	int getRain();
	int getIsInfo();
	int getIsText();
	int getKeepAlive();
};

#endif 
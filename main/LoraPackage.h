#ifndef LORA_PACKAGE_H
#define LORA_PACKAGE_H

#include <stdio.h>
#include <cstring>
#include "parameters.h"
#include <memory>


class LoraPackage {
private:
	uint8_t* payload;
	int payload_size;
	uint8_t* package;
	int package_size;
	int destiny_number;
	bool creation;
	
	void encapsulate();
	void decapsulate();

public:
    LoraPackage();
    ~LoraPackage();
	void setPackage(uint8_t* pck, int size);
	void setPayload(uint8_t* pay, int size, int destiny);
	uint8_t* getPackage();
	int getPackageSize();
	uint8_t* getPayload();
	int getPayloadSize();
	int getDestinyNumber();
};

#endif 
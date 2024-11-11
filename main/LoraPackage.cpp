#include "LoraPackage.h"

LoraPackage::LoraPackage()
{
	creation = true;
}

LoraPackage::~LoraPackage()
{
	
}

void LoraPackage::encapsulate()
{
    int header_size = 1;
    int new_len = payload_size + header_size;

	if(!creation)
		delete[] package;
	creation = false;

    package = new uint8_t[new_len];
    package_size = new_len;

    package[0] = destiny_number;
    memcpy(package + header_size, payload, payload_size);
}

void LoraPackage::decapsulate()
{
    int header_size = 1;
    int new_len = package_size - header_size;

	if(!creation)
		delete[] payload;
	creation = false;	

    payload = new uint8_t[new_len];
    payload_size = new_len;

    memcpy(payload, package + header_size, new_len);
    destiny_number = package[0];
}

void LoraPackage::setPackage(uint8_t* pck, int size)
{
	package = pck;
	package_size = size;
	decapsulate();
}

void LoraPackage::setPayload(uint8_t* pay, int size, int destiny)
{
	payload = pay;
	payload_size = size;
	destiny_number = destiny;
	encapsulate();
}

uint8_t* LoraPackage::getPackage()
{
	return package;
}

int LoraPackage::getPackageSize()
{
	return package_size;
}

uint8_t* LoraPackage::getPayload()
{
	return payload;
}

int LoraPackage::getPayloadSize()
{
	return payload_size;
}

int LoraPackage::getDestinyNumber()
{
	return destiny_number;
}


#include "LoraPackage.h"
#include <stdint.h>

LoraPackage::LoraPackage() 
{
    isText = false;
    iskeepAlive = false;
}

LoraPackage::~LoraPackage()
{
	
}

void LoraPackage::encapsulate()
{
    int header_size = 6;
    int sensors_size = isText?0:14;
    int sum_size = 2;
    int new_len = payload_size + header_size + sensors_size + sum_size;

    package_size = new_len;

    package[0] = destiny_number;
    package[1] = sender_number;
    package[2] = isText;
    package[3] = isInfo;
    package[4] = iskeepAlive;
    package[5] = original_number;
    
    if(!isText)
    {
		uint8_t* tempPointer = (uint8_t*)&temperature;
	    memcpy(package + header_size, tempPointer, 2);
	    
	    uint8_t* ahPointer = (uint8_t*)&air_humidity;
	    memcpy(package + header_size + 2, ahPointer, 2);
	    
	    uint8_t* shPointer = (uint8_t*)&soil_humidity;
	    memcpy(package + header_size + 4, shPointer, 2);
	    
	    uint8_t* wsPointer = (uint8_t*)&wind_speed;
	    memcpy(package + header_size + 6, wsPointer, 2);
	    
	    uint8_t* rnPointer = (uint8_t*)&rain;
	    memcpy(package + header_size + 8, rnPointer, 2);	
	    
	    uint8_t* b1Pointer = (uint8_t*)&battery1;
	    memcpy(package + header_size + 10, b1Pointer, 2);
	    
	    uint8_t* b2Pointer = (uint8_t*)&battery2;
	    memcpy(package + header_size + 12, b2Pointer, 2);
	}
    
    memcpy(package + header_size + sensors_size, payload, payload_size);
    
    calculateSum();
    
    uint8_t* sumPointer = (uint8_t*)&sum;
    memcpy(package + new_len - sum_size, sumPointer, sum_size);
}

void LoraPackage::decapsulate()
{
    int header_size = 6;
    int sum_size = 2;
    
    destiny_number = package[0];
    sender_number = package[1];
    isText = package[2];
    isInfo = package[3];
    iskeepAlive = package[4];
    original_number = package[5];
    
    int sensors_size = isText?0:14;
    
    int new_len = package_size - header_size - sensors_size - sum_size;

    payload_size = new_len;
    
    if(!isText)
    {
		uint8_t received_temp[2];
	    memcpy(received_temp, package + header_size, 2);
	    temperature = (int)((received_temp[1]<<8) | (received_temp[0]));	
	    
	    uint8_t received_ah[2];
	    memcpy(received_ah, package + header_size + 2, 2);
	    air_humidity = (int)((received_ah[1]<<8) | (received_ah[0]));	
	    
	    uint8_t received_sh[2];
	    memcpy(received_sh, package + header_size + 4, 2);
	    soil_humidity = (int)((received_sh[1]<<8) | (received_sh[0]));	
	    
	    uint8_t received_ws[2];
	    memcpy(received_ws, package + header_size + 6, 2);
	    wind_speed = (int)((received_ws[1]<<8) | (received_ws[0]));	
	    
	    uint8_t received_rn[2];
	    memcpy(received_rn, package + header_size + 8, 2);
	    rain = (int)((received_rn[1]<<8) | (received_rn[0]));	
	    
	    uint8_t received_b1[2];
	    memcpy(received_b1, package + header_size + 10, 2);
	    battery1 = (int)((received_b1[1]<<8) | (received_b1[0]));
	    
	    uint8_t received_b2[2];
	    memcpy(received_b2, package + header_size + 12, 2);
	    battery2 = (int)((received_b2[1]<<8) | (received_b2[0]));
	}   

    memcpy(payload, package + header_size + sensors_size, new_len);
    printf("\nRECEBIDO PAYLOAD: [%.*s]", new_len,payload);
    
    uint8_t received_sum[2];
    memcpy(received_sum, package + header_size + sensors_size + new_len, 2);
    sum = (int)((received_sum[1]<<8) | (received_sum[0]));
}

void LoraPackage::setPackage(uint8_t* pck, int size)
{
	printf("Tentando desempacotar!\n");
	if(size < 1000 && size > 0)
	{
		for(int i = 0; i < size; i++)
			package[i] = pck[i];	
		package_size = size;
		decapsulate();
	}
	
}

void LoraPackage::setPayload(uint8_t* pay, int size, int destiny, int sender, bool ka, bool info, int txt, int temp, int ah,int sh,int ws,int rn,int b1, int b2)
{
	printf("ENVIANDO PAYLOAD: [%.*s]", size,pay);
	if(size < 1000 && size > 0)
	{
		for(int i = 0; i < size; i++)
			payload[i] = pay[i];	
		payload_size = size;
		destiny_number = destiny;
		sender_number = sender;
		original_number = sender;
		isText = txt;
		isInfo = info;
		iskeepAlive = ka;
		temperature = temp;
		air_humidity = ah;
		soil_humidity = sh;
		wind_speed = ws;
		rain = rn;
		battery1 = b1;
		battery2 = b2;
		encapsulate();
	}
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

void LoraPackage::setSenderNumber(int sender)
{
	sender_number = sender;
	encapsulate();
}

int LoraPackage::getSenderNumber()
{
	return sender_number;
}

int LoraPackage::getOriginalNumber()
{
	return original_number;
}

void LoraPackage::calculateSum()
{
	int partial = 0;
	for(int i = 0; i < package_size - 2; i++)
	{
		partial += package[i];
	}
	sum = partial % 65536;
}

int LoraPackage::getSum()
{
	return sum;
}

int LoraPackage::getTemperature()
{
	return temperature;
}

int LoraPackage::getAirHumidity()
{
	return air_humidity;
}

int LoraPackage::getSoilHumidity()
{
	return soil_humidity;
}

int LoraPackage::getWindSpeed()
{
	return wind_speed;
}

int LoraPackage::getRain()
{
	return rain;
}

int LoraPackage::getIsInfo()
{
	return isInfo;
}

int LoraPackage::getIsText()
{
	return isText;
}

void LoraPackage::setIsText(int text)
{
	isText = text;
	encapsulate();
}

int LoraPackage::getKeepAlive()
{
	return iskeepAlive;
}

int LoraPackage::getBatt1()
{
	return battery1;
}

int LoraPackage::getBatt2()
{
	return battery2;
}

void LoraPackage::setBattery2(int batt)
{
	battery2 = batt;
	encapsulate();
}


#ifndef BATTERY_H
#define BATTERY_H  

class Battery {
private:
	int adc_raw[1][10];
public:
	Battery();
	~Battery();
	int measure();
};

#endif

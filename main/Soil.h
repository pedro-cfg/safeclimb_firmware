
#ifndef SOIL_H_
#define SOIL_H_

class Soil {
private:
	int adc_raw[1][10];
public:
	Soil();
	~Soil();
	int measure();
};

#endif

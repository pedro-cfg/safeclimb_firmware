
#ifndef WIND_H_
#define WIND_H_

class Wind {
private:
	int adc_raw[1][10];
public:
	Wind();
	~Wind();
	int measure();
};

#endif



#ifndef RAINDROP_H 
#define RAINDROP_H  

class RainDrop {
private:
	int adc_raw[1][10];
public:
	RainDrop();
	~RainDrop();
	int measure();
};

#endif

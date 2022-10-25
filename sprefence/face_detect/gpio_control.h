#ifndef GPIO_CONTROL
#define GPIO_CONTROL

const int FENCE_ON_DURATION = 60;

int GPIOInit(void);

void FenceOn(void);

void FenceOff(void);

#endif
#ifndef __HWH_GPIO_H__
#define __HWH_GPIO_H__


#define CLR_LED(PORT, IOPIN)			GPIO_SET_OUT_DATA(PORT,GPIO_GET_OUT_DATA(PORT)&(~IOPIN))
#define SET_LED(PORT, IOPIN)			GPIO_SET_OUT_DATA(PORT,GPIO_GET_OUT_DATA(PORT)|IOPIN)

extern uint8_t audio_switch;

void PA15_INT_Init(void);


#endif


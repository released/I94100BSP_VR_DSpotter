
#ifndef __HWH_PWM_H__
#define __HWH_PWM_H__

#include  "stdint.h"


typedef struct		//
{
	uint8_t Duty;		//Õ¼¿Õ±È
	uint32_t rate;	  //ÆµÂÊ
}pwm_type;

extern pwm_type pwm0_0;
extern pwm_type pwm0_1;
extern pwm_type pwm0_2;


void pwm_init(void);
uint8_t PWM0_DUTY(uint8_t ChannelNum,uint8_t DutyCycle);
void LED_control(void);






#endif
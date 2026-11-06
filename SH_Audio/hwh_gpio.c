
#include <stdio.h>
#include <string.h>
#include "I94100.h"

#include "hwh_gpio.h"



void PA15_INT_Init(void)
{
    /* Configure PA.15 as Quasi-bidirection mode and enable interrupt by falling edge trigger */
	GPIO_SetMode(PA, BIT15, GPIO_MODE_QUASI);
	PA15 = 1;
	GPIO_EnableInt(PA, 15, GPIO_INT_FALLING);
	NVIC_EnableIRQ(GPA_IRQn);
    
    /* Enable interrupt de-bounce function and select de-bounce sampling cycle time is 1024 clocks of LIRC clock */
	GPIO_SET_DEBOUNCE_TIME(GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_512);
	GPIO_ENABLE_DEBOUNCE(PA, BIT15);
}


uint8_t audio_switch=0;
void GPA_IRQHandler(void)
{
	if(GPIO_GET_INT_FLAG(PA,(1<<15)))
	{
		audio_switch++;
		if(audio_switch>1)	audio_switch=0;
		
		if(audio_switch)
		{
			GPIO_SetMode(PB,BIT0,GPIO_MODE_OUTPUT);
			//PB0=1;
			SET_LED(PB,BIT0);
		}
		else 
		{
			GPIO_SetMode(PB,BIT0,GPIO_MODE_QUASI);
			CLR_LED(PB,BIT0);//µ„¡¡
		}
		GPIO_CLR_INT_FLAG(PA,(1<<15));
	}
}















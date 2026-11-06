

#ifndef __HWH_LEDSHOW_H__
#define __HWH_LEDSHOW_H__


#define LED_1_CH_ON (PC4=1)
#define LED_2_CH_ON (PD7=1)
#define LED_1_CH_OFF (PC4=0)
#define LED_2_CH_OFF (PD7=0)
#define G_LED_ON  (PC9=0) 




extern uint8_t LED_ON_OFF;

extern void LED_display_num(uint8_t loc, uint8_t num , uint8_t point);
extern void GPIO_init_for_LED(void);


void LEDSHOW_send(	uint8_t nRet);
#endif


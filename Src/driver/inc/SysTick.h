#ifndef __SYSTICK_H__
#define __SYSTICK_H__

#include "I94100.h"

#define SYSTICK_1MS	 (SystemCoreClock/1000)

void Systick_Init(void);
void delay_100us(int timer);
void delay_1ms(int timer);
extern volatile uint32_t g_msTicks;
extern volatile uint32_t g_100msTicks;
#endif

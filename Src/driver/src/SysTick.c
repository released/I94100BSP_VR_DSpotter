/********************************************************************************
*     SysTick.c
*********************************************************************************/
#include "SysTick.h"

volatile uint32_t g_msTicks = 0;
volatile uint32_t g_100msTicks = 0;
volatile uint32_t msTicks = 0;
void SysTick_Handler(void)
{
    g_msTicks++; // used for AWE server to count processing MIPS
    g_100msTicks++;
    msTicks++;
}

void Systick_Init(void)
{
    SystemCoreClockUpdate();
    NVIC_SetPriorityGrouping(3U);
    SysTick_Config(SYSTICK_1MS);
    NVIC_SetPriority(SysTick_IRQn, 3);
    //NVIC_EnableIRQ(SysTick_IRQn);
}

void delay_100us(int timer)
{
    int j;
    int t = timer ;
    while(t--)
    {
        for(j=0; j<4500; j++)
        {

        }
    }
}

void delay_1ms(int timer)
{
    msTicks = 0;
    while(msTicks < timer);
}

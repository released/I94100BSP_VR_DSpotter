#include "c_stdfx.h"
#include <stdio.h>
#include <string.h>
#include "I94100.h"
#include "hwh_gpio.h"

#define CLR_LED(PORT, IOPIN)			GPIO_SET_OUT_DATA(PORT,GPIO_GET_OUT_DATA(PORT)&(~IOPIN))
#define SET_LED(PORT, IOPIN)			GPIO_SET_OUT_DATA(PORT,GPIO_GET_OUT_DATA(PORT)|IOPIN)
#define LED_1_CH_ON (PC4=1)
#define LED_2_CH_ON (PD7=1)
#define LED_1_CH_OFF (PC4=0)
#define LED_2_CH_OFF (PD7=0)
#define G_LED_ON  (PC9=0)


void GPA_Init(void)
{
	//IO
    /* Configure PA.15 as Quasi-bidirection mode and enable interrupt by falling edge trigger */
	GPIO_SetMode(PA, BIT15, GPIO_MODE_QUASI);
	PA15 = 1;
	GPIO_EnableInt(PA, 15, GPIO_INT_FALLING);
	NVIC_EnableIRQ(GPA_IRQn);
    /* Enable interrupt de-bounce function and select de-bounce sampling cycle time is 1024 clocks of LIRC clock */
	GPIO_SET_DEBOUNCE_TIME(GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_512);
	GPIO_ENABLE_DEBOUNCE(PA, BIT15);
	//LED
	GPIO_SetMode(PC,BIT2|BIT3|BIT4|BIT5|BIT6|BIT8|BIT9|BIT13|BIT14,GPIO_MODE_OUTPUT);    // 0X637C
	GPIO_SetMode(PD,BIT7,GPIO_MODE_OUTPUT);
	PD7=0;
	PC4=0;
	LED_1_CH_OFF;
	LED_2_CH_OFF;
	G_LED_ON;
	GPIO_SetMode(PB,BIT0|BIT1|BIT2,GPIO_MODE_OUTPUT);
//	SET_LED(PB,BIT0);
}

uint16_t audio_switch=2;
void GPA_IRQHandler(void)
{
	if(GPIO_GET_INT_FLAG(PA,(1<<15)))
	{
		audio_switch++;
		if(audio_switch>=4)			audio_switch=0;
		if(audio_switch == 1)
		{
//			GPIO_SetMode(PB,BIT0,GPIO_MODE_OUTPUT);
//			SET_LED(PB,BIT0);
		}
		else  if(audio_switch == 0)
		{
//			GPIO_SetMode(PB,BIT0,GPIO_MODE_QUASI);
//			CLR_LED(PB,BIT0);
		}
		else  if(audio_switch == 2)
		{
//			GPIO_SetMode(PB,BIT0,GPIO_MODE_OUTPUT);
//			CLR_LED(PB,BIT0);
		}
		else  if(audio_switch == 3)
		{
//			GPIO_SetMode(PB,BIT0,GPIO_MODE_OUTPUT);
//			CLR_LED(PB,BIT0);
		}
		GPIO_CLR_INT_FLAG(PA,(1<<15));
	}
}


//void GPA_SetLED(void)
//{
//	CLR_LED(PB,BIT0);
//}

//void GPA_ClrLED(void)
//{
//	SET_LED(PB,BIT0);
//}

void GPA_TOGGLELED(void)
{
	GPIO_TOGGLE(PB0);
}













//void time1_init(void)	//1ms֨ʱǷ
//{	uint8_t u8Lock;
//	//ʹŜtime0΢ʨʱד
//	CLK_EnableModuleClock(TMR1_MODULE);
//		// Enable HXT clock for TIMER0 and TIMER3.
//	u8Lock = SYS_Unlock();
//	CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
//	CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
//	SYS_Lock(u8Lock);
//	//ʨ׃ʱדԴۍؖƵ
//	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HXT, 0);
//	TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);//
//	TIMER_EnableInt(TIMER1);
//	NVIC_EnableIRQ(TMR1_IRQn);
//	TIMER_Start(TIMER1);
//}

//extern void time_cyek(void);
//extern void LED_display_time(void);
//void TMR1_IRQHandler(void)
//{
//   if(TIMER_GetIntFlag(TIMER1) == 1)
//   {
//      /* Clear Timer0 time-out interrupt flag */
//      TIMER_ClearIntFlag(TIMER1);
//			uart_time_task();	//UART݆ʱ
//			LED_display_time();
//		 time_cyek();
//		 
//   }
//}



uint16_t wakeup_delay=0;
#define delay_t		20000
UINT16 display_num[16]={0X6168,0X0028,0X6260,0X4268,0X0328,0X4348,0X6348,0X0068,0X6368,0X4368};
uint8_t LED_DATA[2];
uint8_t LED_ON_OFF;



void time_cyek(void)
{
	if(wakeup_delay)
	{
		wakeup_delay--;
//		if(wakeup_delay < (delay_t -200))	PB0=1; 
		if(wakeup_delay==0)PB0=1;
		LED_ON_OFF=1;
	}
	else LED_ON_OFF=0;
}



void GPIO_init_for_LED(void){
	GPIO_SetMode(PC,BIT2|BIT3|BIT4|BIT5|BIT6|BIT8|BIT9|BIT13|BIT14,GPIO_MODE_OUTPUT);    // 0X637C
	GPIO_SetMode(PD,BIT7,GPIO_MODE_OUTPUT);
	PD7=0;
	PC4=0;
	
	LED_1_CH_OFF;
	LED_2_CH_OFF;
	G_LED_ON;
	
	GPIO_SetMode(PB,BIT0,GPIO_MODE_OUTPUT);
}


void LED_display_num(UINT8 loc, UINT8 num )
{
  if(loc == 1)
  {
		LED_DATA[0] =num;
  }
  else if(loc == 2)
  {
		LED_DATA[1] =num;
  }
}


void LED_display_time(void)
{
	static uint8_t aa=0;
	if(LED_ON_OFF==0)
	{
		PC9=0;
		PC4=1;
		PD7=1;
		PC2=1; PC3=1; PC5=1; PC6=1; PC8=1; PC13=1; PC14=1; 
		return;
	}
	if(aa)
	{
		aa=0;
		LED_2_CH_OFF;
    GPIO_SET_OUT_DATA(PC,(GPIO_GET_OUT_DATA(PC)|0X636C)&(~display_num[LED_DATA[0]]));   // TURN OFF 0X636C;
    LED_1_CH_ON;
	}
	else
	{
		aa=1;
		LED_1_CH_OFF;
    GPIO_SET_OUT_DATA(PC,(GPIO_GET_OUT_DATA(PC)|0X636C)&(~display_num[LED_DATA[1]]));
    LED_2_CH_ON;
	}
}

void LED_display_turn_on(void)
{
  GPIO_SET_OUT_DATA(PC,(GPIO_GET_OUT_DATA(PC)|0X636C));
  LED_1_CH_ON;
  LED_2_CH_ON;
  G_LED_ON;
}


void VR_sand(	uint8_t nRet)
{
	wakeup_delay = delay_t;	//טтғʱ
	
	PB0=0;
	LED_display_num(2,(nRet-1)%10);
	LED_display_num(1,(nRet-1)/10);
	
//	UART_int_send(&nRet, 0x01);
}


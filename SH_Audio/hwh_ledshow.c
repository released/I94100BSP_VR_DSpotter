
#include <stdio.h>
#include "I94100.h"
#include "hwh_ledshow.h"

#include "hwh_gpio.h"



uint16_t display_num[]={0X6168,0X0028,0X6260,0X4268,0X0328,0X4348,0X6348,0X0068,0X6368,0X4368};
#define point_PC2  (1<<2)

uint8_t LED_DATA[2];
uint8_t LED_ON_OFF;
uint8_t LED_POINT[2]={0};
extern uint16_t wakeup_delay;
                     //数码管通道  显示数字     是否有小数点
void LED_display_num(uint8_t loc, uint8_t num, uint8_t point)
{
  if(loc == 1)
  {
		LED_DATA[0] =(num);
		if(point)	LED_POINT[0] = point_PC2;
		else LED_POINT[0] = 0;
  }
  else if(loc == 2)
  {
		LED_DATA[1] =(num);
		if(point)	LED_POINT[1] = point_PC2;
		else LED_POINT[1] = 0;
  }
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
	SET_LED(PB,BIT0);  //拉高
}

#define delay_t		11000
void time_cyek(void)
{
	if(wakeup_delay)
	{
		wakeup_delay--;
		if(wakeup_delay < (delay_t -200))	SET_LED(PB,BIT0);
		LED_ON_OFF=1;
	}
	else LED_ON_OFF=0;
}


void LED_display_time(void)  //放入1ms中断
{
	static uint8_t aa=0;
	//
	
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
    GPIO_SET_OUT_DATA(PC,(GPIO_GET_OUT_DATA(PC)|0X636C)&(~(display_num[LED_DATA[0]]|LED_POINT[0])));   // TURN OFF 0X636C;
    LED_1_CH_ON;
	}
	else
	{
		aa=1;
		LED_1_CH_OFF;
    GPIO_SET_OUT_DATA(PC,(GPIO_GET_OUT_DATA(PC)|0X636C)&(~(display_num[LED_DATA[1]]|LED_POINT[1])));
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






void LEDSHOW_send(	uint8_t nRet)  //显示调用接口
{
	if((nRet==0)||(wakeup_delay)) //如果是唤醒词或者，唤醒时间内
	{
		wakeup_delay = delay_t;	//重新延时

		CLR_LED(PB,BIT0);
		LED_display_num(2,(nRet)%10,0);
		LED_display_num(1,(nRet)/10,0);
	}
	else return;
}






#include <stdio.h>
#include "I94100.h"
#include "hwh_uart.h"

#include "string.h"

uart_type ap_uart0;

S_APP 	g_sApp;
DSCT_T     sPDMA_SPK[2];

void uart_time_task(void)
{
	if(ap_uart0.uart_rx_time)	ap_uart0.uart_rx_time--;
	
}


void UART0_IRQHandler(void)   ////modify 20170804
{
	uint32_t  u32IntStatus;
	
	u32IntStatus= UART0->INTSTS;
	if(u32IntStatus & (1<<0))	//接收中断
	{
		ap_uart0.uart_rx_data[ap_uart0.uart_rx_len] = UART0->DAT;
		if((ap_uart0.uart_rx_len+1) <UART_DA)	ap_uart0.uart_rx_len ++;	//预防越界
		ap_uart0.uart_rx_time = UART_TT;
		
	}
	else if(u32IntStatus & (1<<1))	//发送缓冲空中断
	{
		UART0->INTEN &= ~(1<<1);     /////THREIEN
		if(ap_uart0.uart_tx_len)
		{
			UART0->DAT= ap_uart0.uart_tx_data[ap_uart0.uart_tx_per]  ;
			ap_uart0.uart_tx_len--;
			ap_uart0.uart_tx_per ++;
			
			UART0->INTEN |=(1<<1);
		}
		else
		{
			ap_uart0.uart_tx_per =0;
			UART0->INTEN &= ~(1<<1);		//发送完毕，关闭发送中断
			//UART0->INTEN |=(1<<1);
		}
	}
}


void UART0_Init(void)
{
	/* Enable UART module clock */
	CLK_EnableModuleClock(UART0_MODULE);

	/* Select UART module clock source as HXT and UART module clock divider as 1 */
	CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_PLL, CLK_CLKDIV0_UART0(1));
	/* Reset UART module */
	SYS_ResetModule(UART0_RST);
	UART_Open(UART0, 1200000);

	/* Enable PDMA clock source */
	CLK_EnableModuleClock(PDMA_MODULE);

	/* Set PB multi-function pins for UART0 RXD(PB.9) and TXD(PB.8) */
	SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB8MFP_Msk | SYS_GPB_MFPH_PB9MFP_Msk);
	SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB8MFP_UART0_TXD | SYS_GPB_MFPH_PB9MFP_UART0_RXD);
	
//	UART0->FUNCSEL = UART_FUNCSEL_LINE;//使能LIN
//	UART0->ALTCTL |= UART_ALTCTL_LINRXEN_Msk;	//打开接收
	
	UART_ClearIntFlag(UART0,UART_INTSTS_BUFERRINT_Msk);
	UART_ClearIntFlag(UART0,UART_INTSTS_RLSINT_Msk);
	UART_ClearIntFlag(UART0,UART_INTSTS_MODEMINT_Msk);
	UART_ClearIntFlag(UART0,UART_INTSTS_RXTOINT_Msk);
	UART0->INTEN |=(1<<0); //使能接收中断
	UART0->TOUT |= (0x02<<0);  //接收超时时间
  NVIC_EnableIRQ(UART0_IRQn);
	
}


void UART_int_send(uint8_t *dara, uint16_t len)
{
	uint16_t ii;
	UART0->INTEN |=(1<<1);
	
	for(ii=0; ii<len; ii++)
	{
		ap_uart0.uart_tx_data[ii] = *dara++;
	}
	ap_uart0.uart_tx_len =len;
	if(ap_uart0.uart_tx_len==0)	return;
	
	while((UART0->FIFOSTS &(1<<22)) == 0)	;
	UART0->DAT= ap_uart0.uart_tx_data[0];//发送
	
	ap_uart0.uart_tx_per++;
	ap_uart0.uart_tx_len --;
	UART0->INTEN |=(1<<1);	//打开发送中断
}

extern uint32_t VarApp;
void uart0_rev_task(void)
{
//	uint8_t *str_add;
	if((ap_uart0.uart_rx_time==0)&&(ap_uart0.uart_rx_len))//有接收到数据
	{
		ap_uart0.uart_rx_len =0;

		//UART_int_send(ap_uart0.uart_rx_data	, ap_uart0.uart_rx_len);	//发送接收到的数据
		if((ap_uart0.record_ser==0)&&(ap_uart0.uart_rx_data[0]=='C')&&
			 (ap_uart0.uart_rx_data[1]=='Y')&&(ap_uart0.uart_rx_data[2]=='B')&&
		   (ap_uart0.uart_rx_data[3]=='1'))
		{
			ap_uart0.record_ser=1;//开始录音
		}
		else if((ap_uart0.record_ser==1)&&(ap_uart0.uart_rx_data[0]=='C')&&
			 (ap_uart0.uart_rx_data[1]=='Y')&&(ap_uart0.uart_rx_data[2]=='B')&&
		   (ap_uart0.uart_rx_data[3]=='2'))
		{
			ap_uart0.record_ser=0;//停止录音
		}

		
		 memset(ap_uart0.uart_rx_data, 0, sizeof(ap_uart0.uart_rx_data));
	}
}



void time1_init(void)	//1ms定时器
{	uint8_t u8Lock;
	//使能time0外设时钟
	CLK_EnableModuleClock(TMR1_MODULE);
		// Enable HXT clock for TIMER0 and TIMER3.
	u8Lock = SYS_Unlock();
	CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
	CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
	SYS_Lock(u8Lock);
	//设置时钟源和分频
	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HXT, 0);
	TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);//
	TIMER_EnableInt(TIMER1);
	NVIC_EnableIRQ(TMR1_IRQn);
	TIMER_Start(TIMER1);
}

extern void time_cyek(void);
extern void LED_display_time(void);
extern void LED_2S_task(void);
void TMR1_IRQHandler(void)
{
   if(TIMER_GetIntFlag(TIMER1) == 1)
   {
      /* Clear Timer0 time-out interrupt flag */
      TIMER_ClearIntFlag(TIMER1);
			uart_time_task();	//UART计时
			LED_display_time();
			time_cyek();
//		 LED_2S_task();
   }
}




uint16_t wakeup_delay=0;








#ifndef __HWH_UART_H__
#define __HWH_UART_H__

#include "stdint.h"

#define UART_TT		6	//uart数据每帧之间有时间间隔，         /////modify 20170804
#define UART_DA		20                                       /////modify 20170804


typedef struct		//uart中断收发数据结构                     /////modify 20170804
{
	uint16_t uart_tx_len;
	uint8_t uart_tx_data[32];
	uint16_t uart_rx_len;
	uint8_t uart_rx_data[20];
	uint16_t uart_tx_per;
	uint16_t uart_rx_time;
	uint8_t record_ser;
	uint8_t voice_ch;
}uart_type;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////2019年8月30日//////////////////////////////////

//#define AWE_FRAME_SIZE          40
//#define OUTPUT_CHANNEL_COUNT    2

typedef struct
{

	uint8_t VR_Result;
}S_Mic_APP;


typedef struct
{
	uint8_t  u32TotalAudioNum;
	uint8_t  u32PlayID;
	uint8_t  bPCMPlaying;
	uint8_t  bBufferEmpty;
	
	
}S_NuOne_APP;

typedef struct
{
	union
	{
		S_Mic_APP  sMicGetApp;
		S_NuOne_APP  sNuOneApp;
	}Mic_NuOne;
	
	
	
} S_APP;	

extern  S_APP 	g_sApp;




extern uint16_t wakeup_delay;


extern uart_type ap_uart0;


void uart_time_task(void);
void UART0_Init(void);
void UART_int_send(uint8_t *dara, uint16_t len);
void uart0_rev_task(void);
void time1_init(void);	//1ms定时器





#endif

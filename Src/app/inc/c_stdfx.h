/********************************************************************************
*     c_stdfx.c
*********************************************************************************/
#ifndef __stdfx_H__
#define __stdfx_H__
#include <stdio.h>
#include <string.h>
#include "I94100.h"
#include "NuOneEx.h"

#define UNUSED(x) 			(void)(x);
//#define FALSE 				(0)
//#define TRUE 				(1)
#define false 				(0)
#define true 				(1)
//#define BIT0 				0x01
//#define BIT1 				0x02
//#define BIT2 				0x04
//#define BIT3 				0x08
//#define BIT4 				0x10
//#define BIT5 				0x20
//#define BIT6 				0x40
//#define BIT7 				0x80

#define Setbit(x,y) x|= (1<<y) //将X的第Y位置1
#define Clrbit(x,y) x&=~(1<<y) //将X的第Y位清0
typedef unsigned char		u8;
typedef unsigned short int  u16;
typedef unsigned long int   u32;
typedef signed char 		int8;
typedef short int   		int16;
typedef long  int   		int32;
typedef BOOL  				bool;
#include "osDefine.h"
#include "appDefine.h"
//Get MAX or MIN value
#define GetMax( x1, x2 ) ( ( x1 ) > ( x2 ) ? ( x1 ) : ( x2 ) )
#define GetMin( x1, x2 ) ( ( x1 ) > ( x2 ) ? ( x2 ) : ( x1 ) )


#if (MIC_PDMA_16BIT==2)   //DMIC is 24bit
#define AMIC_DMIC_BUF_LEN   AMIC2PDMA_BUFF_LEN_2CH
#else
#define AMIC_DMIC_BUF_LEN   AMIC2PDMA_BUFF_LEN_2CH/2
#endif
//
extern u32 g_u32MICBuffer[2][AMIC_DMIC_BUF_LEN];
extern u32 g_u32SPKBuffer[2][NUONEEX_DECODE_SAMPLE_PER_FRAME];	
extern u32 g_u32TempBuffer[2][AMIC2PDMA_BUFF_LEN_2CH/2];
extern DSCT_T DMA_TXDESC[2],DMA_RXDESC[2];
extern PDMA_CONFIG_T PDMA_CONFIG;
extern u8 g_u8Current_Mic_ChannNum;
extern u8 g_usbd_UsbAudioState;
extern u8 g_u8amic_pdma_bufidx;
extern u8 g_u8I2S0TxBuffIndex;
extern u8 g_u8I2S0RxBuffIndex;
extern u8 g_u8I2S2TxBuffIndex;
extern u8 g_u8I2S2RxBuffIndex;

extern u16 g_u16UAC_Buff_ReadIndex;
extern u16 g_u16UAC_Buff_WriteIndex;
extern u32 g_au32UAC_RingBuff[AMIC_RING_BUFFER_LEN_2CH];
extern u32 g_usbd_PlaySampleRate; 
extern u8 g_usbd_RecMute;      
extern int16 g_usbd_RecVolumeL;
extern int16 g_usbd_RecVolumeR;
extern int16 g_usbd_RecMaxVolume;
extern int16 g_usbd_RecMinVolume;
extern int16 g_usbd_RecResVolume;
extern u8 g_usbd_PlayMute;             
extern int16 g_usbd_PlayVolumeL;         
extern int16 g_usbd_PlayVolumeR;
extern int16 g_usbd_PlayMaxVolume;
extern int16 g_usbd_PlayMinVolume;
extern int16 g_usbd_PlayResVolume;
//MICBUF
extern uint16_t audio_switch;
//extern uint16_t AEC_CHANN_MIC[];

extern uint16_t CHANN_TEMP[2][160*3];
//extern uint16_t AEC_CHANN_USB_Temp[];
//extern uint16_t AEC_CHANN_USB[5][320];


extern BOOL Sem_BufFill;
// HID control
extern u8 g_u8HIDSetFlag;
extern u8 g_u8HIDData[8];
// EP
extern u8 g_u8EP2_Enable;
//PlayBack
extern volatile u16 g_u16PlayBackReadPtr;
extern volatile u16 g_u16PlayBackWritePtr;


extern volatile u32 g_au32USBPlaybackRingBuff[MAX_I2STX_BUFFER_LEN];


#endif

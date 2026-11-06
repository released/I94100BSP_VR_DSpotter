/********************************************************************************
*     c_stdfx.c
*********************************************************************************/
#include "c_stdfx.h"
#include "NuOneEx.h"

u32 g_u32MICBuffer[2][AMIC_DMIC_BUF_LEN];

u32 g_u32SPKBuffer[2][NUONEEX_DECODE_SAMPLE_PER_FRAME]={0};
u32 g_u32TempBuffer[2][AMIC2PDMA_BUFF_LEN_2CH/2]={0};
DSCT_T DMA_TXDESC[2] __attribute__((section(".ARM.__at_0x20001e00")));
DSCT_T DMA_RXDESC[2] __attribute__((section(".ARM.__at_0x20001e20")));
DSCT_T DMA_DMICDESC[2] __attribute__((section(".ARM.__at_0x20001e40")));
DSCT_T DMA_DPWMDESC[2] __attribute__((section(".ARM.__at_0x20001e60")));

PDMA_CONFIG_T PDMA_CONFIG;
u8 g_u8Current_Mic_ChannNum = REC_CHANNELS;
u8 g_usbd_UsbAudioState = 0;
u8 g_u8amic_pdma_bufidx = 1;
u8 g_u8I2S0TxBuffIndex = 0;
u8 g_u8I2S0RxBuffIndex = 0;

u8 g_u8I2S2TxBuffIndex = 0;
u8 g_u8I2S2RxBuffIndex = 0;

uint16_t g_u16UAC_Buff_ReadIndex = 0;
uint16_t g_u16UAC_Buff_WriteIndex = 0;
//UAC
u32 g_au32UAC_RingBuff[AMIC_RING_BUFFER_LEN_2CH]={0};
u32 g_usbd_PlaySampleRate = PLAY_RATE_48K; 
u8 g_usbd_RecMute = 0x00;     
int16 g_usbd_RecVolumeL = 0x1000;
int16 g_usbd_RecVolumeR = 0x1000;
int16 g_usbd_RecMaxVolume = 0x0000;
int16 g_usbd_RecMinVolume = 0x9D00;
int16 g_usbd_RecResVolume = 0x400;
uint8_t g_usbd_PlayMute = 0x00;             
int16 g_usbd_PlayVolumeL = 0x1000;         
int16 g_usbd_PlayVolumeR = 0x1000;
int16 g_usbd_PlayMaxVolume = 0x0000;
int16 g_usbd_PlayMinVolume = 0x9D00;
int16 g_usbd_PlayResVolume = 0x400;
//MICBUF
//uint16_t audio_switch=2;

uint16_t CHANN_TEMP[2][160*3] = {0};



BOOL Sem_BufFill = false;
// HID control
u8 g_u8HIDSetFlag = 0;
u8 g_u8HIDData[8];
// EP
u8 g_u8EP2_Enable = 0;
//PlayBack
volatile u16 g_u16PlayBackReadPtr = 0;
volatile u16 g_u16PlayBackWritePtr = 0;

volatile u32 g_au32USBPlaybackRingBuff[MAX_I2STX_BUFFER_LEN];

//ZKSS

//void *ZKSS_handle = NULL;
//volatile  __align(4)  BYTE AEC_NS_MemPool[COLSONIC_BUF_LEN_MODE];//  __attribute__  ((at(0x20002000)));//强制指定位置





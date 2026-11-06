/*
 * appAudioLine.c
 *
 *  Created on: 2019年9月9日
 *      Author: miaodai
 */
#include "appAudioLine.h"
#include "vr.h"


u16 *LineInNear,*LineInFar,*LineInDecode;
u16 *LineOutMC1,*LineOutMC2;
u16 *LinePlayMC1,*LinePlayMC2;
USBDMA_Ctrl_t Flow_Ctrl;


void AudioCodec_Init(void)
{
   NVIC_DMA_Init();
	
	
//	MIC_Start();	

	g_usbd_UsbAudioState = UAC_START_AUDIO_RECORD;
	g_u16UAC_Buff_WriteIndex = 0;
	g_u16UAC_Buff_ReadIndex = 0;
	// PDMA_DISABLE_CHANNEL(PDMA_CH2_MASK);
	// PDMA_CONFIG.play_en = 0;
	PDMA_CONFIG.rec_en = 1;
	PDMA_CONFIG.play_en = 1;
	PDMA_CONFIG.rec_buflen = AMIC2PDMA_BUFF_LEN_2CH;    //320
	PDMA_CONFIG.play_buflen = AMIC2PDMA_BUFF_LEN_2CH;   //320
	PDMA_Init(&PDMA_CONFIG);
	
//	SPK_Start();
	I2S0_Init();
	MIC_Stop();
	
//////////////////////////////////////////////////MIC
#if  ((MIC_PDMA_16BIT==0)||(MIC_PDMA_16BIT==1))	//88C22
#ifdef NAU88C22	
	NAU88C22_Init();
#endif
#ifdef NAU88L21
	CODEC_I2C1_Init();
	NAU88L21_LineIn_Init();
#endif
//	I2S0_Init();
//	MIC_Stop();
#elif  (MIC_PDMA_16BIT==2) //DMIC
#ifdef NAU88L21
	CODEC_I2C1_Init();
	NAU88L21_LineIn_Init();
#endif
	DMIC_Init(REC_RATE, 2);
//	MIC_Stop();	
#elif (MIC_PDMA_16BIT ==3) //85L40
	
#endif

	
/////////////////////////////////////////////////SPK
//#if ((SPK_PDMA_16BIT==0)||(SPK_PDMA_16BIT==1))
////	NAU88C22_Init();
////	I2S0_Init();	
//#elif (SPK_PDMA_16BIT==2)
////	DPWM_init(REC_RATE);
//#endif

}


uint8_t USBDMA_TXHandle(void)
{
	int i = 0;
	u16 pdma_buffer_len;
	pdma_buffer_len = AMIC2PDMA_BUFF_LEN_2CH;
	//
	for(i=0; i<(pdma_buffer_len/2); i++)
	{
		//获取USB播放的音乐 , 注意 SPI_I2S使能后， 数据也丢入 USB buf,， 但USB播放枚举依然关闭
		if (g_u16PlayBackReadPtr == g_u16PlayBackWritePtr)
		{
			if(g_u16PlayBackReadPtr != 0)			g_u16PlayBackReadPtr--;
			else		g_u16PlayBackReadPtr = MAX_I2STX_BUFFER_LEN-1;
		}
		if(g_usbd_PlayMute)		g_u32SPKBuffer[g_u8I2S0TxBuffIndex][i] = 0;
		else		g_u32SPKBuffer[g_u8I2S0TxBuffIndex][i] = g_au32USBPlaybackRingBuff[g_u16PlayBackReadPtr];
		g_u16PlayBackReadPtr++;
		if (g_u16PlayBackReadPtr >= MAX_I2STX_BUFFER_LEN)			g_u16PlayBackReadPtr = 0;

	}
	Flow_Ctrl.u8TxDMACount ++;
	if(Flow_Ctrl.u8TxDMACount >= 10)
	{
		Flow_Ctrl.u8TxDMACount = 0;
		Flow_Ctrl.u8TxDMAFull = true;
	}
		g_u8I2S0TxBuffIndex ^= 0x1;
	
	return g_u8I2S0TxBuffIndex;
}

extern uint16_t triangle[];
extern uint16_t j;

void USBDMA_RXHandle(void)
{
	int i = 0;
	u16 pdma_buffer_len;
	static uint16_t apc_num=0;
	pdma_buffer_len = AMIC2PDMA_BUFF_LEN_2CH;
	
	//DMIC is 24bit ,需要转换为16bit 
	for ( i= 0 ; i < (pdma_buffer_len/2) ; i++)
	{	
#if  ((MIC_PDMA_16BIT ==0) ||(MIC_PDMA_16BIT ==1)) //88C22 / 88L21 	
		CHANN_TEMP[0][i] = (g_u32MICBuffer[g_u8I2S0RxBuffIndex][i]&0xffff);//MIC1 L 
		CHANN_TEMP[1][i] = (g_u32MICBuffer[g_u8I2S0RxBuffIndex][i]>>16);//MIC2 R 
		g_au32UAC_RingBuff[g_u16UAC_Buff_WriteIndex++] = ((uint32_t)CHANN_TEMP[0][i] <<16) | CHANN_TEMP[1][i] ;
//		g_au32UAC_RingBuff[g_u16UAC_Buff_WriteIndex++] = g_u32MICBuffer[g_u8I2S0RxBuffIndex][i];
//		g_au32UAC_RingBuff[g_u16UAC_Buff_WriteIndex++] = 0;
//		g_au32UAC_RingBuff[g_u16UAC_Buff_WriteIndex] = CHANN_TEMP[0][i];
//		g_au32UAC_RingBuff[g_u16UAC_Buff_WriteIndex] <<=16;
//		if(g_sApp.Mic_NuOne.sNuOneApp.bPCMPlaying) g_au32UAC_RingBuff[g_u16UAC_Buff_WriteIndex] += 
		
#elif (MIC_PDMA_16BIT==2)  //DMIC 24bit 
		CHANN_TEMP[0][i] = (uint16_t)(DMIC_volume*(g_u32MICBuffer[g_u8I2S0RxBuffIndex][2*i] >>8));//MIC1 L 
		CHANN_TEMP[1][i] = (uint16_t)(DMIC_volume*(g_u32MICBuffer[g_u8I2S0RxBuffIndex][2*i+1] >>8));//MIC2 R 
		g_au32UAC_RingBuff[g_u16UAC_Buff_WriteIndex++] = ((uint32_t)CHANN_TEMP[0][i] <<16) + CHANN_TEMP[1][i] ;
#endif
		/* Keep the published frame stable while both recognizers consume it. */
		if (Cyberon_VR.ai16cy_fifi_inder < Audio_Frame_Len * 3U)
			Cyberon_VR.ai16cy_capturebuf[Cyberon_VR.ai16cy_fifi_inder++] = CHANN_TEMP[0][i];
		
		if ( g_u16UAC_Buff_WriteIndex >= AMIC_RING_BUFFER_LEN_2CH )		  		g_u16UAC_Buff_WriteIndex = 0;
	}
	
	apc_num++;
	if(apc_num>=3) apc_num=0;
	if(apc_num==0)
	{
		if (g_sApp.Mic_NuOne.sNuOneApp.bPCMPlaying == 0) {
			if (Cyberon_VR.bCybBufReady == 0U
				&& Cyberon_VR.ai16cy_fifi_inder == Audio_Frame_Len * 3U) {
				for (i = 0; i < Audio_Frame_Len * 3; ++i)
					Cyberon_VR.ai16cy_fifibuf[i] = Cyberon_VR.ai16cy_capturebuf[i];
				Cyberon_VR.bCybBufReady = 1U;
			} else {
				++Cyberon_VR.droppedFrames;
			}
		}
		Cyberon_VR.ai16cy_fifi_inder = 0;
	}
	
	Flow_Ctrl.u8RxDMAFull = true;

	g_u8I2S0RxBuffIndex ^= 0x1;
}






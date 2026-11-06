/*
 * appAudioLine.h
 *
 *  Created on: 2019年9月9日
 *      Author: miaodai
 */
#ifndef APPAUDIOLINE_H_
#define APPAUDIOLINE_H_
#include "c_stdfx.h"
#include "appDefine.h"
typedef struct __attribute__((packed))
{
    u8 u8TxDMACount;
    bool u8TxDMAFull;
    u8 u8RxDMACount; 
	bool u8RxDMAFull;
} USBDMA_Ctrl_t;
extern USBDMA_Ctrl_t Flow_Ctrl;
extern void AudioLine_Init(void);
extern void AudioCodec_Init(void);


extern uint8_t USBDMA_TXHandle(void);
extern void USBDMA_RXHandle(void);

#endif /* APPAUDIOLINE_H_ */

#ifndef PLAY_NUONE_H
#define PLAY_NUONE_H

#include <stdio.h>
#include "I94100.h"
#include "AudioRom.h"
#include "NuOneEx.h"
#include "SPIFlash.h"
#include "AirconAudio.h"

#define AUDIOROM_STORAGE_START_ADDR \
    (AIRCON_AUDIO_ADDRESS + AIRCON_AUDIO_HEADER_BYTES)




extern BOOL	bPCMPlaying;
extern S_SPIFLASH_HANDLER g_sSPIFlash;

extern uint32_t 	PDMA1CallBackCount;

BOOL PlayNuOnebyIdx(UINT8 u8PlayIdx);
BOOL PlayNuOneSequence(const uint8_t *indexes, uint32_t count);
void PlayNuOneSetAudioSize(uint32_t bytes);
void PlayLoop(void);
//void PlayClose(void);
uint32_t ReadDataCallback(void *pu8Buf, uint32_t u32StartAddr, uint32_t u32Count);

#endif


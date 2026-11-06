#ifndef PLAYBACK_TEST_SDK_H
#define PLAYBACK_TEST_SDK_H

#include <stdint.h>
#include <stddef.h>
#include "AirconAudio.h"

#define PLAY_NUONE_H
#define _AUDIOCOMMON_H_
#define __NAU88C22_H__
#define __align(x) __declspec(align(x))
#define TRUE 1
#define FALSE 0
#define NUONEEX_DECODE_WORK_BUF_SIZE 652U
#define NUONEEX_DECODE_TEMP_BUF_SIZE 648U
#define NUONEEX_DECODE_SAMPLE_PER_FRAME 320U
#define AUDIOROM_STORAGE_START_ADDR (AIRCON_AUDIO_ADDRESS + AIRCON_AUDIO_HEADER_BYTES)

typedef int BOOL;
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef int16_t INT16;
typedef uint32_t UINT32;
typedef int32_t INT32;
typedef uint32_t u32;
typedef uint32_t (*TEST_READ)(void *, uint32_t, uint32_t);
typedef uint32_t (*TEST_EVENT)(uint16_t, uint16_t);
typedef struct {
    uint32_t u32AudioChunkAddr;
    uint32_t u32AudioChunkSize;
} S_ROM_AUDIO_CHUNK_INFO;
typedef struct {
    struct {
        struct {
            uint32_t u32TotalAudioNum;
            BOOL bPCMPlaying;
            BOOL bBufferEmpty;
        } sNuOneApp;
    } Mic_NuOne;
} S_APP;
typedef struct {
    TEST_READ read;
    void *write;
    TEST_EVENT event;
    void *midi;
} S_AUDIO_CALLBACK;

extern S_APP g_sApp;
extern uint32_t PDMA1CallBackCount;
extern uint32_t g_u32SPKBuffer[2][320];
uint32_t ReadDataCallback(void *buffer, uint32_t address, uint32_t count);
void PlayNuOneSetAudioSize(uint32_t bytes);
BOOL PlayNuOneSequence(const uint8_t *indexes, uint32_t count);
BOOL PlayNuOnebyIdx(UINT8 index);
void PlayLoop(void);
void SPK_Start(void);
void SPK_Stop(void);
BOOL AudioRom_GetAudioChunkInfo(TEST_READ read, uint32_t base, uint32_t index,
                               S_ROM_AUDIO_CHUNK_INFO *info);
UINT32 NuOneEx_DecodeInitiate(UINT8 *work, UINT8 *temp, uint32_t address, TEST_READ read);
BOOL NuOneEx_DecodeIsEnd(UINT8 *work);
INT32 NuOneEx_DecodeProcess(UINT8 *work, UINT8 *temp, INT16 *out, TEST_READ read, TEST_EVENT event);

#endif

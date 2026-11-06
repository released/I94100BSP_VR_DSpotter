#include "PlayNuOne.h"
#include "AudioCommon.h"
#include "coder_88C22_88L21_Dmic_Dpwm_8315.h"
#include <stdio.h>
#include <string.h>

S_ROM_AUDIO_CHUNK_INFO sAudioChunkInfo;
S_APP g_sApp;
BOOL bLastOneBuffer;
UINT16 u16SampleRate;
uint32_t PDMA1CallBackCount;

__align(4) UINT32 au32DecodeWorkBuf[(NUONEEX_DECODE_WORK_BUF_SIZE + 3) / 4];
__align(4) UINT32 u32NuOneExTempBuf[(NUONEEX_DECODE_TEMP_BUF_SIZE + 3) / 4];
INT16 i16OutBuf[NUONEEX_DECODE_SAMPLE_PER_FRAME];

/** add code start **/
static uint32_t audio_rom_bytes;
static uint8_t reply_indexes[AIRCON_REPLY_MAX_PARTS];
static uint32_t reply_count;
static uint32_t reply_position;
static uint32_t reply_gap_frames;
/** add code end **/

uint32_t UserEvent_ProcessCallback(uint16_t event_index, uint16_t event_subindex);
extern u32 g_u32SPKBuffer[2][NUONEEX_DECODE_SAMPLE_PER_FRAME];
extern void SPK_Stop(void);

S_AUDIO_CALLBACK const g_asAppCallBack[] = {
    { ReadDataCallback, NULL, NULL, NULL }
};

/** add code start **/
void PlayNuOneSetAudioSize(uint32_t bytes)
{
    audio_rom_bytes = bytes;
}

uint32_t ReadDataCallback(void *buffer, uint32_t address, uint32_t count)
{
    uint32_t offset;

    if (buffer == NULL || address < AUDIOROM_STORAGE_START_ADDR)
        return 0U;
    offset = address - AUDIOROM_STORAGE_START_ADDR;
    if (offset > audio_rom_bytes || count > audio_rom_bytes - offset)
        return 0U;
    memcpy(buffer, (const void *)(uintptr_t)address, count);
    return count;
}
/** add code end **/

uint32_t UserEvent_ProcessCallback(uint16_t event_index, uint16_t event_subindex)
{
    (void)event_index;
    (void)event_subindex;
    return 0U;
}

void duplicate(uint8_t index)
{
    uint16_t i;
    uint32_t sample;

    for (i = 0U; i < NUONEEX_DECODE_SAMPLE_PER_FRAME; ++i) {
        sample = (uint16_t)i16OutBuf[i];
        g_u32SPKBuffer[index][i] = (sample << 16) | sample;
    }
}

/** add code start **/
static BOOL BeginReplyPart(uint8_t index)
{
    if (index >= g_sApp.Mic_NuOne.sNuOneApp.u32TotalAudioNum
        || !AudioRom_GetAudioChunkInfo(ReadDataCallback,
            AUDIOROM_STORAGE_START_ADDR, index, &sAudioChunkInfo))
        return FALSE;
    u16SampleRate = (UINT16)NuOneEx_DecodeInitiate((UINT8 *)au32DecodeWorkBuf,
        (UINT8 *)u32NuOneExTempBuf, sAudioChunkInfo.u32AudioChunkAddr,
        ReadDataCallback);
    return u16SampleRate == 16000U;
}

static BOOL NextReplyFrame(void)
{
    INT32 decoded;

    for (;;) {
        memset(i16OutBuf, 0, sizeof(i16OutBuf));
        if (reply_count == 0U)
            return FALSE;
        if (reply_gap_frames != 0U) {
            --reply_gap_frames;
            return TRUE;
        }
        if (!NuOneEx_DecodeIsEnd((UINT8 *)au32DecodeWorkBuf)) {
            decoded = NuOneEx_DecodeProcess((UINT8 *)au32DecodeWorkBuf,
                (UINT8 *)u32NuOneExTempBuf, i16OutBuf, ReadDataCallback,
                UserEvent_ProcessCallback);
            if (decoded < 0) {
                printf("[AUDIO_ERROR] reason=decode code=%ld\n", (long)decoded);
                reply_count = 0U;
                return FALSE;
            }
            return TRUE;
        }
        if (reply_position + 1U >= reply_count)
            return FALSE;
        ++reply_position;
        if (!BeginReplyPart(reply_indexes[reply_position])) {
            printf("[AUDIO_ERROR] reason=next_part index=%u\n",
                (unsigned int)reply_indexes[reply_position]);
            reply_count = 0U;
            return FALSE;
        }
        reply_gap_frames = AIRCON_REPLY_GAP_FRAMES;
    }
}

BOOL PlayNuOneSequence(const uint8_t *indexes, uint32_t count)
{
    uint32_t i;

    if (indexes == NULL || count == 0U || count > AIRCON_REPLY_MAX_PARTS
        || g_sApp.Mic_NuOne.sNuOneApp.bPCMPlaying)
        return FALSE;
    for (i = 0U; i < count; ++i) {
        if (indexes[i] >= g_sApp.Mic_NuOne.sNuOneApp.u32TotalAudioNum)
            return FALSE;
        reply_indexes[i] = indexes[i];
    }
    reply_count = count;
    reply_position = 0U;
    reply_gap_frames = 0U;
    if (!BeginReplyPart(reply_indexes[0]))
        return FALSE;
    PDMA1CallBackCount = 0U;
    g_sApp.Mic_NuOne.sNuOneApp.bBufferEmpty = FALSE;
    bLastOneBuffer = FALSE;
    if (!NextReplyFrame())
        return FALSE;
    duplicate(0U);
    if (!NextReplyFrame())
        bLastOneBuffer = TRUE;
    duplicate(1U);
    g_sApp.Mic_NuOne.sNuOneApp.bPCMPlaying = TRUE;
    SPK_Start();
    return TRUE;
}
/** add code end **/

BOOL PlayNuOnebyIdx(UINT8 index)
{
    return PlayNuOneSequence(&index, 1U);
}

/** add code start **/
void PlayLoop(void)
{
    uint8_t half;

    if (!g_sApp.Mic_NuOne.sNuOneApp.bBufferEmpty)
        return;
    g_sApp.Mic_NuOne.sNuOneApp.bBufferEmpty = FALSE;
    half = (uint8_t)((PDMA1CallBackCount & 1U) != 0U ? 0U : 1U);
    if (NextReplyFrame()) {
        duplicate(half);
        bLastOneBuffer = FALSE;
    } else {
        memset(i16OutBuf, 0, sizeof(i16OutBuf));
        duplicate(half);
        if (bLastOneBuffer) {
            SPK_Stop();
            g_sApp.Mic_NuOne.sNuOneApp.bPCMPlaying = FALSE;
            g_sApp.Mic_NuOne.sNuOneApp.bBufferEmpty = FALSE;
            reply_count = 0U;
        } else {
            bLastOneBuffer = TRUE;
        }
    }
}
/** add code end **/


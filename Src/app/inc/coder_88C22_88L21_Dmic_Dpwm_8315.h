#ifndef __NAU88C22_H__
#define __NAU88C22_H__
#include "c_stdfx.h"

typedef enum
{
    Sound_Left = 1,
    Sound_Right,
} S_Track;
typedef enum
{
    LIN_MIC = 1,
    LIN_AUX,
} S_MICch;
typedef struct
{
    uint8_t VR_Result;
} S_Mic_APP;


typedef struct
{
    uint8_t  u32TotalAudioNum;
    uint8_t  u32PlayID;
    uint8_t  bPCMPlaying;
    uint8_t  bBufferEmpty;


} S_NuOne_APP;

typedef struct
{
    union
    {
        S_Mic_APP  sMicGetApp;
        S_NuOne_APP  sNuOneApp;
    } Mic_NuOne;

} S_APP;

typedef struct
{
    uint16_t uart_tx_len;
    uint8_t uart_tx_data[320*4];
    uint16_t uart_rx_len;
    uint8_t uart_rx_data[1000];
    uint16_t uart_tx_per;
    uint16_t uart_rx_time;
    uint8_t record_ser;
    uint8_t voice_ch;
} uart_type;

extern  S_APP   g_sApp;
extern uart_type ap_uart0;
extern void NAU88C22_Init(void);
extern void I2S0_Init(void);
extern void I2S_PDMA_Init(void);
extern BOOL NAU88C22_WirteReg(u8 _Register, u8 _RegValue);
extern BOOL NAU88C22_SetPGAGAIN(S_Track _Track, float _GaindB);
extern BOOL NAU88C22_SetDACGAIN(S_Track _Track, float _GaindB);
extern BOOL NAU88C22_SetAuxInputSelect(S_Track _Track, S_MICch _MICch);
extern BOOL NAU88C22_SetAuxIputADCBoost(S_Track _Track, float _GaindB);

void CODEC_I2C1_Init(void);
void NAU88L21_LineIn_Init(void);

void DMIC_Init(uint32_t u32SampleRate, uint8_t ChannelNum);
void DPWM_init(uint32_t u32SamleRate);


extern void MIC_Start(void);
extern void MIC_Stop(void);
extern void SPK_Start(void);
extern void SPK_Stop(void);






#endif

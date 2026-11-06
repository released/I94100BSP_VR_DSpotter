/******************************************************************************
 * @file     NAU88C22.c
 * @brief    I94100 series USBD driver Sample file
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

/*!<Includes */
#include "coder_88C22_88L21_Dmic_Dpwm_8315.h"
#include "I94100.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Define Constant                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/




// Microphone(88C22) ============================================================================================
#define MIC_I2C1_PIN_MASK  (SYS_GPD_MFPL_PD0MFP_Msk|SYS_GPD_MFPL_PD1MFP_Msk)
#define MIC_I2C1_PIN       (SYS_GPD_MFPL_PD0MFP_I2C1_SCL|SYS_GPD_MFPL_PD1MFP_I2C1_SDA)
#define MIC_I2C1_BUS_FREQ  (100000)
#define MIC_I2C1_DEV_ADDR  (0x1A)

// I2S ============================================================================================
#define MIC_I2S0_MCLK_FREQ (256*REC_RATE)    // 256*Fs
#define MIC_I2S0_PIN_MASK  (SYS_GPD_MFPL_PD2MFP_Msk|SYS_GPD_MFPL_PD3MFP_Msk|SYS_GPD_MFPL_PD4MFP_Msk|SYS_GPD_MFPL_PD5MFP_Msk|SYS_GPD_MFPL_PD6MFP_Msk)
#define MIC_I2S0_PIN       (SYS_GPD_MFPL_PD2MFP_I2S0_MCLK|SYS_GPD_MFPL_PD3MFP_I2S0_LRCK|SYS_GPD_MFPL_PD4MFP_I2S0_DI|SYS_GPD_MFPL_PD5MFP_I2S0_DO|SYS_GPD_MFPL_PD6MFP_I2S0_BCLK)

/*---------------------------------------------------------------------------------------------------------*/
/* Define Variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct
{
    u8  u8DeviceAddr;
    u16 u16Counter;
    u16 u16MaxCount;
    u8* pau8Cmd;
} S_MIC_I2CCTRL;

typedef struct
{
    u8  u8Reg[1];
    u8  u8Value[1];
} S_MIC_I2CCMD;

volatile S_MIC_I2CCTRL s_MIC_I2CCtrl;       // Provide microphone send command to 85L40S_BUFCTRL* psSPK_BufCtrl = NULL;
// Microphone() Command =======================================================================
// Command for 88C22(transfer via I2C1)
S_MIC_I2CCMD const asMIC_Cmd_88C22[] =
{
    {   (0x00<<1    )+  0x00,   0x00    },         // Reset all registers
    {   (0x01<<1    )+  0x01,   0x1F    },         //Enable PWR  org:0xDF
	{   (0x02<<1    )+  0x00,   0x3F    },
//-------
    {   (0x03<<1    )+  0x00,   0x0F    },         //Enable L/R main mixer, DAC  org:0x1EF
    {   (0x04<<1    )+  0x00,   0x10    },         //16-bit word length, I2S format, Stereo
    {   (0x05<<1    )+  0x00,   0x00    },         //Companding control and loop back mode (all disable)
    {   (0x06<<1    )+  0x00,   0x00    },         //Divide by 3, 16K
    {   (0x07<<1    )+  0x00,   0x06    },         //16K for internal filter coefficients

    {   (0x0A<<1    )+  0x00,   0x08    },         // DAC soft mute is disabled, DAC oversampling rate is 128x
    {   (0x0B<<1    )+  0x01,   0xFF    },         // DAC Left volume  org:0xEF
    {   (0x0C<<1    )+  0x01,   0xFF    },         // DAC Right volume  org:0xEF

    {   (0x0E<<1    )+  0x01,   0x88    },         // ADC HPF filter is enabled, ADC oversampling rate is 128x org:0x198 (102Hz)
    {   (0x0F<<1    )+  0x01,   0xF6    },         // ADC left digital volume control

    {   (0x10<<1    )+  0x01,   0xF6    },         // ADC right digital volume control
//EQ1~EQ5
    {   (0x12<<1    )+  0x01,   0x2c    },		   // EQ on DAC
    {   (0x13<<1    )+  0x00,   0x2c    },
    {   (0x14<<1    )+  0x00,   0x2c    },
    {   (0x15<<1    )+  0x00,   0x2c    },
    {   (0x16<<1    )+  0x00,   0x2c    },         //5.3Khz +6db org:0x06 on ADC
// ALC Control
    {   (0x20<<1    )+  0x00,   0x38    },         //ADC right enable alc org:0xb8
    {   (0x21<<1    )+  0x00,   0x0b    },
    {   (0x22<<1    )+  0x00,   0x32    },
//
    {   (0x2B<<1    )+  0x00,   0x1A    },		   // org:0x1A
    {   (0x2D<<1    )+  0x01,   0x38    },         // Left PGA gain org:0x12C(+21dB), 0x138(+30dB)
    {   (0x2E<<1    )+  0x01,   0x38    },         // Right PGA gain org:0x12c

    {   (0x2F<<1    )+  0x01,   0x00    },         // LPGA boost 20dB enable
    {   (0x30<<1    )+  0x01,   0x00    },         // RPGA boost 20dB enable
    {   (0x2C<<1    )+  0x00,   0x33    },         // Left and Right MICN/MICP is connected to PGA

    {   (0x31<<1    )+  0x00,   0x02    },		   // org:0x62
    {   (0x32<<1    )+  0x00,   0x1d    },         // Left  connected to LMIX
    {   (0x33<<1    )+  0x00,   0x1d    },         // Right  connected to RMIX

    {   (0x34<<1    )+  0x00,   0x39    },
    {   (0x35<<1  	)+  0x00,   0x39    },
    {   (0x36<<1    )+  0x01,   0x39    },			//org:0x139
    {   (0x37<<1    )+  0x01,   0x39    },			//org:0x139

    {   (0x38<<1    )+  0x00,   0x40    },			//org:0x00
    {   (0x39<<1    )+  0x00,   0x40    },			//org:0x00
	{   (0x01<<1    )+  0x01,   0x1D    },          //Set REFIMP to 80kohm
	{   (0x02<<1    )+  0x01,   0xBF    },			//HP driver enable
	{   (0x03<<1    )+  0x00,   0x6F    },			//SPK driver enable

};

/*---------------------------------------------------------------------------------------------------------*/
/* Functions Definition                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/

static void delay_us(uint32_t us)
{
    u32 i,j;
    for(i=0; i<us; i++)
        for(j=0; j<2000; j++);
}


// ==========================================================================================
void MIC_Start(void)
{
//    PDMA->CHRST |= (1<<SPK_PDMA_CH) ;
//    PDMA_ENABLE_CHANNEL((1<<MIC_PDMA_CH));
//    PDMA->SWREQ |= ((1 << MIC_PDMA_CH));//»Ìº˛¥•∑¢
#if (MIC_PDMA_16BIT ==2)  //2:DMIC
    DMIC_ENABLE_CHANNEL(DMIC, DMIC_CTL_CH1|DMIC_CTL_CH0);
    DMIC_ENABLE_PDMA(DMIC);
#else 										 //0:88C22 //1:88L21//3:85L40
    I2S_CLR_RX_FIFO(I2S0);
 //   I2S_ENABLE(I2S0);
    I2S_ENABLE_RX(I2S0);
    I2S_ENABLE_RXDMA(I2S0);    
#endif
}
void MIC_Stop(void)
{
//    PDMA_DISABLE_CHANNEL((1<<MIC_PDMA_CH));
#if (MIC_PDMA_16BIT ==2)  //2:DMIC
    DMIC_DISABLE_CHANNEL(DMIC, DMIC_CTL_CH1|DMIC_CTL_CH0);
    DMIC_DISABLE_PDMA(DMIC);
#else 										 //0:88C22 //1:88L21//3:85L40
//    I2S_DISABLE(I2S0);
    I2S_DISABLE_RX(I2S0);
    I2S_DISABLE_RXDMA(I2S0);
#endif
}

// 0:88c22, 1:8315,  2:DPWM
void SPK_Start(void)
{
//    PDMA_CONFIG.rec_en = 0;
//    PDMA_CONFIG.play_en = 1;
//    PDMA_Init(&PDMA_CONFIG);
//	PDMA_SetTransferMode(SPK_PDMA_CH, PDMA_I2S0_TX, TRUE, (uint32_t)&DMA_TXDESC[0]);
//    PDMA->SWREQ |= ((1 << SPK_PDMA_CH));//»Ìº˛¥•∑¢
//    PDMA->CHRST |= (1<<SPK_PDMA_CH) ;
//    PDMA_ENABLE_CHANNEL((1<<SPK_PDMA_CH));
#if (SPK_PDMA_16BIT ==2)  //2:DPWM
    DPWM_CLEAR_FIFO(DPWM);
    DPWM_START_PLAY(DPWM);
    DPWM_ENABLE_PDMA(DPWM);
#else 										 //0:88C22 //1:88L21
    I2S_CLR_TX_FIFO(I2S0);
 //   I2S_ENABLE(I2S0);
    I2S_ENABLE_TX(I2S0);
    I2S_ENABLE_TXDMA(I2S0);
#endif
}
void SPK_Stop(void)
{
//    PDMA->SWREQ &= (~((1 << SPK_PDMA_CH)));//»Ìº˛¥•∑¢
//    PDMA_DISABLE_CHANNEL((1<<SPK_PDMA_CH));
#if (SPK_PDMA_16BIT ==2)  //2:DPWM
    DPWM_STOP_PLAY(DPWM);
    DPWM_DISABLE_PDMA(DPWM);
#else 										 //0:88C22 //1:88L21
//    I2S_DISABLE(I2S0);
    I2S_DISABLE_TX(I2S0);
    I2S_DISABLE_TXDMA(I2S0);
//    I2S_CLR_TX_FIFO(I2S0);
#endif
}


void NAU88C22_Init(void)
{
    u16 u16i, delaycnt=0;
    SYS_ResetModule(I2C1_RST);    // Reset module.
    CLK_EnableModuleClock(I2C1_MODULE); // Enable I2C0 module clock.
    I2C_Open(I2C1, MIC_I2C1_BUS_FREQ);    // Open I2C module and set bus clock.

    I2C_EnableInt(I2C1);    // Enable I2C interrupt.
    NVIC_EnableIRQ(I2C1_IRQn);

    // GPIO multi-function. (GPD0:I2C1_SCL, GPD1:I2C1_SDA)
    SYS->GPD_MFPL = (SYS->GPD_MFPL & ~MIC_I2C1_PIN_MASK)|MIC_I2C1_PIN;

    // (1-2) Send command to 85L40 via I2C1
    I2C_SetBusClockFreq(I2C1,MIC_I2C1_BUS_FREQ);
    s_MIC_I2CCtrl.u8DeviceAddr = MIC_I2C1_DEV_ADDR;

    for(u16i=0; u16i<sizeof(asMIC_Cmd_88C22)/sizeof(S_MIC_I2CCMD); u16i++)
    {
        s_MIC_I2CCtrl.pau8Cmd = (uint8_t*)&asMIC_Cmd_88C22[u16i];
        s_MIC_I2CCtrl.u16Counter = 0;
        s_MIC_I2CCtrl.u16MaxCount = sizeof(S_MIC_I2CCMD);
        I2C_START(I2C1);
        /* Wait for I2C transmit completed. */
        while(s_MIC_I2CCtrl.u16MaxCount>0);
    }
}

//8315 UP read data
void I2S0_Init(void)
{
    u16 pdma_len;

    // (2) Configure I2S to ge voice data from 85L40
    {
        // PD2: I2S0_MCLK, PD3: I2S0_LRCK, PD4: I2S0_DI, PD5: I2S0_DO, PD6: I2S0_BCLK
        SYS->GPD_MFPL = (SYS->GPD_MFPL & ~MIC_I2S0_PIN_MASK)|MIC_I2S0_PIN;
        // Enable I2S clock.
        CLK_EnableModuleClock(I2S0_MODULE);
        // Select I2S clock.
        CLK_SetModuleClock(I2S0_MODULE, CLK_CLKSEL3_I2S0SEL_PCLK0, NULL);
        // I2S IPReset.
        SYS_ResetModule(I2S0_RST);
        // Open I2S and enable master clock.

#if ( (REC_CHANNELS == 1) || (REC_CHANNELS == 2) )
        {
            I2S_Open(I2S0, I2S_MASTER, REC_RATE, I2S_DATABIT_16, I2S_TDMCHNUM_2CH, I2S_STEREO, I2S_FORMAT_I2S);
            pdma_len = AMIC2PDMA_BUFF_LEN_2CH;
        }
#else
        {
            I2S_Open(I2S0, I2S_MASTER, REC_RATE, I2S_DATABIT_16, I2S_TDMCHNUM_4CH, I2S_STEREO, I2S_FORMAT_I2S);
            pdma_len = AMIC2PDMA_BUFF_LEN_34CH;
        }
#endif
        // Enable I2C0 MCLK
        I2S_EnableMCLK(I2S0, MIC_I2S0_MCLK_FREQ);

        // I2S Configuration.
        I2S_SET_PCMSYNC(I2S0, I2S_PCMSYNC_BCLK);

        // Set RX channel for Mono mode.
#if (REC_CHANNELS == 1)
        {
            I2S_SET_MONO_RX_CHANNEL(I2S0, I2S_MONO_RX_LEFT);
        }
#else
        {
            I2S_SET_MONO_RX_CHANNEL(I2S0, I2S_MONO_RX_RIGHT);
        }
#endif

        // Set data order.
        I2S_SET_STEREOORDER(I2S0, I2S_ORDER_EVENLOW);
        // Enable I2S RXDMA
//#if  (MIC_PDMA_16BIT==0)
//        I2S_ENABLE_RXDMA(I2S0);
//#endif
//        I2S_ENABLE_TXDMA(I2S0);

        // Set channel width.
#if (AMIC_BIT_RES == 16)
        I2S_SET_CHWIDTH(I2S0, I2S_CHWIDTH_16);
#endif
#if (AMIC_BIT_RES == 32)
        I2S_SET_CHWIDTH(I2S0, I2S_CHWIDTH_32);
#endif
        // Set 16 bits data width.
        I2S_SET_PBWIDTH(I2S0, I2S_PBWIDTH_16);
        // Set FIFO Read/Write Order in 16-bit Width of Peripheral Bus.
        I2S_SET_PB16ORD(I2S0, I2S_PB16ORD_LOW);
        // Set FIFO threshold.
//        I2S_SET_RXTH(I2S0, I2S_FIFO_RX_LEVEL_WORD_9);
//        I2S_SET_TXTH(I2S0, I2S_FIFO_TX_LEVEL_WORD_9);
        // Clear TX, RX FIFO buffer
//        I2S_CLR_TX_FIFO(I2S0);
//        I2S_CLR_RX_FIFO(I2S0);
    }
	
	 I2S_ENABLE(I2S0);
}

volatile uint8_t g_u8CodecInit = 0;
volatile uint16_t g_u16CodecTimeOutCnt = 0;

typedef struct {
	uint8_t  u8Reg[2];
	uint8_t  u8Value[2];
} S_MIC_I2CCMD_2;

S_MIC_I2CCMD_2 asMIC_Cmd_88L21_MicIn[] = {
{	0x00	,	0x00	,	0xFF	,	0xFF	}	,
{	0x00	,	0x00	,	0xFF	,	0xFF	}	,
{	0x00	,	0x31	,	0x03	,	0x00	}	,	//0x0300
{	0x00	,	0x66	,	0x00	,	0x60	}	,
{	0x00	,	0x76	,	0x11	,	0x40	}	,
{	0x00	,	0x01	,	0x0F	,	0xD6	}	,
{	0x00	,	0x03	,	0x00	,	0x00	}	,	//MCLK_DIV=1
{	0x00	,	0x1D	,	0x00	,	0x00	}	,
{	0x00	,	0x72	,	0x01	,	0x70	}	,
{	0x00	,	0x2B	,	0x00	,	0x03	}	,
{	0x00	,	0x73	,	0x33	,	0x08	}	,
{	0x00	,	0x76	,	0x71	,	0x40	}	,
{	0x00	,	0x32	,	0x80	,	0x00	}	,	//0x8000
{	0x00	,	0x34	,	0xCF	,	0xCF	}	,	//DAC Gain
{	0x00	,	0x35	,	0xFB	,	0xFB	}	,	//mic,DGAIN_ADC org:0xF3F3(+18dB), 0xFBFB(+22dB)
{	0x00	,	0x4B	,	0x20	,	0x07	}	,
{	0x00	,	0x6A	,	0x10	,	0x03	}	,	//HP current control
{	0x00	,	0x80	,	0x00	,	0x20	}	,	//Charge pump and power down control
{	0x00	,	0x74	,	0x15	,	0x06	}	,
{	0x00	,	0x77	,	0x00	,	0x00	}	,	//PGA_GAIN
{	0x00	,	0x7E	,	0x15	,	0x15	}	,	//mic,PGA_GAIN org:0x1111(+16dB), 0x1F1F(+30dB),0x1515(+20dB)
{	0x00	,	0x7F	,	0x00	,	0x30	}	,	//Power up control sequence0
{	0x00	,	0x7F	,	0x00	,	0x3C	}	,	//Power up control sequence1
{	0x00	,	0x7F	,	0xC0	,	0x3F	}	,	//Power up control sequence2
{	0x00	,	0x32	,	0x00	,	0x00	}	,
{	0x00	,	0x80	,	0x07	,	0x20	}	,
{	0x00	,	0x0D	,	0x00	,	0x00	}	,	//0xc000, caused HPL and HPR have 0.9V DC
{	0x00	,	0x31	,	0x10	,	0x00	}   ,
{	0x00	,	0x21	,	0x23	,	0xCE	}   ,
{	0x00	,	0x22	,	0x00	,	0x06	}   ,
{	0x00	,	0x23	,	0xDE	,	0x80	}   ,
{	0x00	,	0x24	,	0x00	,	0x00	}   ,
{	0x00	,	0x25	,	0xEE	,	0xB0	}   ,
{	0x00	,	0x26	,	0x00	,	0x00	}   ,
{	0x00	,	0x27	,	0x22	,	0xA0	}   ,
{	0x00	,	0x28	,	0x00	,	0x06	}   ,
{	0x00	,	0x29	,	0xEE	,	0xB0	}   ,
{	0x00	,	0x2A	,	0x00	,	0x08	}   ,
};

void NAU88L21_LineIn_Init(void)
{
    uint16_t u16i;
    s_MIC_I2CCtrl.u8DeviceAddr = 0x1B;
    
    g_u8CodecInit = 1;
    g_u16CodecTimeOutCnt = 0;    
    
    for(u16i=0;u16i<sizeof(asMIC_Cmd_88L21_MicIn)/sizeof(S_MIC_I2CCMD_2);u16i++) 
    {
        s_MIC_I2CCtrl.pau8Cmd = (uint8_t*)&asMIC_Cmd_88L21_MicIn[u16i];
        s_MIC_I2CCtrl.u16Counter = 0;
        s_MIC_I2CCtrl.u16MaxCount = sizeof(S_MIC_I2CCMD_2);
        I2C_START(I2C1);
        /* Wait for I2C transmit completed. */
        while((s_MIC_I2CCtrl.u16MaxCount>0));// && (g_u16CodecTimeOutCnt < 100)); // 100ms timeout
            
    }
    g_u8CodecInit = 0;
    g_u16CodecTimeOutCnt = 0;
}

#define I2C1_MFP 		    	SYS->GPD_MFPH
#define I2C1_PIN_MASK          (SYS_GPD_MFPH_PD14MFP_Msk|SYS_GPD_MFPH_PD15MFP_Msk)
#define I2C1_PIN               (SYS_GPD_MFPH_PD14MFP_I2C1_SCL|SYS_GPD_MFPH_PD15MFP_I2C1_SDA)

void CODEC_I2C1_Init(void)
{
    // Reset module.
    SYS_ResetModule(I2C1_RST);
    // Enable I2C0 module clock.
    CLK_EnableModuleClock(I2C1_MODULE);
    // Open I2C module and set bus clock.
    I2C_Open(I2C1, 100000);

	I2C1_MFP = (I2C1_MFP & ~I2C1_PIN_MASK)|I2C1_PIN;
	
    I2C_EnableInt(I2C1);
	NVIC_ClearPendingIRQ(I2C1_IRQn);
    NVIC_EnableIRQ(I2C1_IRQn);	
}


void I2C1_IRQHandler(void)
{
    if(I2C_GET_TIMEOUT_FLAG(I2C1))
        I2C_ClearTimeoutFlag(I2C1);
    else
    {
        switch(I2C_GET_STATUS(I2C1))
        {
        // START has been transmitted and Write SLA+W to Register I2CDAT.
        case 0x08:
            I2C_SET_DATA(I2C1, s_MIC_I2CCtrl.u8DeviceAddr << 1);
            I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI);
            break;
        // SLA+W has been transmitted and ACK has been received.
        case 0x18:
            I2C_SET_DATA(I2C1, s_MIC_I2CCtrl.pau8Cmd[s_MIC_I2CCtrl.u16Counter++]);
            I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI);
            break;
        // SLA+W has been transmitted and NACK has been received.
        case 0x20:
            I2C_STOP(I2C1);
            I2C_START(I2C1);
            s_MIC_I2CCtrl.u16MaxCount = 0;
            break;
        // DATA has been transmitted and ACK has been received.
        case 0x28:
            if(s_MIC_I2CCtrl.u16Counter < s_MIC_I2CCtrl.u16MaxCount)
            {
                I2C_SET_DATA(I2C1, s_MIC_I2CCtrl.pau8Cmd[s_MIC_I2CCtrl.u16Counter++]);
                I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI);
            }
            else
            {
                I2C_SET_CONTROL_REG(I2C1, I2C_CTL_STO_SI);
                // transfer complete
                s_MIC_I2CCtrl.u16MaxCount = 0;
            }
            break;
        }
    }
}
/*
* NAU88C22_WirteReg
* _Register ÂØÑÂ≠òÂô®Âú∞ÂùÄ
* _RegValue ÂØÑÂ≠òÂô®Êï∞ÊçÆ
* return true or false
*/
BOOL NAU88C22_WirteReg(u8 _Register, u8 _RegValue)
{
    u16 delaycnt=0;
    u8  Cmd[2];
    Cmd[0] = _Register;
    Cmd[1] = _RegValue;
    s_MIC_I2CCtrl.pau8Cmd = (uint8_t*)Cmd;
    s_MIC_I2CCtrl.u16Counter = 0;
    s_MIC_I2CCtrl.u16MaxCount = sizeof(Cmd);
    I2C_START(I2C1);
    /* Wait for I2C transmit completed. */
    while(s_MIC_I2CCtrl.u16MaxCount>0)
    {
        delay_us(50);
        if (++delaycnt >= 500)
        {
            return FALSE;
        }
    }
    return TRUE;
}
/*
* NAU88C22_SetPGAGAIN
* _Track Â£∞ÈÅì
* _GaindB db ËåÉÂõ¥ -12Ëá≥+35.25
* return true or false
*/
BOOL NAU88C22_SetPGAGAIN(S_Track _Track, float _GaindB)
{
    u8 Gain = 0;
    BOOL ret = FALSE;
    float GainTempdB = _GaindB;
    if(GainTempdB < -12)
        GainTempdB = -12;
    else if(GainTempdB > 35.25)
        GainTempdB = 35.25;
    GainTempdB = GainTempdB + 12;
    Gain = (uint8_t)(GainTempdB/0.75);
    switch(_Track)
    {
    case Sound_Left:
        ret = NAU88C22_WirteReg((0x2D<<1)+0x01,Gain);
        break;
    case Sound_Right:
        ret = NAU88C22_WirteReg((0x2E<<1)+0x01,Gain);
        break;
    }
    return ret;
}
/*
* NAU88C22_SetDACGAIN
* _Track Â£∞ÈÅì
* _GaindB db ËåÉÂõ¥ -127Ëá≥+0
* return true or false
*/
BOOL NAU88C22_SetDACGAIN(S_Track _Track, float _GaindB)
{
    u8 Gain = 0;
    BOOL ret = FALSE;
    float GainTempdB = _GaindB;
    if(GainTempdB < -127)
        GainTempdB = -127;
    else if(GainTempdB > 0)
        GainTempdB = 0;
    GainTempdB = GainTempdB + 127;
    Gain = (uint8_t)(GainTempdB/0.5);
    switch(_Track)
    {
    case Sound_Left:
        ret = NAU88C22_WirteReg((0x0B<<1)+0x01,Gain);
        break;
    case Sound_Right:
        ret = NAU88C22_WirteReg((0x0C<<1)+0x01,Gain);
        break;
    }
    return ret;
}
/*
* NAU88C22_SetAuxIputADCBoost
* _Track Â£∞ÈÅì
* _GaindB db ËåÉÂõ¥ -12Ëá≥+6
* return true or false
*/
BOOL NAU88C22_SetAuxIputADCBoost(S_Track _Track, float _GaindB)
{
    u8 Gain = 0;
    BOOL ret = FALSE;
    float GainTempdB = _GaindB;
    if(GainTempdB < -12)
        GainTempdB = -12;
    else if(GainTempdB > 6)
        GainTempdB = 6;
    GainTempdB = GainTempdB + 12;
    Gain = (uint8_t)(GainTempdB/3)+1;
    switch(_Track)
    {
    case Sound_Left:
        ret = NAU88C22_WirteReg((0x2F<<1)+0x00,Gain);
        break;
    case Sound_Right:
        ret = NAU88C22_WirteReg((0x30<<1)+0x00,Gain);
        break;
    }
    return ret;
}

/*
* NAU88C22_SetDACGAIN
* _Track Â£∞ÈÅì
* _GaindB db ËåÉÂõ¥ -127Ëá≥+0
* return true or false
*/
BOOL NAU88C22_SetAuxInputSelect(S_Track _Track, S_MICch _MICch)
{
    static u8 IO = 0x33;
    BOOL ret = FALSE;
    switch(_Track)
    {
    case Sound_Left:
        if(_MICch == LIN_MIC)
        {
            Setbit(IO,0);
            Setbit(IO,1);
            //Clrbit(IO,2);
            NAU88C22_WirteReg((0x2F<<1)+0x01,0x00);
        }
        else
        {
            Clrbit(IO,0);
            Clrbit(IO,1);
            //Setbit(IO,2);
            NAU88C22_WirteReg((0x2F<<1)+0x00,0x02);
        }
        ret = NAU88C22_WirteReg((0x2C<<1)+0x00,IO);
        break;
    case Sound_Right:
        if(_MICch == LIN_MIC)
        {
            Setbit(IO,4);
            Setbit(IO,5);
            //Clrbit(IO,6);
            NAU88C22_WirteReg((0x30<<1)+0x01,0x00);
        }
        else
        {
            Clrbit(IO,4);
            Clrbit(IO,5);
            //Setbit(IO,6);
            NAU88C22_WirteReg((0x30<<1)+0x00,0x02);
        }
        ret = NAU88C22_WirteReg((0x2C<<1)+0x00,IO);
        break;
    }
    return ret;
}







//----------------------------------------------------------------------------
//  DMIC Initial function
//----------------------------------------------------------------------------
void DMIC_Init(uint32_t u32SampleRate, uint8_t ChannelNum)
{

    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA0MFP_Msk | SYS_GPA_MFPL_PA1MFP_Msk );
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA0MFP_DMIC_DAT0 | SYS_GPA_MFPL_PA1MFP_DMIC_CLK0);

    // DMIC_CLK Select.
    CLK_SetModuleClock(DMIC_MODULE, CLK_CLKSEL2_DMICSEL_PCLK1, NULL);
    // Enable DMIC clock.
    CLK_EnableModuleClock(DMIC_MODULE);
    // DPWM IP Reset.
    SYS_ResetModule(DMIC_RST);
    // Set Down Sample
    DMIC_ENABLE_DOWMSAMPLE(DMIC, DMIC_CTL_DOWNSAMPLE_128);
    // Set Sample rate
    DMIC_SetSampleRate(DMIC, u32SampleRate);
    DMIC->DIV &= (~(0xff<<8));
    DMIC->DIV |= (1<<8);   //≈‰÷√MCLK
    DMIC->DIV &= (~ 0xff);
    DMIC->DIV |= (23);   //≈‰÷√PCLK

    // Set data latch
    DMIC_SET_LATCHDATA(DMIC, DMIC_CTL_LATCHDATA_CH01R);
    // FIFO threshold
    DMIC_ENABLE_FIFOTHRESHOLDINT(DMIC, 8);
    DMIC_ENABLE_PDMA(DMIC);
    DMIC_DISABLE_CHANNEL(DMIC, DMIC_CTL_CH0|DMIC_CTL_CH1|DMIC_CTL_CH2|DMIC_CTL_CH3);

    // ch1 - ch0 - ch3 - ch2
    switch (ChannelNum)
    {
    case 0x1:
        DMIC_ENABLE_CHANNEL(DMIC, DMIC_CTL_CH1);
        break;
    case 0x2:
        DMIC_ENABLE_CHANNEL(DMIC, DMIC_CTL_CH1|DMIC_CTL_CH0);
        break;
    case 0x3:
        DMIC_ENABLE_CHANNEL(DMIC, DMIC_CTL_CH1|DMIC_CTL_CH0|DMIC_CTL_CH3);
        break;
    case 0x4:
        DMIC_ENABLE_CHANNEL(DMIC, DMIC_CTL_CH1|DMIC_CTL_CH0|DMIC_CTL_CH3|DMIC_CTL_CH2);
        break;
    }
}


void DPWM_Start(void)
{
    DPWM_ENABLE_DRIVER(DPWM);
    DPWM_START_PLAY(DPWM);
    DPWM_ENABLE_PDMA(DPWM);
}
//void DPWM_Close(void)
//{
//	DPWM_DISABLE_DRIVER(DPWM);
//	DPWM_STOP_PLAY(DPWM);
//	DPWM_DISABLE_PDMA(DPWM);
//}

void DPWM_init(uint32_t u32SamleRate)
{
    SYS_UnlockReg();
    CLK_EnableModuleClock(DPWM_MODULE);
    CLK_SetModuleClock(DPWM_MODULE,CLK_CLKSEL2_DPWMSEL_PCLK0, NULL);

    SYS_ResetModule(DPWM_RST);
    DPWM_SET_CLKSET(DPWM,DPWM_CLKSET_512FS);
    DPWM_SetSampleRate(u32SamleRate);

    DPWM_SET_FIFODATAWIDTH(DPWM,DPWM_FIFO_DATAWIDTH_16BITS);
    SYS_LockReg();

//	SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA5MFP_Msk|SYS_GPA_MFPL_PA4MFP_Msk))|(SYS_GPA_MFPL_PA5MFP_DPWM_LP | SYS_GPA_MFPL_PA4MFP_DPWM_LN);
//	SYS->GPA_MFPH = (SYS->GPA_MFPH & ~(SYS_GPA_MFPH_PA11MFP_Msk|SYS_GPA_MFPH_PA10MFP_Msk))|(SYS_GPA_MFPH_PA11MFP_DPWM_RP | SYS_GPA_MFPH_PA10MFP_DPWM_RN);

    SYS->GPC_MFPH = (SYS->GPC_MFPH & ~(SYS_GPC_MFPH_PC12MFP_Msk|SYS_GPC_MFPH_PC13MFP_Msk))|(SYS_GPC_MFPH_PC13MFP_DPWM_LP | SYS_GPC_MFPH_PC12MFP_DPWM_LN);
    SYS->GPC_MFPH = (SYS->GPC_MFPH & ~(SYS_GPC_MFPH_PC10MFP_Msk|SYS_GPC_MFPH_PC11MFP_Msk))|(SYS_GPC_MFPH_PC10MFP_DPWM_RN | SYS_GPC_MFPH_PC11MFP_DPWM_RP);

    DPWM_Start();
}





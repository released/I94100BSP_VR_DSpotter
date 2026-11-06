/******************************************************************************
 * @file     MassStorage.c
 * @brief    M480 series USBD driver Sample file
 *
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

/*!<Includes */
#include <stdio.h>
#include <string.h>
#include "c_stdfx.h"
#include "I94100.h"
#include "usbd_audio.h"
#include "AudioClass.h"
#include "pdma_config.h"
#include "user_cmnd.h"
//#include "w684386Regs.h"


/*---------------------------------------------------------------------------------------------------------*/
/* Extern functions declaration                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
extern void MIC_Start(void);
extern void MIC_Stop(void);
extern void SPK_Start(void);
extern void SPK_Stop(void);

extern void DPWM_Init(uint32_t u32SampleRate);
extern void DPWM_PDMA_Init(PDMA_CONFIG_T *pdma_ctl);

/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables and constants                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
extern void I2S_PDMA_Init(void);
static uint32_t g_u32SpeakerByteCount, g_u32SpeakerByteCountRec;
static uint8_t g_u8AudioSpeakerDebounce, g_u8AudioSpeakerPause;
static uint16_t g_u16StartOfFrameCount;

//
volatile uint8_t g_u8AudioSpeakerState = 0;
volatile uint16_t g_u16PayLoadLen;
volatile uint8_t g_u8USB_IN_Start = 0;


void USBPlayVolumeControl(uint32_t *u32Data);
void USBRecVolumeControl(uint32_t *u32Data);

#if 0
#define DBG_PRINTF      printf
#else
#define DBG_PRINTF(...)
#endif


static __INLINE uint32_t get_be32(uint8_t *buf)
{
    return ((uint32_t) buf[0] << 24) | ((uint32_t) buf[1] << 16) |
           ((uint32_t) buf[2] << 8) | ((uint32_t) buf[3]);
}

void USBD_IRQHandler(void)
{
    uint32_t u32IntSts = USBD_GET_INT_FLAG();
    uint32_t u32State = USBD_GET_BUS_STATE();

    if(u32IntSts & USBD_INTSTS_FLDET)
    {
        // Floating detect
        USBD_CLR_INT_FLAG(USBD_INTSTS_FLDET);

        if(USBD_IS_ATTACHED())
        {
            // Enable GPB15(VBUS) pull down state to solute suspend event issue.
            GPIO_EnablePullState(PB,BIT15,GPIO_PUSEL_PULL_DOWN);
            // USB Plug In
            USBD_ENABLE_USB();
        }
        else
        {
            // Disable GPB15 pull down state.
            GPIO_DisablePullState(PB,BIT15);
            // USB Un-plug
            USBD_DISABLE_USB();
        }
    }

    if ( u32IntSts & USBD_INTSTS_SOFIF_Msk )
    {
        // Clear event flag
        USBD_CLR_INT_FLAG(USBD_INTSTS_SOFIF_Msk);
        if (++g_u16StartOfFrameCount >= 1000)
        {
            g_u16StartOfFrameCount = 0;
            SYS->IRCTCTL = (SYS->IRCTCTL&~(SYS_IRCTCTL_FREQSEL_Msk | SYS_IRCTCTL_REFCKSEL_Msk))| (SYS_IRCTCTL_FREQSEL_48M | SYS_IRCTCTL_REFCKSEL_Msk);
        }

        if ( g_u32SpeakerByteCountRec != g_u32SpeakerByteCount )
        {
            g_u32SpeakerByteCountRec = g_u32SpeakerByteCount;
        }
        else
        {
            g_u8AudioSpeakerDebounce++;
            if (g_u8AudioSpeakerDebounce >= 5)
            {
                g_u8AudioSpeakerDebounce = 0;
                g_u8AudioSpeakerPause = 1;
                g_u32SpeakerByteCount = 0;
                g_u32SpeakerByteCountRec = 0;
            }
        }
    }

    if(u32IntSts & USBD_INTSTS_BUS)
    {
        // Clear event flag
        USBD_CLR_INT_FLAG(USBD_INTSTS_BUS);

        if(u32State & USBD_STATE_USBRST)
        {
            // Bus reset
            USBD_ENABLE_USB();
            USBD_SwReset();
            DBG_PRINTF("Bus reset\n");
        }
        if(u32State & USBD_STATE_SUSPEND)
        {
            // Enable USB but disable PHY
            USBD_DISABLE_PHY();
            DBG_PRINTF("Suspend\n");
        }
        if(u32State & USBD_STATE_RESUME)
        {
            // Enable USB and enable PHY
            USBD_ENABLE_USB();
            DBG_PRINTF("Resume\n");
        }
    }

    if(u32IntSts & USBD_INTSTS_USB)
    {
        // EP events
        if(u32IntSts & USBD_INTSTS_EP0)
        {
            // Clear event flag
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP0);
            // control IN
            USBD_CtrlIn();
        }
        if(u32IntSts & USBD_INTSTS_EP1)
        {
            // Clear event flag
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP1);
            // control OUT
            USBD_CtrlOut();
        }
        if(u32IntSts & USBD_INTSTS_EP2)
        {
            // Clear event flag
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP2);
            EP2_Handler();
        }
        if(u32IntSts & USBD_INTSTS_EP3)
        {
            // Clear event flag
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP3);
          	EP3_Handler();
        }
        if(u32IntSts & USBD_INTSTS_EP4)
        {
            // Clear event flag
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP4);
            EP4_Handler();
        }
        if(u32IntSts & USBD_INTSTS_EP5)
        {
            // Clear event flag
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP5);
            EP5_Handler();
        }
        if(u32IntSts & USBD_INTSTS_EP6)
        {
            // Clear event flag
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP6);
            EP6_Handler();
        }
        if(u32IntSts & USBD_INTSTS_EP7)
        {
            // Clear event flag
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP7);
            EP7_Handler();
        }
        /*
        if(u32IntSts & USBD_INTSTS_EP8)
        {
            // Clear event flag
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP8);
        }
        if(u32IntSts & USBD_INTSTS_EP9)
        {
            // Clear event flag
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP9);
        }
        */
        // USB event
        if(u32IntSts & USBD_INTSTS_SETUP)
        {
            // Setup packet
            // Clear event flag
            USBD_CLR_INT_FLAG(USBD_INTSTS_SETUP);

            // Clear the data IN/OUT ready flag of control end-points
            USBD_STOP_TRANSACTION(EP0);
            USBD_STOP_TRANSACTION(EP1);

            USBD_ProcessSetupPacket();
        }
    }
}

void EP2_Handler(void)
{
    int i;
    uint16_t u16EP2_Byte_Count;
    uint8_t *pu8buf;
    uint16_t  u16Distance;

    if (g_u8Current_Mic_ChannNum != UAC_REC_CHANNEL_2CH)
    {
        USBD_SET_PAYLOAD_LEN(EP2, 0);
        return;
    }
    else if (g_u8USB_IN_Start == 0)
    {
        USBD_SET_PAYLOAD_LEN(EP2, 0);
        return;
    }

    if (g_usbd_UsbAudioState == UAC_BUSY_AUDIO_RECORD)
    {
        if ( g_u16UAC_Buff_WriteIndex >= g_u16UAC_Buff_ReadIndex )
        {
            u16Distance = g_u16UAC_Buff_WriteIndex - g_u16UAC_Buff_ReadIndex;
        }
        else
        {
            u16Distance = (AMIC_RING_BUFFER_LEN_2CH - g_u16UAC_Buff_ReadIndex) + g_u16UAC_Buff_WriteIndex;
        }

        if ( u16Distance > AMIC_BUFF_UPPER_THRE_2CH )
        {
            u16EP2_Byte_Count = RECORD_BYTE_BASE + AMIC_RECORD_ADD;
        }
        else if ( u16Distance < AMIC_BUFF_LOWER_THRE_2CH )
        {
            u16EP2_Byte_Count = RECORD_BYTE_BASE - AMIC_RECORD_SUB;
        }
        else
        {
            u16EP2_Byte_Count = RECORD_BYTE_BASE;
        }

        pu8buf = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP2));
        for ( i= 0 ; i < (u16EP2_Byte_Count/4) ; i++)
        {
            pu8buf[i*4+0] = (g_au32UAC_RingBuff[g_u16UAC_Buff_ReadIndex] >>  0) & 0xff;
            pu8buf[i*4+1] = (g_au32UAC_RingBuff[g_u16UAC_Buff_ReadIndex] >>  8) & 0xff;

            pu8buf[i*4+2] = (g_au32UAC_RingBuff[g_u16UAC_Buff_ReadIndex] >> 16) & 0xff;
            pu8buf[i*4+3] = (g_au32UAC_RingBuff[g_u16UAC_Buff_ReadIndex] >> 24) & 0xff;
            g_u16UAC_Buff_ReadIndex++;

            if (g_u16UAC_Buff_ReadIndex >= AMIC_RING_BUFFER_LEN_2CH )
                g_u16UAC_Buff_ReadIndex = 0;
        }
        USBD_SET_PAYLOAD_LEN(EP2, u16EP2_Byte_Count);
    }
    else
    {
        USBD_SET_PAYLOAD_LEN(EP2, 0);
    }

}
void EP3_Handler(void)
{
    uint32_t i;
    uint16_t u16Outlen;
    uint32_t *pu32Buf;
    uint8_t *pu8buf;

	g_u16PayLoadLen = USBD_GET_PAYLOAD_LEN(EP3);
	g_u32SpeakerByteCount += g_u16PayLoadLen;

//	USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE );
//	return;


    if (g_u8AudioSpeakerState == UAC_BUSY_AUDIO_SPEAK)
    {
        // Set data pu32Tempbuf point to End-Point buffer address.
        pu32Buf = (uint32_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));
        u16Outlen = USBD_GET_PAYLOAD_LEN(EP3);
        u16Outlen >>= 2;
        i = 0;

        while(u16Outlen)
        {
            // Get data from End-Point buffer.
            g_au32USBPlaybackRingBuff[g_u16PlayBackWritePtr] = pu32Buf[i++];
            USBPlayVolumeControl((uint32_t *)&g_au32USBPlaybackRingBuff[g_u16PlayBackWritePtr]);
            g_u16PlayBackWritePtr++;

            // Ring buffer write pointer turn around.
            if( g_u16PlayBackWritePtr >= MAX_I2STX_BUFFER_LEN )
            {
                g_u16PlayBackWritePtr = 0;
            }
            u16Outlen--;
        }

        // Prepare for nex OUT packet.
        USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
        return;
    }
    // Audio start. To setup Buffer Control.
    else if (g_u8AudioSpeakerState == UAC_START_AUDIO_SPEAK)
    {
        // Set Payload to start EP4
        pu8buf = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP4));
        pu8buf[0] = 0x00;
        pu8buf[1] = 0x00;
        pu8buf[2] = 0x04;
        USBD_SET_PAYLOAD_LEN(EP4, 3);

        g_u8AudioSpeakerState = UAC_PROCESS1_AUDIO_SPEAK;
        g_u16PlayBackWritePtr = 0;
        g_u16PlayBackReadPtr = 0;

        pu32Buf = (uint32_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));
        g_u16PayLoadLen = USBD_GET_PAYLOAD_LEN(EP3);

        for ( i = 0 ; i < (g_u16PayLoadLen>>2) ; i++)
        {
            g_au32USBPlaybackRingBuff[g_u16PlayBackWritePtr] = pu32Buf[i];
            g_u16PlayBackWritePtr++;
        }
    }
    // First Audio transfer.
    else if (g_u8AudioSpeakerState == UAC_PROCESS1_AUDIO_SPEAK)
    {
        g_u8AudioSpeakerState = UAC_PROCESS2_AUDIO_SPEAK;

        pu32Buf = (uint32_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));
        g_u16PayLoadLen = USBD_GET_PAYLOAD_LEN(EP3);

        for ( i = 0 ; i < (g_u16PayLoadLen>>2) ; i++)
        {
            g_au32USBPlaybackRingBuff[g_u16PlayBackWritePtr] = pu32Buf[i];
            g_u16PlayBackWritePtr++;
        }
    }
    // Second Audio transfer.
    else if (g_u8AudioSpeakerState == UAC_PROCESS2_AUDIO_SPEAK)
    {
        g_u8AudioSpeakerState = UAC_BUSY_AUDIO_SPEAK;

        pu32Buf = (uint32_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));
        g_u16PayLoadLen = USBD_GET_PAYLOAD_LEN(EP3);

        for ( i = 0 ; i < (g_u16PayLoadLen>>2) ; i++)
        {
            g_au32USBPlaybackRingBuff[g_u16PlayBackWritePtr] = pu32Buf[i];
            g_u16PlayBackWritePtr++;
        }
    }
	USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE );

//#endif	
}
void EP4_Handler(void)
{
    uint8_t *pu8buf;
    uint16_t u16distance;
    uint32_t u32Temp;

    u32Temp = g_u32SpeakerByteCount;
    g_u32SpeakerByteCount = 0;

    if (g_u8AudioSpeakerPause)
    {
        g_u8AudioSpeakerPause = 0;
        pu8buf = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP4));
        pu8buf[0] = 0x00;
        pu8buf[1] = 0x00;
        pu8buf[2] = 0x04;
        USBD_SET_PAYLOAD_LEN(EP4, 3);
        return;
    }
    // Calculate how many data in ring buffer.
    if ( g_u16PlayBackWritePtr >= g_u16PlayBackReadPtr )
    {
        u16distance = g_u16PlayBackWritePtr - g_u16PlayBackReadPtr;
    }
    else
    {
        u16distance = (MAX_I2STX_BUFFER_LEN - g_u16PlayBackReadPtr) + g_u16PlayBackWritePtr;
    }
    // Adjust the amount of data that EP3 transfer from host.
    // Ring buffer is close to overflow.
    if ( u16distance > I2STX_BUFFER_UPPER_THRE )
    {
        u32Temp = USB_BUFFER_EP4_BASE - USB_BUFFER_EP4_SUB;
    }
    // Ring buffer is close to underflow.
    else if ( u16distance < I2STX_BUFFER_LOWER_THRE )
    {
        u32Temp = USB_BUFFER_EP4_BASE + USB_BUFFER_EP4_ADD;
    }
    else
    {
        u32Temp = USB_BUFFER_EP4_BASE;
    }

    pu8buf = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP4));
    pu8buf[0] = (u32Temp>> 0) & 0xff;
    pu8buf[1] = (u32Temp>> 8) & 0xff;
    pu8buf[2] = (u32Temp>>16) & 0xff;
    USBD_SET_PAYLOAD_LEN(EP4, 3);

}
void EP5_Handler(void)
{
    /* Clear event flag */
    USBD_CLR_INT_FLAG(USBD_INTSTS_EP5);
    if (g_u8HIDSetFlag)
    {
        uint8_t *ptr = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP5));

        g_u8HIDSetFlag = 0;
        g_u8HIDData[0] = 0x01;  // report ID
        g_u8HIDData[1] = 0x00;  // data
        // below are reserved bytes
        g_u8HIDData[2] = 0x00;
        g_u8HIDData[3] = 0x00;
        g_u8HIDData[4] = 0x00;
        g_u8HIDData[5] = 0x00;
        g_u8HIDData[6] = 0x00;
        g_u8HIDData[7] = 0x00;
        USBD_MemCopy(ptr, g_u8HIDData, 8);
        USBD_SET_PAYLOAD_LEN(EP5, EP5_MAX_PKT_SIZE);
    }

}

// Interrupt IN handler
void EP6_Handler(void)
{

}
// Interrupt OUT handler
void EP7_Handler(void)
{

}

void AUDIO_Init(void)
{
//    uint8_t i;
//    uint8_t *pu8;
//    uint8_t *pSerial = (uint8_t*)__TIME__;

    // Init setup packet buffer
    // Buffer range for setup packet -> [0 ~ 0x7]
    USBD->STBUFSEG = SETUP_BUF_BASE;

    //***************************************************
    // EP0 ==> control IN endpoint, address 0
    USBD_CONFIG_EP(EP0, USBD_CFG_CSTALL | USBD_CFG_EPMODE_IN | 0);
    // Buffer range for EP0
    USBD_SET_EP_BUF_ADDR(EP0, EP0_BUF_BASE);

    // EP1 ==> control OUT endpoint, address 0
    USBD_CONFIG_EP(EP1, USBD_CFG_CSTALL | USBD_CFG_EPMODE_OUT | 0);
    // Buffer range for EP1
    USBD_SET_EP_BUF_ADDR(EP1, EP1_BUF_BASE);

    //***************************************************
    // EP2 ==> Iso IN endpoint, address 2
    USBD_CONFIG_EP(EP2, USBD_CFG_EPMODE_IN | USBD_CFG_TYPE_ISO | EP2);
    // Buffer range for EP2
    USBD_SET_EP_BUF_ADDR(EP2, EP2_BUF_BASE);

    // EP3 ==> Iso IN endpoint, address 3
    USBD_CONFIG_EP(EP3, USBD_CFG_EPMODE_OUT | USBD_CFG_TYPE_ISO | EP3);
    // Buffer range for EP3
    USBD_SET_EP_BUF_ADDR(EP3, EP3_BUF_BASE);
	/* trigger to receive OUT data */
	USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);

    // EP4 ==> Iso IN endpoint, address 4
    USBD_CONFIG_EP(EP4, USBD_CFG_EPMODE_IN | USBD_CFG_TYPE_ISO | EP4);
    // Buffer range for EP4
    USBD_SET_EP_BUF_ADDR(EP4, EP4_BUF_BASE);
    // EP5 ==> Iso IN endpoint, address 5
    USBD_CONFIG_EP(EP5, USBD_CFG_EPMODE_IN | EP5);
    // Buffer range for EP5
    USBD_SET_EP_BUF_ADDR(EP5, EP5_BUF_BASE);

    //***************************************************
    /* EP6 ==> Iso Out endpoint, address 6 */
    USBD_CONFIG_EP(EP6, USBD_CFG_EPMODE_IN | EP6);
    /* Buffer range for EP6 */
    USBD_SET_EP_BUF_ADDR(EP6, EP6_BUF_BASE);
	USBD_SET_PAYLOAD_LEN(EP6, EP6_MAX_PKT_SIZE);

    /* EP7 ==> Interrupt IN endpoint number 7 */
    USBD_CONFIG_EP(EP7, USBD_CFG_EPMODE_OUT | EP7);
    /* Buffer range for EP7 */
    USBD_SET_EP_BUF_ADDR(EP7, EP7_BUF_BASE);
	/* trigger to receive OUT data */
    USBD_SET_PAYLOAD_LEN(EP7, EP7_MAX_PKT_SIZE);
#if 0
    /*****************************************************/
    /* EP8 ==> Interrupt IN endpoint, address 8 */
    USBD_CONFIG_EP(EP8, USBD_CFG_EPMODE_IN | 0x8);
    /* Buffer range for EP8 */
    USBD_SET_EP_BUF_ADDR(EP8, EP8_BUF_BASE);

    /* EP9 ==> Interrupt OUT endpoint, address 9 */
    USBD_CONFIG_EP(EP9, USBD_CFG_EPMODE_OUT | 0x9);
    /* Buffer range for EP9 */
    USBD_SET_EP_BUF_ADDR(EP9, EP9_BUF_BASE);
    /* trigger to receive OUT data */
    USBD_SET_PAYLOAD_LEN(EP9, EP9_MAX_PKT_SIZE);
#endif
    /*
       Generate Mass-Storage Device serial number
       To compliant USB-IF MSC test, we must enable serial string descriptor.
       However, window may fail to recognize the devices if PID/VID and serial number are all the same
       when plug them to Windows at the sample time.
       Therefore, we must generate different serial number for each device to avoid conflict
       when plug more then 2 MassStorage devices to Windows at the same time.

       NOTE: We use compiler predefine macro "__TIME__" to generate different number for serial
       at each build but each device here for a demo.
       User must change it to make sure all serial number is different between each device.
     */
    
//       pu8 = (uint8_t *)gsInfo.gu8StringDesc[3];
//       for(i = 0; i < 8; i++)
//		   pu8[pu8[0] - 16 + i * 2] = pSerial[i];
    
}

void AUDIO_ClassRequest(void)
{
    uint8_t buf[8];
    uint32_t u32Temp;

    USBD_GetSetupPacket(buf);

    if((buf[0] & 0x7f) == 0x22)     //bmRequestType= class, endpoint
    {
        if(buf[0] & 0x80)       // Device to host
        {
            switch(buf[1])
            {
                case UAC_GET_CUR:
                {
                    if(  buf[3]==SAMPLING_FREQ_CONTROL && (buf[4]==EP3))
                    {
                        //CX  g_u8RecEn = 0;
                        u32Temp = g_usbd_PlaySampleRate;
                        M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 0) = (uint8_t)u32Temp;
                        M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = (uint8_t)(u32Temp >> 8);
                        M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 2) = (uint8_t)(u32Temp >> 16);

                        /* Data stage */
                        USBD_SET_DATA1(EP0);
                        USBD_SET_PAYLOAD_LEN(EP0, 3);
                        printf("Get_CUR: %08lX\n",g_usbd_PlaySampleRate);
                    }
                    else
                    {
                        {
                            uint8_t i;
                            puts("error 1: ");
                            for(i=0; i<8; i++)
                                printf(" %2x", buf[i]);

                            printf("\n");
                        }
                        /* error endpoint or un identified Control*/
                        /* Setup error, stall the device */
                        USBD_SetStall(0);
                        printf("Get_CUR USBD_SetStall(0): %08lX\n",g_usbd_PlaySampleRate);
                    }

                    // Trigger next Control Out DATA1 Transaction.
                    /* Status stage */
                    USBD_PrepareCtrlOut(0, 0);
                    break;
                }

                default:
                {
                    /* Setup error, stall the device */
                    /*unidentify CONTROL*/
                    USBD_SetStall(0);
                }

            }

        }
        else        // Host to device
        {
            switch(buf[1])
            {
                case UAC_SET_CUR:
                {

                    if(buf[3]==SAMPLING_FREQ_CONTROL && (buf[4]==EP3))
                    {
                        USBD_PrepareCtrlOut((uint8_t *)&g_usbd_PlaySampleRate, buf[6]);

                        /* Status stage */
                        USBD_SET_DATA1(EP0);
                        USBD_SET_PAYLOAD_LEN(EP0, 0);


                    }
                    else
                    {
                        {
                            uint8_t i;
                            puts("error 2: ");
                            for(i=0; i<8; i++)
                                printf(" %2x", buf[i]);

                            printf("\n");
                        }
                        /* Setup error, stall the device */
                        /* Unidentify CONTROL*/
                        /* STALL control pipe */
                        USBD_SetStall(0);
                    }

                    break;
                }
                default:
                {
                    /*unimplement CONTROL or wrong endpoint number*/
                    /* Setup error, stall the device */
                    USBD_SetStall(0);
                }
            }
        }

    }
    else        /*Feature unit control*/
    {
        if (buf[0] & 0x80)    // Device to host
        {

            switch(buf[1])
            {
                case UAC_GET_CUR:
                {
                    switch(buf[3])
                    {
                        case MUTE_CONTROL:
                        {
                            if(REC_FEATURE_UNITID == buf[5])
                            {
                                u32Temp = g_usbd_RecMute;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                            }
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {
                                u32Temp = g_usbd_PlayMute;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                            }

                            /* Data stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 1);
                            break;
                        }
                        case VOLUME_CONTROL:
                        {

                            if(REC_FEATURE_UNITID == buf[5])
                            {
                                /* Left or right channel */
                                if(buf[2] == 1)
                                {
                                    u32Temp = g_usbd_RecVolumeL;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                    u32Temp = g_usbd_RecVolumeL >> 8;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                                }
                                else
                                {
                                    u32Temp = g_usbd_RecVolumeR;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                    u32Temp = g_usbd_RecVolumeR >> 8;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                                }

                            }
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {
                                /* Left or right channel */
                                if(buf[2] == 1)
                                {
                                    u32Temp = g_usbd_PlayVolumeL;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                    u32Temp = g_usbd_PlayVolumeL >> 8;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                                }
                                else
                                {
                                    u32Temp = g_usbd_PlayVolumeR;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                    u32Temp = g_usbd_PlayVolumeR >> 8;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                                }
                            }

                            /* Data stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 2);
                            break;
                        }
                        default:
                        {
                            /* Setup error, stall the device */
                            USBD_SetStall(0);
                        }
                    }

                    // Trigger next Control Out DATA1 Transaction.
                    /* Status stage */
                    USBD_PrepareCtrlOut(0, 0);
                    break;
                }

                case UAC_GET_MIN:
                {
                    switch(buf[3])
                    {
                        case VOLUME_CONTROL:
                        {
                            if(REC_FEATURE_UNITID == buf[5])
                            {
                                u32Temp = g_usbd_RecMinVolume;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                u32Temp = g_usbd_RecMinVolume >> 8;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                            }
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {
                                u32Temp = g_usbd_PlayMinVolume;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                u32Temp = g_usbd_PlayMinVolume >> 8;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                            }
                            /* Data stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 2);
                            break;
                        }
                        default:
                            /* STALL control pipe */
                            USBD_SetStall(0);
                    }
                    // Trigger next Control Out DATA1 Transaction.
                    /* Status stage */
                    USBD_PrepareCtrlOut(0, 0);
                    break;
                }

                case UAC_GET_MAX:
                {
                    switch(buf[3])
                    {
                        case VOLUME_CONTROL:
                        {
                            if(REC_FEATURE_UNITID == buf[5])
                            {
                                u32Temp = g_usbd_RecMaxVolume;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                u32Temp = g_usbd_RecMaxVolume >> 8;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                            }
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {
                                u32Temp = g_usbd_PlayMaxVolume;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                u32Temp = g_usbd_PlayMaxVolume >> 8;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                            }
                            /* Data stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 2);
                            break;
                        }
                        default:
                            /* STALL control pipe */
                            USBD_SetStall(0);
                    }

                    // Trigger next Control Out DATA1 Transaction.
                    /* Status stage */
                    USBD_PrepareCtrlOut(0, 0);
                    break;
                }

                case UAC_GET_RES:
                {
                    switch(buf[3])
                    {
                        case VOLUME_CONTROL:
                        {
                            if(REC_FEATURE_UNITID == buf[5])
                            {
                                u32Temp = g_usbd_RecResVolume;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                u32Temp = g_usbd_RecResVolume >> 8;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                            }
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {
                                u32Temp = g_usbd_PlayResVolume;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                u32Temp = g_usbd_PlayResVolume >> 8;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) =u32Temp;
                            }
                            /* Data stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 2);
                            break;
                        }
                        default:
                            /* STALL control pipe */
                            USBD_SetStall(0);
                    }
                    // Trigger next Control Out DATA1 Transaction.
                    /* Status stage */
                    USBD_PrepareCtrlOut(0, 0);
                    break;
                }


                default:
                {
                    /* Setup error, stall the device */
                    USBD_SetStall(0);
                }
            }
        }
        else
        {
            // Host to device
            switch(buf[1])
            {
                case UAC_SET_CUR:
                {
                    switch(buf[3])
                    {
                        case MUTE_CONTROL:
                            if(REC_FEATURE_UNITID == buf[5])
                                USBD_PrepareCtrlOut((uint8_t *)&g_usbd_RecMute, buf[6]);
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {
                                USBD_PrepareCtrlOut((uint8_t *)&g_usbd_PlayMute, buf[6]);
                            }
                            /* Status stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 0);
                            break;

                        case VOLUME_CONTROL:
                            if(REC_FEATURE_UNITID == buf[5])
                            {
                                if(buf[2] == 1)
                                {
                                    /* Prepare the buffer for new record volume of left channel */
                                    USBD_PrepareCtrlOut((uint8_t *)&g_usbd_RecVolumeL, buf[6]);
                                }
                                else
                                {
                                    /* Prepare the buffer for new record volume of right channel */
                                    USBD_PrepareCtrlOut((uint8_t *)&g_usbd_RecVolumeR, buf[6]);
                                }
                            }
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {

                                if(buf[2] == 1)
                                {
                                    /* Prepare the buffer for new play volume of left channel */
                                    USBD_PrepareCtrlOut((uint8_t *)&g_usbd_PlayVolumeL, buf[6]);
                                }
                                else
                                {
                                    /* Prepare the buffer for new play volume of right channel */
                                    USBD_PrepareCtrlOut((uint8_t *)&g_usbd_PlayVolumeR, buf[6]);
                                }
                            }

                            /* Status stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 0);
                            break;

                        default:
                            /* STALL control pipe */
                            USBD_SetStall(0);
                            break;
                    }
                    break;
                }
                // HID
                case HID_SET_REPORT:
                {
                    if(buf[3] == 2)
                    {
                        /* Request Type = Output */
                        USBD_SET_DATA1(EP1);
                        USBD_SET_PAYLOAD_LEN(EP1, buf[6]);

                        /* Status stage */
                        USBD_PrepareCtrlIn(0, 0);
                    }
                    break;
                }
                case HID_SET_IDLE:
                {
                    /* Status stage */
                    USBD_SET_DATA1(EP0);
                    USBD_SET_PAYLOAD_LEN(EP0, 0);
                    break;
                }
                case HID_SET_PROTOCOL:

                default:
                {
                    /* Setup error, stall the device */
                    USBD_SetStall(0);
                    break;
                }
            }
        }
    }
}


/**
 * @brief       Set Interface standard request
 *
 * @param[in]   u32AltInterface Interface
 *
 * @return      None
 *
 * @details     This function is used to set UAC Class relative setting
 */
void AUDIO_SetInterface(void)
{
    uint8_t buf[8];
    uint8_t u32AltInterface;
    uint8_t g_usbd_UsbInterface;

    USBD_GetSetupPacket(buf);
    u32AltInterface = buf[2];
    g_usbd_UsbInterface = buf[4];
    //OS_Printf("AUDIO_SetInterface %d %d\n",g_usbd_UsbInterface,u32AltInterface);
    if ( g_usbd_UsbInterface == 1 )     //Recorder
    {
        if (u32AltInterface == 1)
        {
            USBD_SET_DATA1(EP2);
            USBD_SET_PAYLOAD_LEN(EP2, 0);
            if (g_u16UAC_Buff_WriteIndex >= 640)
            {
                g_u16UAC_Buff_ReadIndex = g_u16UAC_Buff_WriteIndex - 640;
            }
            else
            {
                g_u16UAC_Buff_ReadIndex = AMIC_RING_BUFFER_LEN_2CH-(640-g_u16UAC_Buff_WriteIndex);
            }
            g_u8USB_IN_Start = 1;
        }
        else
        {
            USBD_SET_DATA1(EP2);
            USBD_SET_PAYLOAD_LEN(EP2, 0);
            g_u8USB_IN_Start = 0;
        }
    }
	else if ( g_usbd_UsbInterface == 2 )
    {
        /* Audio Iso OUT interface */
        if (u32AltInterface == 1)
        {
            g_u8AudioSpeakerState = UAC_START_AUDIO_SPEAK;
            g_u32SpeakerByteCount = 0;
            g_u32SpeakerByteCountRec = 0;
        }
        else
        {

            g_u8AudioSpeakerState = UAC_STOP_AUDIO_SPEAK;
            g_u32SpeakerByteCount = 0;
            g_u32SpeakerByteCountRec = 0;
			//
			g_u16PlayBackReadPtr = 0;
            g_u16PlayBackWritePtr = 0;
        }
    }
}
#if 1
// Volume control
volatile uint8_t g_u8PlayVolumeIndexL = 0;
volatile uint8_t g_u8PlayVolumeIndexR = 0;
volatile uint8_t g_u8RecVolumeIndexR = 0;
volatile int16_t g_usbd_CurPlayVolumeL;
volatile int16_t g_usbd_CurPlayVolumeR;
volatile int16_t g_usbd_CurRecVolumeR;

const int16_t VolumeHex[] =
{
    0x0000, 0xFFD9, 0xFFB2, 0xFF8A, 0xFF62, 0xFF39, 0xFF10, 0xFEE7, 0xFEBD, 0xFE92,
    0xFE67, 0xFE3C, 0xFE10, 0xFDE3, 0xFDB7, 0xFD89, 0xFD5B, 0xFD2D, 0xFD00, 0xFCFE,
    0xFCCE, 0xFC9E, 0xFC6D, 0xFC3B, 0xFC09, 0xFBD6, 0xFBA3, 0xFB6F, 0xFB3A, 0xFB04,
    0xFACE, 0xFA97, 0xFA5F, 0xFA26, 0xF9ED, 0xF9B3, 0xF977, 0xF93B, 0xF900, 0xF8FE,
    0xF8C0, 0xF881, 0xF841, 0xF7FF, 0xF7BD, 0xF779, 0xF735, 0xF6EF, 0xF6A8, 0xF65F,
    0xF615, 0xF5CA, 0xF57D, 0xF52F, 0xF500, 0xF4DF, 0xF48D, 0xF43A, 0xF3E4, 0xF38D,
    0xF334, 0xF2D9, 0xF27B, 0xF21C, 0xF1B9, 0xF155, 0xF100, 0xF0ED, 0xF083, 0xF016,
    0xEFA6, 0xEF32, 0xEEBB, 0xEE40, 0xEDC1, 0xED3E, 0xED00, 0xECB6, 0xEC29, 0xEB97,
    0xEAFF, 0xEA61, 0xE9BD, 0xE911, 0xE900, 0xE85D, 0xE7A1, 0xE6DB, 0xE60A, 0xE52D,
    0xE500, 0xE444, 0xE34B, 0xE241, 0xE124, 0xE100, 0xDFF0, 0xDEA2, 0xDD34, 0xDD00,
    0xDBA0, 0xD9DE, 0xD900, 0xD7E1, 0xD596, 0xD500, 0xD2E3, 0xD100, 0xCF9B, 0xCD00,
    0xCB6A, 0xC900, 0xC59B, 0xC500, 0xC100, 0xBD00, 0xBC16, 0xB900, 0xB500, 0xB100,
    0xAD00, 0xA900, 0xA500, 0xA100, 0x9D00
};


const float VolumeMultiplier[] =
{
    1.0000, 0.9826, 0.9655, 0.9483, 0.9314, 0.9144, 0.8977, 0.8813, 0.8648, 0.8482,
    0.8320, 0.8161, 0.8001, 0.7840, 0.7687, 0.7529, 0.7375, 0.7224, 0.7079, 0.7073,
    0.6922, 0.6774, 0.6627, 0.6479, 0.6335, 0.6192, 0.6051, 0.5911, 0.5772, 0.5634,
    0.5498, 0.5364, 0.5231, 0.5098, 0.4969, 0.4841, 0.4712, 0.4587, 0.4467, 0.4463,
    0.4340, 0.4219, 0.4099, 0.3979, 0.3863, 0.3747, 0.3634, 0.3521, 0.3410, 0.3300,
    0.3192, 0.3086, 0.2981, 0.2879, 0.2818, 0.2777, 0.2676, 0.2578, 0.2480, 0.2385,
    0.2292, 0.2200, 0.2109, 0.2021, 0.1933, 0.1848, 0.1778, 0.1763, 0.1681, 0.1601,
    0.1522, 0.1445, 0.1369, 0.1296, 0.1224, 0.1154, 0.1122, 0.1085, 0.1019, 0.0954,
    0.0891, 0.0830, 0.0771, 0.0713, 0.0708, 0.0658, 0.0605, 0.0553, 0.0503, 0.0456,
    0.0447, 0.0410, 0.0367, 0.0326, 0.0286, 0.0282, 0.0249, 0.0215, 0.0182, 0.0178,
    0.0152, 0.0124, 0.0112, 0.0099, 0.0076, 0.0071, 0.0056, 0.0045, 0.0038, 0.0028,
    0.0023, 0.0018, 0.0012, 0.0011, 0.0007, 0.0004, 0.0004, 0.0003, 0.0002, 0.0001,
    0.0001, 0.0000, 0.0000, 0.0000, 0.0000
};

void USBPlayVolumeControl(uint32_t *u32Data)
{
    uint16_t i;
    int16_t i16Left, i16Right;
    float f32Left, f32Right;

    i16Left = (int16_t)(*u32Data & 0x0000FFFF);
    i16Right = (int16_t)((*u32Data >> 16) & 0x0000FFFF);

    f32Left = (float)i16Left;
    f32Right = (float)i16Right;

    // Find left channel new volume
    if (g_usbd_CurPlayVolumeL != g_usbd_PlayVolumeL)
    {
        g_usbd_CurPlayVolumeL = g_usbd_PlayVolumeL;

        if (g_usbd_CurPlayVolumeL == 0)
        {
            g_u8PlayVolumeIndexL = 0;
        }
        else
        {
            for (i=0; i<=124; i++)
            {
                if ((VolumeHex[i] > g_usbd_CurPlayVolumeL) && (VolumeHex[i+1] <= g_usbd_CurPlayVolumeL))
                {
                    g_u8PlayVolumeIndexL = i+1;
                    break;
                }
            }
        }
    }

    // Find right channel new volume
    if (g_usbd_CurPlayVolumeR != g_usbd_PlayVolumeR)
    {
        g_usbd_CurPlayVolumeR = g_usbd_PlayVolumeR;

        if (g_usbd_CurPlayVolumeR == g_usbd_CurPlayVolumeL)
        {
            g_u8PlayVolumeIndexR = g_u8PlayVolumeIndexL;
        }
        else
        {
            if (g_usbd_CurPlayVolumeR == 0)
            {
                g_u8PlayVolumeIndexR = 0;
            }
            else
            {
                for (i=0; i<=124; i++)
                {
                    if ((VolumeHex[i] > g_usbd_CurPlayVolumeR) && (VolumeHex[i+1] <= g_usbd_CurPlayVolumeR))
                    {
                        g_u8PlayVolumeIndexR = i+1;
                        break;
                    }
                }
            }
        }
    }

    // Adjust volume
    f32Left = f32Left*VolumeMultiplier[g_u8PlayVolumeIndexL];
    f32Right = f32Right*VolumeMultiplier[g_u8PlayVolumeIndexR]*0.5;//miao add *0.5

    i16Left = (int16_t)f32Left;
    i16Right = (int16_t)f32Right;

    *u32Data = (((uint16_t)i16Right << 16) | (uint16_t)i16Left);
}


void USBRecVolumeControl(uint32_t *u32Data)
{
    uint16_t i;
    int16_t i16Left, i16Right;
    float f32Left, f32Right;

    i16Left = (int16_t)(*u32Data & 0x0000FFFF);
    i16Right = (int16_t)((*u32Data >> 16) & 0x0000FFFF);

    f32Left = (float)i16Left;
    f32Right = (float)i16Right;

    // Find left channel new volume
    if (g_usbd_CurRecVolumeR != g_usbd_RecVolumeR)
    {
        g_usbd_CurRecVolumeR = g_usbd_RecVolumeR;

        if (g_usbd_CurRecVolumeR == 0)
        {
            g_u8RecVolumeIndexR = 0;
        }
        else
        {
            for (i=0; i<=124; i++)
            {
                if ((VolumeHex[i] > g_usbd_CurRecVolumeR) && (VolumeHex[i+1] <= g_usbd_CurRecVolumeR))
                {
                    g_u8RecVolumeIndexR = i+1;
                    break;
                }
            }
        }
    }
    f32Left = f32Left * VolumeMultiplier[g_u8RecVolumeIndexR];
    f32Right = f32Right * VolumeMultiplier[g_u8RecVolumeIndexR];

    i16Left = (int16_t)f32Left;
    i16Right = (int16_t)f32Right;

    *u32Data = (((uint16_t)i16Right << 16) | (uint16_t)i16Left);
}
#endif

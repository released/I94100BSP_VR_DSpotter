/********************************************************************************
*     DMA.c
*********************************************************************************/
#include "c_stdfx.h"
#include "DMA.h"
#include "appAudioLine.h"
#include "vr.h"
#include "PlayNuOne.h"


void NVIC_DMA_Init(void)
{
    // Enable PDMA clock.
    CLK_EnableModuleClock(PDMA_MODULE);
    // Reset PDMA module
    SYS_ResetModule(PDMA_RST);
    // Enable PDMA's NVIC
    NVIC_EnableIRQ(PDMA_IRQn);
}

uint16_t triangle[] = 
{
	0x0,    0x0800, 0x1000, 0x1800, 
	0x2001, 0x2801, 0x3001, 0x3801,
	0x4001, 0x3801, 0x3001, 0x2801,
	0x2001, 0x1800, 0x1000, 0x0800,
	0x0,    0xf800, 0xf000, 0xe800,
	0xdfff, 0xd7ff, 0xcfff, 0xc7ff,
	0xbfff, 0xc7ff, 0xcfff, 0xd7ff,
	0xdfff, 0xe800, 0xf000, 0xf800,
};
uint16_t j=0;

extern DSCT_T SPI2_DMA_RXDESC[2];
void PDMA_IRQHandler(void)
{
    int i = 0;
    u32 u32Status;
    u32 u32PDMA_TDFlag;

    // Get interrupt status.
    //OS_Printf("PDMA_IRQHandler In\n");
    u32Status = PDMA_GET_INT_STATUS();
    // TDF-PDMA Channel Transfer Done Flag
    u32PDMA_TDFlag = PDMA_GET_TD_STS();
    PDMA_CLR_TD_FLAG(u32PDMA_TDFlag);

    // PDMA Read/Write Target Abort Interrupt Flag
    if (u32Status & PDMA_STATUS_ABTIF)
    {
        // if (PDMA_GET_ABORT_STS() & PDMA_CH2_MASK)
        {
            PDMA_CLR_ABORT_FLAG(PDMA_GET_ABORT_STS());
        }
    }
    else if (u32Status & PDMA_STATUS_TDIF)
    {
        if ( u32PDMA_TDFlag & (1 << SPK_PDMA_CH))
        {
            PDMA_CLR_TD_FLAG((1<<SPK_PDMA_CH));
            //	USBDMA_TXHandle();
            g_sApp.Mic_NuOne.sNuOneApp.bBufferEmpty=TRUE;
            PDMA1CallBackCount++;
        }


        if ( u32PDMA_TDFlag & (1 << MIC_PDMA_CH) )
        {
            PDMA_CLR_TD_FLAG((1<<MIC_PDMA_CH));

            if (g_usbd_UsbAudioState == UAC_START_AUDIO_RECORD)
            {
                for ( i= 0; i < (AMIC2PDMA_BUFF_LEN_2CH/2); i+=1)
                {
                    g_au32UAC_RingBuff[g_u16UAC_Buff_WriteIndex++] = g_u32MICBuffer[0][i];
                }
                g_usbd_UsbAudioState = UAC_READY_AUDIO_RECORD;
            }
            else if (g_usbd_UsbAudioState == UAC_READY_AUDIO_RECORD)
            {
                for ( i= 0 ; i < (AMIC2PDMA_BUFF_LEN_2CH/2) ; i++)
                {
                    g_au32UAC_RingBuff[g_u16UAC_Buff_WriteIndex++] = g_u32MICBuffer[1][i];			
                }
                g_usbd_UsbAudioState = UAC_BUSY_AUDIO_RECORD;
                g_u8amic_pdma_bufidx = 0;
                g_u8I2S0RxBuffIndex = 0;
                g_u8I2S0TxBuffIndex = 0;
                memset(&Flow_Ctrl,0,sizeof(USBDMA_Ctrl_t));
            }
            else if (g_usbd_UsbAudioState == UAC_BUSY_AUDIO_RECORD)
            {
                USBDMA_RXHandle();
            }
        }
		
        if ( u32PDMA_TDFlag & (1 << DMIC_PDMA_CH) )
        {
            PDMA_CLR_TD_FLAG((1<<DMIC_PDMA_CH));
			
			uint16_t u16L, u16R;

            if (g_usbd_UsbAudioState == UAC_START_AUDIO_RECORD)
            {
                for ( i= 0; i < (AMIC2PDMA_BUFF_LEN_2CH/2); i++)
                {
					u16L = (uint16_t)(DMIC_volume*(g_u32MICBuffer[0][2*i] >>8));
					u16R = (uint16_t)(DMIC_volume*(g_u32MICBuffer[0][2*i+1] >>8));
                    g_au32UAC_RingBuff[g_u16UAC_Buff_WriteIndex++] = (uint32_t)((u16L<<16)+u16R);
                }
                g_usbd_UsbAudioState = UAC_READY_AUDIO_RECORD;
            }
            else if (g_usbd_UsbAudioState == UAC_READY_AUDIO_RECORD)
            {
                for ( i= 0 ; i < (AMIC2PDMA_BUFF_LEN_2CH/2) ; i++)
                {
					u16L = (uint16_t)(DMIC_volume*(g_u32MICBuffer[1][2*i] >>8));
					u16R = (uint16_t)(DMIC_volume*(g_u32MICBuffer[1][2*i+1] >>8));
                    g_au32UAC_RingBuff[g_u16UAC_Buff_WriteIndex++] = (uint32_t)((u16L<<16)+u16R);
                }
                g_usbd_UsbAudioState = UAC_BUSY_AUDIO_RECORD;
                g_u8amic_pdma_bufidx = 0;
                g_u8I2S0RxBuffIndex = 0;
                g_u8I2S0TxBuffIndex = 0;
                memset(&Flow_Ctrl,0,sizeof(USBDMA_Ctrl_t));
            }
            else if (g_usbd_UsbAudioState == UAC_BUSY_AUDIO_RECORD)
            {
                USBDMA_RXHandle();
            }
        }		
    }

    //OS_Printf("PDMA_IRQHandler Out\n");
}

//----------------------------------------------------------------------------
//  PDMA Initial function
//----------------------------------------------------------------------------
void PDMA_Init(PDMA_CONFIG_T *pdma_ctl)
{
    if (pdma_ctl->rec_en)
    {
#if ((MIC_PDMA_16BIT ==0)||(MIC_PDMA_16BIT ==1))		
        // Open PDMA channel
        PDMA_Open((1 << MIC_PDMA_CH));
        /* Rx description */
        DMA_RXDESC[0].CTL = ((pdma_ctl->rec_buflen-1)<<PDMA_DSCT_CTL_TXCNT_Pos)|
                            PDMA_WIDTH_16		|
                            PDMA_SAR_FIX		|
                            PDMA_DAR_INC		|
                            PDMA_REQ_SINGLE	    |
                            PDMA_OP_SCATTER;
        DMA_RXDESC[0].SA = (uint32_t)&I2S0->RXFIFO;
        DMA_RXDESC[0].DA = (uint32_t)&g_u32MICBuffer[0][0];
        DMA_RXDESC[0].NEXT = (uint32_t)&DMA_RXDESC[1] - (PDMA->SCATBA);

        DMA_RXDESC[1].CTL = ((pdma_ctl->rec_buflen-1)<<PDMA_DSCT_CTL_TXCNT_Pos)|
                            PDMA_WIDTH_16		|
                            PDMA_SAR_FIX		|
                            PDMA_DAR_INC		|
                            PDMA_REQ_SINGLE	    |
                            PDMA_OP_SCATTER;
        DMA_RXDESC[1].SA = (uint32_t)&I2S0->RXFIFO;
        DMA_RXDESC[1].DA = (uint32_t)&g_u32MICBuffer[1][0];
        DMA_RXDESC[1].NEXT = (uint32_t)&DMA_RXDESC[0] - (PDMA->SCATBA);   //link to first description

        // Set TransMode
        PDMA_SetTransferMode(MIC_PDMA_CH, PDMA_I2S0_RX, TRUE, (uint32_t)&DMA_RXDESC[0]);
		  PDMA_EnableInt(MIC_PDMA_CH,PDMA_INT_TRANS_DONE);
#elif  (MIC_PDMA_16BIT ==2) //DMIC
		PDMA_Open((1 << DMIC_PDMA_CH));
        /* Rx description */
        DMA_RXDESC[0].CTL = ((pdma_ctl->rec_buflen-1)<<PDMA_DSCT_CTL_TXCNT_Pos)|
                            PDMA_WIDTH_32		|
                            PDMA_SAR_FIX		|
                            PDMA_DAR_INC		|
                            PDMA_REQ_SINGLE 	|
                            PDMA_OP_SCATTER;
        DMA_RXDESC[0].SA = (uint32_t)&DMIC->FIFO;
        DMA_RXDESC[0].DA = (uint32_t)&g_u32MICBuffer[0][0];
        DMA_RXDESC[0].NEXT = (uint32_t)&DMA_RXDESC[1] - (PDMA->SCATBA);

        DMA_RXDESC[1].CTL = ((pdma_ctl->rec_buflen-1)<<PDMA_DSCT_CTL_TXCNT_Pos)|
                            PDMA_WIDTH_32	  |
                            PDMA_SAR_FIX		|
                            PDMA_DAR_INC		|
                            PDMA_REQ_SINGLE 	|
                            PDMA_OP_SCATTER;
        DMA_RXDESC[1].SA = (uint32_t)&DMIC->FIFO;
        DMA_RXDESC[1].DA = (uint32_t)&g_u32MICBuffer[1][0];
        DMA_RXDESC[1].NEXT = (uint32_t)&DMA_RXDESC[0] - (PDMA->SCATBA);   //link to first description

        // Set TransMode
        PDMA_SetTransferMode(DMIC_PDMA_CH, PDMA_DMIC_RX, TRUE, (uint32_t)&DMA_RXDESC[0]);
		// Enable interrupt
		PDMA_EnableInt(DMIC_PDMA_CH,PDMA_INT_TRANS_DONE);
#endif        
    }

///////////////////////////////////////////////////////////////////////////////////////

    if (pdma_ctl->play_en)
    {
        PDMA_Open((1 << SPK_PDMA_CH));        // Open PDMA channel
//		PDMA->CHCTL |= (1 << SPK_PDMA_CH);

#if ((SPK_PDMA_16BIT==0)||(SPK_PDMA_16BIT==1))
        // Tx description.
        DMA_TXDESC[0].CTL = ((NUONEEX_DECODE_SAMPLE_PER_FRAME*2-1)<<PDMA_DSCT_CTL_TXCNT_Pos)	|		// transfer byte count
                            PDMA_WIDTH_16	|
                            PDMA_SAR_INC		|		// Source Address Increment
                            PDMA_DAR_FIX		|		// Destination Address fixed
                            PDMA_REQ_SINGLE	|		// Transfer Type = single
                            PDMA_OP_SCATTER;

        DMA_TXDESC[0].SA = (uint32_t)&g_u32SPKBuffer[0][0];
        DMA_TXDESC[0].DA = (uint32_t)&I2S0->TXFIFO;
        DMA_TXDESC[0].NEXT = ((uint32_t)&DMA_TXDESC[1] - (PDMA->SCATBA));

        DMA_TXDESC[1].CTL = ((NUONEEX_DECODE_SAMPLE_PER_FRAME*2-1)<<PDMA_DSCT_CTL_TXCNT_Pos)	|		// transfer byte count
                            PDMA_WIDTH_16	|
                            PDMA_SAR_INC		|		// Source Address Increment
                            PDMA_DAR_FIX		|		// Destination Address fixed
                            PDMA_REQ_SINGLE	|		// Transfer Type = single
                            PDMA_OP_SCATTER;

        DMA_TXDESC[1].SA = (uint32_t)&g_u32SPKBuffer[1][0];
        DMA_TXDESC[1].DA = (uint32_t)&I2S0->TXFIFO;
        DMA_TXDESC[1].NEXT = ((uint32_t)&DMA_TXDESC[0] - (PDMA->SCATBA));

        // Request source is memory to memory
        PDMA_SetTransferMode(SPK_PDMA_CH, PDMA_I2S0_TX, TRUE, (uint32_t)&DMA_TXDESC[0]);


#elif (SPK_PDMA_16BIT==2)
        DMA_TXDESC[0].CTL = ((NUONEEX_DECODE_SAMPLE_PER_FRAME*2-1)<<PDMA_DSCT_CTL_TXCNT_Pos)	|		// transfer byte count
                            PDMA_WIDTH_16	|
                            PDMA_SAR_INC		|		// Source Address Increment
                            PDMA_DAR_FIX		|		// Destination Address fixed
                            PDMA_REQ_SINGLE	|		// Transfer Type = single
                            PDMA_OP_SCATTER;	    //

        DMA_TXDESC[0].SA = (uint32_t)&g_u32SPKBuffer[0][0];
        DMA_TXDESC[0].DA = (uint32_t)&DPWM->FIFO;
        DMA_TXDESC[0].NEXT = ((uint32_t)&DMA_TXDESC[1] - (PDMA->SCATBA));

        DMA_TXDESC[1].CTL = ((NUONEEX_DECODE_SAMPLE_PER_FRAME*2-1)<<PDMA_DSCT_CTL_TXCNT_Pos)	|
                            PDMA_WIDTH_16	|
                            PDMA_SAR_INC		|		// Source Address Increment
                            PDMA_DAR_FIX		|		// Destination Address fixed
                            PDMA_REQ_SINGLE	|		// Transfer Type = single
                            PDMA_OP_SCATTER;	      //

        DMA_TXDESC[1].SA = (uint32_t)&g_u32SPKBuffer[1][0];
        DMA_TXDESC[1].DA = (uint32_t)&DPWM->FIFO;
        DMA_TXDESC[1].NEXT = ((uint32_t)&DMA_TXDESC[0] - (PDMA->SCATBA));

        // Request source is memory to memory
        PDMA_SetTransferMode(SPK_PDMA_CH, PDMA_DPWM_TX, TRUE, (uint32_t)&DMA_TXDESC[0]);
        //PDMA_EnableInt(SPK_PDMA_CH,PDMA_INT_TRANS_DONE);
#endif
        // Enable PDMA channel 1 interrupt
        PDMA_EnableInt(SPK_PDMA_CH, PDMA_INT_TRANS_DONE);
//		SPK_Stop();
    }


    // Enable PDMA's NVIC
    NVIC_EnableIRQ(PDMA_IRQn);
}

void reset_pdma_spk(uint8_t ch)
{
	PDMA->CHRST |= (1 << ch);
//	while(PDMA->CHRST | (1 << ch));
	PDMA_Open((1 << ch)); 
	PDMA_SetTransferMode(ch, PDMA_I2S0_TX, TRUE, (uint32_t)&DMA_TXDESC[0]);
	PDMA_EnableInt(ch, PDMA_INT_TRANS_DONE);
}

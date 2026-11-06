/**************************************************************************//**
 * @file     Periph.c
 * @version  V1.00
 * $Revision: 1 $
 * $Date: 16/06/14 9:36a $
 * @brief    USBD Emulation
             Transfer data from USB device to PC through USB HID interface.
 *           After USB enumeration completed, device always IN data to USB host.
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include <string.h> 
#include "I94100.h"
#include "usbd_audio.h"
#include "audioclass.h"
#include "peripheral.h"
/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables and constants                                                                   */
/*---------------------------------------------------------------------------------------------------------*/

// UAC(USBD) = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
#define USBD_UAC_PIN_MASK (SYS_GPB_MFPH_PB13MFP_Msk|SYS_GPB_MFPH_PB14MFP_Msk|SYS_GPB_MFPH_PB15MFP_Msk)
#define USBD_UAC_PIN      (SYS_GPB_MFPH_PB13MFP_USBD_DN|SYS_GPB_MFPH_PB14MFP_USBD_DP|SYS_GPB_MFPH_PB15MFP_USBD_VBUS)

void UAC_Init(void)
{
	// gpio multi-function configuration.
	SYS->GPB_MFPH = (SYS->GPB_MFPH&(~USBD_UAC_PIN_MASK))|USBD_UAC_PIN;
	// Set USBD clock divid
    //CLK->CLKSEL4 &= ~BIT24;          // 1=PLLOUT, 0=HIRC 
	CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL4_USBSEL_HIRC, CLK_CLKDIV0_USBD(1));  
	// Enable USBD module clock.
	CLK_EnableModuleClock(USBD_MODULE);
	// Initiate USBD hardware IP and input UAC request for hand-shake.
	USBD_Open(&gsInfo, AUDIO_ClassRequest, (SET_INTERFACE_REQ)AUDIO_SetInterface);	
	// Initiate UAC for endpoint configuration and input input buffer control for UAC controlling.
	
	USBD_SetConfigCallback((SET_CONFIG_CB) HIRC_AutoTrim_RefSof);
	
	AUDIO_Init();
	// Enable USB IRQ
	NVIC_EnableIRQ(USBD_IRQn);
}

void UAC_Start(void)
{
	USBD_Start();
}

/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/

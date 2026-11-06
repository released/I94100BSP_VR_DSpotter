/******************************************************************************
 * @file     HIRC_AutoTrim.c
 * @brief    I94100 series USBD driver Sample file
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

/*!<Includes */
#include "HIRC_AutoTrim.h"

extern volatile uint8_t g_usbd_Configured;

#define IRCTCTL_AUTOTRIM48M     (0x1<<0)
#define IRCTCTL_AUTOTRIM49M     (0x3<<0)
#define IRCTCTL_LOOPSEL_4       (0x0<<4)
#define IRCTCTL_LOOPSEL_8       (0x1<<4)
#define IRCTCTL_LOOPSEL_16      (0x2<<4)
#define IRCTCTL_LOOPSEL_32      (0x3<<4)
#define IRCTCTL_RETRYCNT_64		(0x0<<6)
#define IRCTCTL_RETRYCNT_128    (0x1<<6) 
#define IRCTCTL_RETRYCNT_256    (0x2<<6)
#define IRCTCTL_RETRYCNT_512    (0x3<<6)
#define IRCTCTL_CESTOPEN        (0x1<<8)    
#define IRCTCTL_REFCLK_SOF      (0x1<<10)   
    
/*
  HIRC auto-trim init 
*/
void HIRC_AutoTrim_RefSof_Init(void)
{	
    uint32_t CtrlValue; 

//    SYS->IRCTCTL = 0x00;
    CtrlValue |= IRCTCTL_LOOPSEL_4;	
    CtrlValue |= IRCTCTL_RETRYCNT_64; 
    CtrlValue |= IRCTCTL_CESTOPEN;
    CtrlValue |= IRCTCTL_REFCLK_SOF;		
                                                                
    // BIT[1:0] : HIRC auto trim enable/disable 
    CtrlValue |= IRCTCTL_AUTOTRIM48M;  
    
    CtrlValue &= 0x5F3;		    // Ignore reserved bits 
    
    SYS->IRCTIEN = (BIT1|BIT2);		// Enable clock error / trim fail interrupt 		
//	NVIC_EnableIRQ(IRC_IRQn);
                
//    printf("SYS->IRCTCTL = %x\n", CtrlValue);
    // Load Configuration
    SYS->IRCTCTL = CtrlValue;
}


void HIRC_AutoTrim_RefSof(void)
{
    uint32_t value;
    
 //   while(g_usbd_Configured == 0);		
    
//    HIRC_AutoTrim_RefSof_Init();	

//    do 
//    {
//      value = SYS->IRCTISTS;
//    } while ( (value & BIT0) != BIT0);
//        
//    SYS->IRCTISTS = value;
	
	SYS_EnableTrimHIRC(SYS_IRCTCTL_FREQSEL_48M);

	while(!SYS_IS_TRIMHIRC_DONE())
	{
	}
	
	SYS_CLEAR_TRIMHIRC_INT_FLAG(SYS_IRCTISTS_TRIMFAIL_INT_FLAG|SYS_IRCTISTS_CLKERROR_INT_FLAG);	
    
}


void IRC_IRQHandler(void)
{
	// Get Trim Failure Interrupt
    if(SYS_GET_TRIMHIRC_INT_FLAG(SYS_IRCTISTS_TRIMFAIL_INT_FLAG))
	{ 
        // Display HIRC trim status(trim fail) 
//        printf("HIRC Trim Failure Interrupt\r\n");
        // Clear Trim Failure Interrupt
		SYS_CLEAR_TRIMHIRC_INT_FLAG(SYS_IRCTISTS_TRIMFAIL_INT_FLAG);
    }
	// Get LXT Clock Error Interrupt
    if(SYS_GET_TRIMHIRC_INT_FLAG(SYS_IRCTISTS_CLKERROR_INT_FLAG)) 
	{ 
        // Display HIRC trim status(LXT clock error) 
//        printf("LXT Clock Error Interrupt\r\n");
        // Clear LXT Clock Error Interrupt 
        SYS_CLEAR_TRIMHIRC_INT_FLAG(SYS_IRCTISTS_CLKERROR_INT_FLAG);
    }

}

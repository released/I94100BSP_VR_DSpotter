/**************************************************************************//**
 * @file     pdma_config.h
 * @version  V1.00
 * @brief    I94100 series USB driver header file
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __PDMA_CONFIG_H__
#define __PDMA_CONFIG_H__

 #include "stdint.h" 

typedef struct pdma_config_t
{
	uint8_t  play_en;
	uint16_t play_buflen;
 	uint8_t  rec_en;
 	uint16_t rec_buflen;	
} PDMA_CONFIG_T;
extern PDMA_CONFIG_T PDMA_CONFIG; 

#define MIC_PDMA_CH   		(1)
#define SPK_PDMA_CH   		(2)
//#define I2S0_RX_PDMA_CH    	MIC_PDMA_CH
//#define I2S0_TX_PDMA_CH    	SPK_PDMA_CH


#define I2S2_TX_PDMA_CH    	(3)
#define I2S2_RX_PDMA_CH    	(4)

#define DMIC_PDMA_CH		(5)

#endif //__PDMA_CONFIG_H__

/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/


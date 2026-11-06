
#include "I94100.h"
#include "pdma_config.h"
#include "c_stdfx.h"
#include "pdma.h"
#include "hwh_spii2s.h"



#define REC_RATE  16000

PDMA_CONFIG_T 	SPI2_I2S_PDMA_COF ;
DSCT_T SPI2_DMA_TXDESC[2] __attribute__((at(0x20001f00)));
DSCT_T SPI2_DMA_RXDESC[2] __attribute__((at(0x20001f20)));



void SPI2_I2S_Init(void)
{
    CLK_EnableModuleClock(SPI2_MODULE);
    CLK_SetModuleClock(SPI2_MODULE, CLK_CLKSEL2_SPI1SEL_PCLK1, NULL);
    SYS_ResetModule(SPI2_RST);
	
	///// MOSI--PC5  MISO--PC6  BCLK--PC7   SS--PC8   MCLK--PC9
	SYS->GPC_MFPL = (SYS->GPC_MFPL & ~(SYS_GPC_MFPL_PC5MFP_Msk | SYS_GPC_MFPL_PC6MFP_Msk | SYS_GPC_MFPL_PC7MFP_Msk  )) | 
	(SYS_GPC_MFPL_PC5MFP_SPI2_MOSI | SYS_GPC_MFPL_PC6MFP_SPI2_MISO | SYS_GPC_MFPL_PC7MFP_SPI2_CLK );  
	SYS->GPC_MFPH = (SYS->GPC_MFPH & ~( SYS_GPC_MFPH_PC8MFP_Msk | SYS_GPC_MFPH_PC9MFP_Msk )) | 
	( SYS_GPC_MFPH_PC8MFP_SPI2_SS | SYS_GPC_MFPH_PC9MFP_SPI2_I2SMCLK);  
	
    SPI_I2SOpen(SPI2, SPI_I2SMASTER, REC_RATE, SPI_I2SDATABIT_16, SPI_I2SSTEREO, SPI_I2SFORMAT_I2S);
    SPI_I2SEnableMCLK(SPI2, 256*REC_RATE);
    // Clear Rx, Tx FIFO
    SPI_ClearRxFIFO(SPI2);
    SPI_ClearTxFIFO(SPI2);
		SPI_I2S_RST_TX_FIFO(SPI2);
		SPI_I2S_RST_RX_FIFO(SPI2);
    SPI_I2S_CLR_TX_FIFO(SPI2);
    SPI_I2S_CLR_RX_FIFO(SPI2);
		SPI_I2SEnableControl(SPI2);
//		SPI1_I2S_Stop();
}


void SPI2_I2S_Stop(void)
{
    SPI_I2S_DISABLE_TXDMA(SPI2);
    SPI_I2S_DISABLE_RXDMA(SPI2);
    SPI_I2S_CLR_TX_FIFO(SPI2);
    SPI_I2S_CLR_RX_FIFO(SPI2);
    SPI_I2S_DISABLE_TX(SPI2);
    SPI_I2S_DISABLE_RX(SPI2);
//    SPI2_I2S_Init();
}


void SPI2_I2S_Start(void)
{
    SPI_I2SEnableControl(SPI2);
    SPI_I2S_ENABLE_RX(SPI2);
		SPI_I2S_ENABLE_TX(SPI2);
    SPI_I2S_ENABLE_RXDMA(SPI2);
		SPI_I2S_ENABLE_TXDMA(SPI2);
}

 uint32_t SPI2_I2S_decode[4][Frame_Len]; //01TX，23RX

void SPI2_I2S_PDMA(void)   //循环双BUF 自动发送
{
	PDMA->CHRST |= (1 << I2S2_TX_PDMA_CH); //复位通道	
	PDMA_Open((1 << I2S2_TX_PDMA_CH));
	SPI2_I2S_PDMA_COF.play_buflen = Frame_Len;
	
		SPI2_DMA_TXDESC[0].CTL = ((SPI2_I2S_PDMA_COF.play_buflen -1)<<PDMA_DSCT_CTL_TXCNT_Pos)|
							PDMA_WIDTH_32		|
							PDMA_SAR_INC		|		// Source Address Increment
							PDMA_DAR_FIX		|		// Destination Address fixed
							PDMA_REQ_SINGLE	|
							PDMA_OP_SCATTER;
		SPI2_DMA_TXDESC[0].SA = (uint32_t)&SPI2_I2S_decode[2][0];
		SPI2_DMA_TXDESC[0].DA = (uint32_t)&SPI2->TX;
		SPI2_DMA_TXDESC[0].NEXT = (uint32_t)&SPI2_DMA_TXDESC[1] - (PDMA->SCATBA);

		SPI2_DMA_TXDESC[1].CTL = (( SPI2_I2S_PDMA_COF.play_buflen -1)<<PDMA_DSCT_CTL_TXCNT_Pos)|
							PDMA_WIDTH_32		|
							PDMA_SAR_INC		|		// Source Address Increment
							PDMA_DAR_FIX		|		// Destination Address fixed
							PDMA_REQ_SINGLE	|
							PDMA_OP_SCATTER;
		SPI2_DMA_TXDESC[1].SA = (uint32_t)&SPI2_I2S_decode[3][0];
		SPI2_DMA_TXDESC[1].DA = (uint32_t)&SPI2->TX;
		SPI2_DMA_TXDESC[1].NEXT = (uint32_t)&SPI2_DMA_TXDESC[0] - (PDMA->SCATBA);   //link to first description	
		// Set TransMode
		PDMA_SetTransferMode(I2S2_TX_PDMA_CH, PDMA_SPI2_TX, TRUE, (uint32_t)&SPI2_DMA_TXDESC[0]);    
	SPI_TRIGGER_TX_PDMA(SPI2);	
	
	PDMA->CHRST |= (1 << I2S2_RX_PDMA_CH); //复位通道	
	PDMA_Open((1 << I2S2_RX_PDMA_CH));	

		SPI2_DMA_RXDESC[0].CTL = ((SPI2_I2S_PDMA_COF.play_buflen -1)<<PDMA_DSCT_CTL_TXCNT_Pos)|
							PDMA_WIDTH_32		|
							PDMA_SAR_FIX		|		// Source Address Increment
							PDMA_DAR_INC		|		// Destination Address fixed
							PDMA_REQ_SINGLE	|
							PDMA_OP_SCATTER;
		SPI2_DMA_RXDESC[0].SA = (uint32_t)&SPI2->RX;
		SPI2_DMA_RXDESC[0].DA = (uint32_t)&SPI2_I2S_decode[0][0];
		SPI2_DMA_RXDESC[0].NEXT = (uint32_t)&SPI2_DMA_RXDESC[1] - (PDMA->SCATBA);

		SPI2_DMA_RXDESC[1].CTL = (( SPI2_I2S_PDMA_COF.play_buflen -1)<<PDMA_DSCT_CTL_TXCNT_Pos)|
							PDMA_WIDTH_32		|
							PDMA_SAR_FIX		|		// Source Address Increment
							PDMA_DAR_INC		|		// Destination Address fixed
							PDMA_REQ_SINGLE	|
							PDMA_OP_SCATTER;
		SPI2_DMA_RXDESC[1].SA = (uint32_t)&SPI2->RX;  
		SPI2_DMA_RXDESC[1].DA = (uint32_t)&SPI2_I2S_decode[1][0];
		SPI2_DMA_RXDESC[1].NEXT = (uint32_t)&SPI2_DMA_RXDESC[0] - (PDMA->SCATBA);   //link to first description	
		// Set TransMode
		PDMA_SetTransferMode(I2S2_RX_PDMA_CH, PDMA_SPI2_RX, TRUE, (uint32_t)&SPI2_DMA_RXDESC[0]);   
		SPI_TRIGGER_RX_PDMA(SPI2);	
		
		PDMA_EnableInt(I2S2_RX_PDMA_CH, PDMA_INT_TRANS_DONE);
//		PDMA_EnableInt(I2S2_TX_PDMA_CH, PDMA_INT_TRANS_DONE);
}
/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved. *
 *                                                              *
 ****************************************************************/
#include "SPIFlash.h"
#include <string.h>

#if defined ( __CC_ARM )
#pragma O2								// This pragma changes the optimization level, level Om range: O0~O3.
#elif defined ( __ICCARM__ )
#pragma optimize=medium					// optimization level: none, low, medium and high.
#elif defined ( __GNUC__ )
#pragma GCC optimization_level 2		// optimization level range: 0~3.
#endif


#if (defined (__ISD9300__) || defined (__I91200__))
#define SPI0_SS_NONE		(0x0ul<<SPI0_SSCTL_SS_Pos)
#define SPI0_SET_SS(spi,u32SS)									( (spi)->SSCTL = ( (spi)->SSCTL & ~SPI_SSCTL_SS_Msk ) | u32SS )
#define SPI0_SET_SLAVE_ACTIVE_LEVEL(spi,u32Level)				( (spi)->SSCTL = ( (spi)->SSCTL & ~SPI0_SSCTL_SSACTPOL_Msk ) | u32Level )
#define SPI0_SET_TX_NUM(spi,u32TxNum) 
#define SPI0_WRITE_TX0(spi, u32TxData)		SPI0_WRITE_TX(spi, u32TxData)	
#define SPI0_WRITE_TX1(spi, u32TxData)		SPI0_WRITE_TX(spi, u32TxData)
#define SPI0_READ_RX0(spi)					SPI0_READ_RX(spi)
#define SPI0_READ_RX1(spi)					SPI0_READ_RX(spi)
#define SPI0_GO(spi)							//SPI_TRIGGER(spi)
#endif

#define SPI_SS_NONE		                (0x0ul<<SPI_SSCTL_SS0_Pos)
#define SPI_SET_SS(spi,u32SS)	                        ( (spi)->SSCTL = ( (spi)->SSCTL & ~(SPI_SSCTL_SS0_Msk|SPI_SSCTL_SS1_Msk) ) | u32SS )  
#define SPI_SET_SLAVE_ACTIVE_LEVEL(spi,u32Level)		( (spi)->SSCTL = ( (spi)->SSCTL & ~SPI_SSCTL_SSACTPOL_Msk ) | u32Level )
#define SPI_GO(spi)                                     //SPI_WRITE_TX(spi,0);

void
SPIFlash_3ByteAddr_Cmd(SPI_T	*psSpiHandler, UINT32 u32Cmd, UINT32 u32ByteAddr)
{
	SPI_SET_DATA_WIDTH(psSpiHandler,32);
//	SPI_SET_TX_NUM(psSpiHandler,SPI_TXNUM_ONE);
	SPI_WRITE_TX(psSpiHandler,((UINT32)u32Cmd<<24)|(u32ByteAddr&0xffffff));
//	SPI_GO(psSpiHandler);
	while( SPI_IS_BUSY(psSpiHandler) );
}

void
SPIFlash_4ByteAddr_Cmd(SPI_T	*psSpiHandler, UINT32 u32Cmd, UINT32 u32ByteAddr)
{
	SPI_SET_DATA_WIDTH(psSpiHandler,20);
//	SPI_SET_TX_NUM(psSpiHandler,SPI0_TXNUM_TWO);
	SPI_WRITE_TX(psSpiHandler,((UINT32)u32Cmd<<12)|((u32ByteAddr&0xfff00000)>>20));
	SPI_WRITE_TX(psSpiHandler, (u32ByteAddr&0x000fffff));
//	SPI_GO(psSpiHandler);
	while( SPI_IS_BUSY(psSpiHandler) );
//	SPI_SET_TX_NUM(psSpiHandler,SPI0_TXNUM_ONE);
}

/*******************************************************************/
/*             Miscellaneous API code section                      */
/*******************************************************************/
void
SPIFlash_SendRecOneData(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	UINT32 u32Data,
	UINT8  u8DataLen
)
{
	// Active chip select
	SPI_SET_SS(psSpiFlashHandler->psSpiHandler, psSpiFlashHandler->u8SlaveDevice);
		
	// Set transmit Bit Length = u8DataLen
	SPI_SET_DATA_WIDTH(psSpiFlashHandler->psSpiHandler,u8DataLen);
	// Transmit/Receive Numbers = 1
//	SPI_SET_TX_NUM(psSpiFlashHandler->psSpiHandler,SPI0_TXNUM_ONE);
	// Write data to TX0 register
	SPI_WRITE_TX(psSpiFlashHandler->psSpiHandler,u32Data);

//	SPI_GO(psSpiFlashHandler->psSpiHandler);
	while( SPI_IS_BUSY(psSpiFlashHandler->psSpiHandler) );

	// Inactive chip select
	SPI_SET_SS(psSpiFlashHandler->psSpiHandler, SPI_SS_NONE);
}

BOOL
SPIFlash_CheckBusy(
	S_SPIFLASH_HANDLER *psSpiFlashHandler
)
{
	return (SPIFlash_ReadStatusReg(psSpiFlashHandler, eSTATUS_REG1)& SPIFLASH_BUSY);
}

void
SPIFlash_WaitReady(
	S_SPIFLASH_HANDLER *psSpiFlashHandler
)
{
	while( SPIFlash_ReadStatusReg(psSpiFlashHandler, eSTATUS_REG1)& SPIFLASH_BUSY );
}


UINT8
SPIFlash_ReadStatusReg(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	E_SPIFLASH_STATUS_REGISTER eStatusReg
)
{
#if (defined (__ISD9300__) || defined (__I91200__))
	SPI0_ClearRxFIFO(psSpiFlashHandler->psSpiHandler);
#endif
	SPIFlash_SendRecOneData(psSpiFlashHandler,(SPIFLASH_READ_STATUS|eStatusReg)<<8, 16);

	return (UINT8)SPI_READ_RX(psSpiFlashHandler->psSpiHandler);
}

void
SPIFlash_WriteStatusReg(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	UINT8 u8Status
)
{
	SPIFlash_ChipWriteEnable(psSpiFlashHandler, TRUE);

	SPIFlash_SendRecOneData(psSpiFlashHandler,(SPIFLASH_WRITE_STATUS<<8)|u8Status, 16);
	SPIFlash_WaitReady(psSpiFlashHandler);
}

//UINT32
//SPIFlash_GetVersion(void)
//{
//	return SPIFLASH_VERSION_NUM;
//}

void
SPIFlash_EN4BAddress(
	S_SPIFLASH_HANDLER *psSpiFlashHandler
)
{
#if (SPIFLASH_OPERATION_MODE == 2)
	SPIFlash_SendRecOneData(psSpiFlashHandler,SPIFLASH_EN4B_MODE,8);
	psSpiFlashHandler->u8Flag = SPIFLASH_FLAG_HIGH_CAPACITY;
	psSpiFlashHandler->pfnSPIFlashMode = SPIFlash_4ByteAddr_Cmd;
#endif
}

void
SPIFlash_EX4BAddress(
	S_SPIFLASH_HANDLER *psSpiFlashHandler
)
{
#if (SPIFLASH_OPERATION_MODE == 2)
	SPIFlash_SendRecOneData(psSpiFlashHandler,SPIFLASH_EX4B_MODE,8);
	psSpiFlashHandler->u8Flag = SPIFLASH_FLAG_LOW_CAPACITY;
	psSpiFlashHandler->pfnSPIFlashMode = SPIFlash_3ByteAddr_Cmd;
#endif
}

void
SPIFlash_Open(
	SPI_T *psSpiHandler,
	UINT8 u8DrvSlaveDevice,
	UINT32 u32SpiClk,
	S_SPIFLASH_HANDLER *psSpiFlashHandler
)
{
	psSpiFlashHandler->u8SlaveDevice = u8DrvSlaveDevice;
	psSpiFlashHandler->psSpiHandler = psSpiHandler;

#if (defined (__N572F072__) || defined (__N572P072__) || defined (__N571P032__) )
	if (psSpiHandler == SPI0)// Enable high speed pins
	   SYS->GPA_HS = 0x1f;
	// Configure SPI parameters
	// Mode0--> SPI RX latched rising edge of clock; TX latced falling edge of clock; SCLK idle low
	SPI_Open(psSpiFlashHandler->psSpiHandler, SPI_MASTER, SPI_MODE_0, u32SpiClk);
#elif (defined (__N572F065__) || defined (__N572F064__))
	// Configure SPI parameters
	// Mode0--> SPI RX latched rising edge of clock; TX latced falling edge of clock; SCLK idle low
	SPI_Open(psSpiFlashHandler->psSpiHandler, SPI_MODE_0, u32SpiClk);
#elif (defined (__ISD9100__) || defined (__N575F145__) || defined (__N570F064__) || defined (__ISD9000__) || defined (__N569S__) )
	SPI_Open(psSpiFlashHandler->psSpiHandler, SPI_MASTER, SPI_MODE_0, u32SpiClk, 0);
#elif (defined (__ISD9300__) || defined (__I91200__))
	SPI0_Open(psSpiFlashHandler->psSpiHandler, SPI0_MASTER, SPI0_MODE_0, 8, u32SpiClk);
#endif

	// bit MSB first
	SPI_SET_MSB_FIRST(psSpiFlashHandler->psSpiHandler);
	// send/receve command in big endian; write/read data in little endian
	SPI_DISABLE_BYTE_REORDER(psSpiFlashHandler->psSpiHandler);
	// transmit/receive word will be executed in one transfer
//	SPI_SET_TX_NUM(psSpiFlashHandler->psSpiHandler, SPI0_TXNUM_ONE);
	// defalut width 8 bits
	SPI_SET_DATA_WIDTH(psSpiFlashHandler->psSpiHandler, 8);
	// set Slave active level as low selection
	SPI_SET_SLAVE_ACTIVE_LEVEL(psSpiFlashHandler->psSpiHandler, SPI_SS_ACTIVE_LOW);
	// set Suspend Interval = 4 SCLK clock cycles for interval between two successive transmit/receive.
	SPI_SET_SUSPEND_CYCLE(psSpiFlashHandler->psSpiHandler, 4);
	
	psSpiFlashHandler->u32FlashSize = 0;
	psSpiFlashHandler->u8Flag = 0;
	// Inactive chip select
	SPI_SET_SS(psSpiFlashHandler->psSpiHandler, SPI_SS_NONE);

#if (defined (__ISD9300__) || defined (__I91200__)) 
	SPI0_TRIGGER(psSpiFlashHandler->psSpiHandler);
#endif

}

void
SPIFlash_GetChipInfo(
	S_SPIFLASH_HANDLER *psSpiFlashHandler
)
{
	UINT8  u8CapacityOrder;

	UINT32 u32Value;
 
#if (defined (__ISD9300__) || defined (__I91200__))
	SPI0_ClearRxFIFO(psSpiFlashHandler->psSpiHandler);
#endif
	// Get JEDEC ID command to detect Winbond, MXIC and ATmel series
	// Only W25P serious not support JEDEC ID command
	SPIFlash_SendRecOneData(psSpiFlashHandler, (UINT32)SPIFLASH_JEDEC_ID<<24, 32);
	u32Value = SPI_READ_RX(psSpiFlashHandler->psSpiHandler);
	u8CapacityOrder = ((u32Value)&0x0f); // based on 512Kbytes order
	if( ((u32Value>>16)&0xff) == 0x1f ) // Atmel SPIFlash
	{
		u8CapacityOrder = ((u32Value>>8)&0x1f); 
		u8CapacityOrder -= 1; 
	}
	//psSpiFlashHandler->u32FlashSize = (1024*512/8)<<u16CapacityOrder;
	psSpiFlashHandler->u32FlashSize = (64*1024)<<u8CapacityOrder; // Unit: 64k block bytes
}

void
SPIFlash_PowerDown(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	BOOL	bEnable
)
{
	UINT8 u8Cmd;

	if ( bEnable )
		u8Cmd = SPIFLASH_POWER_DOWN;
	else
		u8Cmd = SPIFLASH_RELEASE_PD_ID;

	SPIFlash_SendRecOneData(psSpiFlashHandler,u8Cmd,8);
}


/*******************************************************************/
/*             Protection API code section                         */
/*******************************************************************/
void
SPIFlash_ChipWriteEnable(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	BOOL bEnableWrite
)
{
	UINT8 u8Cmd;

	if ( bEnableWrite == TRUE )
		u8Cmd = SPIFLASH_WRITE_ENABLE;
	else
		u8Cmd = SPIFLASH_WRITE_DISABLE;
	
	SPIFlash_SendRecOneData(psSpiFlashHandler, u8Cmd, 8);
	SPIFlash_WaitReady(psSpiFlashHandler);
}

void
SPIFlash_GlobalProtect(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	BOOL bEnableGlobalProtect
)
{
	UINT8 u8Status=0;//u8Flag = ((psSpiFlashHandler->u8Flag)&0xf0);

	//if( (u8Flag == SPIFLASH_FLAG_WINBOND) || (u8Flag == SPIFLASH_FLAG_MXIC) )
	{
		u8Status = SPIFlash_ReadStatusReg(psSpiFlashHandler, eSTATUS_REG1);

		if(bEnableGlobalProtect)
			u8Status |= 0x1c; // Set 1 to BP0,BP1,BP2 in Status Register 
		else
		{
			if(u8Status&0x20) // Check BP3/TB bit (some flash have it) 
				u8Status &= 0xc3; // Set 0 to BP0,BP1,BP2,BP3/TB in Status Register
			else
				u8Status &= 0xe3; // Set 0 to BP0,BP1,BP2 in Status Register
		}

		SPIFlash_WriteStatusReg(psSpiFlashHandler, u8Status);
	}
	//else if ( u8Flag == SPIFLASH_FLAG_ATMEL )
	//{
	//	if(bEnableGlobalProtect)
	//		u8Status = 0x7f;	// writing a 7Fh to the first byte of the Status Register will perform a Global Protect
	//	else					// and keep the SPRL bit in the logical "0" state.
	//		u8Status = 0x00;	// writing a 00h to the first byte of the Status Register will perform a Global Unprotec
								// without changing the state of the SPRL bit
				
	//	SPIFlash_WriteStatusReg(psSpiFlashHandler, u8Status);
	//}
}

/*******************************************************************/
/*             Erase API code section                              */
/*******************************************************************/
void
SPIFlash_EraseStart(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	UINT8 u8Cmd,
	UINT32 u32Addr
)
{
	SPIFlash_ChipWriteEnable(psSpiFlashHandler, TRUE);
	
	// Active chip select
	SPI_SET_SS(psSpiFlashHandler->psSpiHandler, psSpiFlashHandler->u8SlaveDevice);
	
	// Send erase command
#if (SPIFLASH_OPERATION_MODE == 0)
	SPIFlash_3ByteAddr_Cmd(psSpiFlashHandler->psSpiHandler, u8Cmd, u32Addr);
#elif (SPIFLASH_OPERATION_MODE == 1)
	SPIFlash_4ByteAddr_Cmd(psSpiFlashHandler->psSpiHandler, u8Cmd, u32Addr);
#else
	psSpiFlashHandler->pfnSPIFlashMode(psSpiFlashHandler->psSpiHandler, u8Cmd, u32Addr);
#endif

	// Inactive chip select
	SPI_SET_SS(psSpiFlashHandler->psSpiHandler, SPI_SS_NONE);
}

/*******************************************************************/
/*             Read API code section                               */
/*******************************************************************/
void
SPIFlash_Read(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	UINT32 u32ByteAddr,
	PUINT8 pau8Data,
	UINT32 u32DataLen
)
{
	SPIFlash_ReadStart( psSpiFlashHandler, u32ByteAddr );
	// Read data
	SPIFlash_ReadData(psSpiFlashHandler, pau8Data, u32DataLen);
	SPIFlash_ReadEnd(psSpiFlashHandler);
}

void
SPIFlash_BurstRead(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	UINT32 u32ByteAddr,
	PUINT8 pau8Data,
	UINT32 u32DataLen
)
{
	SPIFlash_ReadStart( psSpiFlashHandler, u32ByteAddr );
	SPIFlash_ReadDataAlign(psSpiFlashHandler, pau8Data, u32DataLen);
	SPIFlash_ReadEnd(psSpiFlashHandler);
}

void
SPIFlash_ReadStart(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	UINT32 u32ByteAddr
)
{

	// Active chip select
	SPI_SET_SS(psSpiFlashHandler->psSpiHandler, psSpiFlashHandler->u8SlaveDevice);

	// Send fast read command
#if (SPIFLASH_OPERATION_MODE == 0)
	SPIFlash_3ByteAddr_Cmd(psSpiFlashHandler->psSpiHandler, SPIFLASH_FAST_READ, u32ByteAddr);
#elif (SPIFLASH_OPERATION_MODE == 1)
	SPIFlash_4ByteAddr_Cmd(psSpiFlashHandler->psSpiHandler, SPIFLASH_FAST_READ, u32ByteAddr);
#else
	psSpiFlashHandler->pfnSPIFlashMode(psSpiFlashHandler->psSpiHandler, SPIFLASH_FAST_READ, u32ByteAddr);
#endif

	// send dummy clcok
	SPI_SET_DATA_WIDTH(psSpiFlashHandler->psSpiHandler,8);
	SPI_WRITE_TX(psSpiFlashHandler->psSpiHandler,0);

	//SPI_SET_TX_NUM(psSpiFlashHandler->psSpiHandler,SPI_TXNUM_ONE);
//	SPI_GO(psSpiFlashHandler->psSpiHandler);
	while( SPI_IS_BUSY(psSpiFlashHandler->psSpiHandler) ){};

#if (defined (__ISD9300__) || defined (__I91200__))
	SPI0_ClearRxFIFO(psSpiFlashHandler->psSpiHandler);
#endif

    SPI_ClearRxFIFO(psSpiFlashHandler->psSpiHandler);
	// send/receve command in big endian; write/read data in little endian
	SPI_ENABLE_BYTE_REORDER(psSpiFlashHandler->psSpiHandler);

}

void
SPIFlash_ReadEnd(
	S_SPIFLASH_HANDLER *psSpiFlashHandler
)
{
	// Inactive all slave devices
	SPI_SET_SS(psSpiFlashHandler->psSpiHandler, SPI_SS_NONE);
	// send/receve command in big endian; write/read data in little endian
	SPI_DISABLE_BYTE_REORDER(psSpiFlashHandler->psSpiHandler);
}

void
SPIFlash_ReadData(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	PUINT8 pau8Data,
	UINT32 u32DataLen
)
{
	//	PA20 CYHuang12 speedup read function.
	UINT32 u32ReadData;
    UINT8  u8ProcBytes;
	
//	SPI_SET_TX_NUM(psSpiFlashHandler->psSpiHandler,SPI0_TXNUM_ONE);
	
	u8ProcBytes = ((UINT32)pau8Data)&0x3;//u8ProcBytes = ((UINT32)pau8Data)%4;
    if (u8ProcBytes!=0)
    {
        u8ProcBytes = 4 - u8ProcBytes;
        if ( u8ProcBytes > u32DataLen )
        	u8ProcBytes = u32DataLen;
		
		SPI_SET_DATA_WIDTH(psSpiFlashHandler->psSpiHandler,u8ProcBytes<<3);
#if (defined (__ISD9300__) || defined (__I91200__))
		SPI0_WRITE_TX0(psSpiFlashHandler->psSpiHandler,0);
#endif
        SPI_WRITE_TX(psSpiFlashHandler->psSpiHandler,0);
//	    SPI_GO(psSpiFlashHandler->psSpiHandler);
        u32DataLen-=u8ProcBytes;

        while( SPI_IS_BUSY(psSpiFlashHandler->psSpiHandler) );
        u32ReadData = SPI_READ_RX(psSpiFlashHandler->psSpiHandler);

        *pau8Data++ = (UINT8)u32ReadData;
        if ( u8ProcBytes >= 2 )
        	*pau8Data++ = (UINT8)(u32ReadData>>8);
        if ( u8ProcBytes >= 3 )
        	*pau8Data++ = (UINT8)(u32ReadData>>16);
    }

    SPI_SET_DATA_WIDTH(psSpiFlashHandler->psSpiHandler, 32);

    while (u32DataLen>=4)
    {
#if (defined (__ISD9300__) || defined (__I91200__))
		SPI0_WRITE_TX0(psSpiFlashHandler->psSpiHandler,0);
#endif
        SPI_WRITE_TX(psSpiFlashHandler->psSpiHandler,0);
//		SPI_GO(psSpiFlashHandler->psSpiHandler);
		while( SPI_IS_BUSY(psSpiFlashHandler->psSpiHandler) ){};
        u32ReadData = SPI_READ_RX(psSpiFlashHandler->psSpiHandler);
        *((UINT32*)pau8Data) = u32ReadData;
        pau8Data+=4;
        u32DataLen-=4;
    }

    if (u32DataLen>0)
    {
        SPI_SET_DATA_WIDTH(psSpiFlashHandler->psSpiHandler, u32DataLen<<3);
#if (defined (__ISD9300__) || defined (__I91200__))
		SPI0_WRITE_TX0(psSpiFlashHandler->psSpiHandler,0);
#endif
        SPI_WRITE_TX(psSpiFlashHandler->psSpiHandler,0);
//	    SPI_GO(psSpiFlashHandler->psSpiHandler);
        while( SPI_IS_BUSY(psSpiFlashHandler->psSpiHandler) );
        u32ReadData = SPI_READ_RX(psSpiFlashHandler->psSpiHandler);

        *pau8Data++ = (UINT8)u32ReadData;
        if ( u32DataLen >= 2 )
        	*pau8Data++ = (UINT8)(u32ReadData>>8);
        if ( u32DataLen >= 3 )
        	*pau8Data++ = (UINT8)(u32ReadData>>16);
    }
}
	
void
SPIFlash_ReadDataAlign(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	PUINT8 pau8Data,
	UINT32 u32DataLen
)
{
	UINT32 *pu32Temp = (UINT32 *)pau8Data;
	// Read data
	SPI_SET_DATA_WIDTH(psSpiFlashHandler->psSpiHandler, 32);
//	SPI_SET_TX_NUM(psSpiFlashHandler->psSpiHandler,SPI0_TXNUM_TWO);
	do
	{
#if (defined (__ISD9300__) || defined (__I91200__))
		SPI0_WRITE_TX0(psSpiFlashHandler->psSpiHandler,0);
		SPI0_WRITE_TX1(psSpiFlashHandler->psSpiHandler,0);
#else
//		SPI_GO(psSpiFlashHandler->psSpiHandler);  
        SPI_WRITE_TX(psSpiFlashHandler->psSpiHandler,0);
        SPI_WRITE_TX(psSpiFlashHandler->psSpiHandler,0);
#endif
		while( SPI_IS_BUSY(psSpiFlashHandler->psSpiHandler) );
		*(UINT32 *)pu32Temp++ = SPI_READ_RX(psSpiFlashHandler->psSpiHandler);
		*(UINT32 *)pu32Temp++ = SPI_READ_RX(psSpiFlashHandler->psSpiHandler);
		u32DataLen -= 8;
	}while(u32DataLen>0);
}

/*******************************************************************/
/*             Write API code section                              */
/*******************************************************************/
void
SPIFlash_Write(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	UINT32 u32Addr,
	PUINT8 pau8Data,
	UINT32 u32DataLen
)
{
	UINT32 u32WriteCount;
	
	while(u32DataLen!=0)
	{
		SPIFlash_WriteStart(psSpiFlashHandler, u32Addr);
		u32WriteCount = SPIFlash_WriteData(psSpiFlashHandler,u32Addr, pau8Data, u32DataLen);
		u32Addr += u32WriteCount;
		pau8Data += u32WriteCount;
		u32DataLen -= u32WriteCount;
		SPIFlash_WriteEnd(psSpiFlashHandler);
		// Wait write completely
		SPIFlash_WaitReady(psSpiFlashHandler);
	}
}

void
SPIFlash_WritePage(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	UINT32 u32PageAddr,
	PUINT8 pau8Data
)
{

	SPIFlash_WriteStart(psSpiFlashHandler, u32PageAddr);
	SPIFlash_WriteDataAlign(psSpiFlashHandler, pau8Data);
	SPIFlash_WriteEnd(psSpiFlashHandler);

	SPIFlash_WaitReady(psSpiFlashHandler);
}

void
SPIFlash_WriteStart(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	UINT32 u32ByteAddr
)
{

	SPIFlash_ChipWriteEnable(psSpiFlashHandler, TRUE);

	// Active chip select
	SPI_SET_SS(psSpiFlashHandler->psSpiHandler, psSpiFlashHandler->u8SlaveDevice);

	// Send write page command
#if (SPIFLASH_OPERATION_MODE == 0)
	SPIFlash_3ByteAddr_Cmd(psSpiFlashHandler->psSpiHandler, SPIFLASH_PAGE_PROGRAM, u32ByteAddr);
#elif (SPIFLASH_OPERATION_MODE == 1)
	SPIFlash_4ByteAddr_Cmd(psSpiFlashHandler->psSpiHandler, SPIFLASH_PAGE_PROGRAM, u32ByteAddr);
#else
	psSpiFlashHandler->pfnSPIFlashMode(psSpiFlashHandler->psSpiHandler, SPIFLASH_PAGE_PROGRAM, u32ByteAddr);
#endif
	// send/receve command in big endian; write/read data in little endian
	SPI_ENABLE_BYTE_REORDER(psSpiFlashHandler->psSpiHandler);
}

void
SPIFlash_WriteEnd(
	S_SPIFLASH_HANDLER *psSpiFlashHandler
)
{
	// Inactive all slave devices
	SPI_SET_SS(psSpiFlashHandler->psSpiHandler, SPI_SS_NONE);
	// send/receve command in big endian; write/read data in little endian
	SPI_DISABLE_BYTE_REORDER(psSpiFlashHandler->psSpiHandler);
}

UINT32
SPIFlash_WriteData(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	UINT32 u32SPIAddr,
	PUINT8 pau8Data,
	UINT32 u32DataLen
)
{
	UINT32 u32WriteCount, u32TotalWriteCount, u32ProcessByte, u32WriteData;
	
//	SPI_SET_TX_NUM(psSpiFlashHandler->psSpiHandler,SPI0_TXNUM_ONE);
	
	u32WriteCount = 256;
	if ( u32SPIAddr&0xff )//if ( u32SPIAddr%256 )
		 u32WriteCount -=  u32SPIAddr&0xff;//u32SPIAddr%256;
	if ( u32WriteCount > u32DataLen )
		u32WriteCount = u32DataLen;
	u32TotalWriteCount = u32WriteCount;

	if ( ((UINT32)pau8Data)&0x3 )//if ( ((UINT32)pau8Data)%4 )&0x3
	{
		u32ProcessByte = 4 - ( ((UINT32)pau8Data)&0x3 ); //((UINT32)pau8Data)%4;
		if ( u32ProcessByte > u32WriteCount )
			u32ProcessByte = u32WriteCount;
		SPI_SET_DATA_WIDTH(psSpiFlashHandler->psSpiHandler, u32ProcessByte*8);
		u32WriteData = *pau8Data ++;
		if ( u32ProcessByte >= 2 )
			u32WriteData |= (*pau8Data ++)<<8;
		if ( u32ProcessByte == 3 )
			u32WriteData |= (*pau8Data ++)<<16;
		SPI_WRITE_TX(psSpiFlashHandler->psSpiHandler,u32WriteData);
//		SPI_GO(psSpiFlashHandler->psSpiHandler);
		while( SPI_IS_BUSY(psSpiFlashHandler->psSpiHandler) );
		u32WriteCount -=  u32ProcessByte;
		//pau8Data += u32ProcessByte;
	}

	SPI_SET_DATA_WIDTH(psSpiFlashHandler->psSpiHandler, 32);
//	SPI_SET_TX_NUM(psSpiFlashHandler->psSpiHandler,SPI0_TXNUM_ONE);
	while(u32WriteCount >= 4)
	{
		SPI_WRITE_TX(psSpiFlashHandler->psSpiHandler,*((PUINT32)pau8Data));
//		SPI_GO(psSpiFlashHandler->psSpiHandler);
		pau8Data += 4;
		u32WriteCount -= 4;
		while( SPI_IS_BUSY(psSpiFlashHandler->psSpiHandler) );
	}
	if (u32WriteCount)
	{
		SPI_SET_DATA_WIDTH(psSpiFlashHandler->psSpiHandler, u32WriteCount*8);
		u32WriteData = *pau8Data ++;
		if ( u32WriteCount >= 2 )
			u32WriteData |= (*pau8Data ++)<<8;
		if ( u32WriteCount == 3 )
			u32WriteData |= (*pau8Data ++)<<16;
		SPI_WRITE_TX(psSpiFlashHandler->psSpiHandler,u32WriteData);
//		SPI_GO(psSpiFlashHandler->psSpiHandler);
		while( SPI_IS_BUSY(psSpiFlashHandler->psSpiHandler) );
		//pau8Data += u32WriteCount;
	}
	return u32TotalWriteCount;
}

void
SPIFlash_WriteDataAlign(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	PUINT8 pau8Data
)
{
	UINT32 u32DataLen;
	UINT32 *pu32Temp = (UINT32 *)pau8Data;
	
	SPI_SET_DATA_WIDTH(psSpiFlashHandler->psSpiHandler, 32);
//	SPI_SET_TX_NUM(psSpiFlashHandler->psSpiHandler,SPI0_TXNUM_TWO);
	u32DataLen = 256;
	do
	{
		//DrvSPI_BurstWriteData(psSpiHandler,(PUINT32)pau8Data);
		SPI_WRITE_TX(psSpiFlashHandler->psSpiHandler, *pu32Temp++);
		SPI_WRITE_TX(psSpiFlashHandler->psSpiHandler, *pu32Temp++);
		SPI_GO(psSpiFlashHandler->psSpiHandler);
		//pau8Data += 8;
		while( SPI_IS_BUSY(psSpiFlashHandler->psSpiHandler) );
	}while(	(u32DataLen -= 8) != 0 );
}


UINT32
SPIFlash_GetJedecID(
	S_SPIFLASH_HANDLER *psSpiFlashHandler
)
{
	SPIFlash_SendRecOneData(psSpiFlashHandler, (UINT32)SPIFLASH_JEDEC_ID<<24, 32);
	return (SPI_READ_RX(psSpiFlashHandler->psSpiHandler)&0x00ffffff);
}


#define SPIFLASH_CHECK_READ_COUNT	100
#define SPIFLASH_CHECK_ID_COUNT		10
void SPIFlash_WaitStable(
	S_SPIFLASH_HANDLER *psSpiFlashHandler,
	UINT32 u32MaxReadVerifyCount
)
{
	UINT32 u32VerifyCount;
	
	// Make sure SPI flash digital part is ready by checking JEDEC ID
	{
		UINT32 u32CheckJedecID;
		UINT32 u32ReadJedecID;
		
		u32VerifyCount = 0;
		u32CheckJedecID = 0;
		while(u32MaxReadVerifyCount)
		{
			u32MaxReadVerifyCount --;	
			u32ReadJedecID = SPIFlash_GetJedecID(psSpiFlashHandler);
			
			if (((UINT8)u32ReadJedecID) == 0 )
			{
				// memory capacity should not be 0
				continue;
			}
			
			if (u32CheckJedecID == u32ReadJedecID)
			{
				if ( (++u32VerifyCount) == SPIFLASH_CHECK_ID_COUNT )
					break;
			}
			else
			{
				u32CheckJedecID = u32ReadJedecID;
				u32VerifyCount = 0;
			}
		}
	}
	
	// Make SPI flash leave power down mode if some where or some time had made it entring power down mode
	SPIFlash_PowerDown(psSpiFlashHandler, FALSE);
	
	// Check SPI flash is ready for accessing
	{
		UINT8 ui8ReadByte, ui8CheckByte;
		UINT16 u16Address;
		
		ui8CheckByte = 0;
		u32VerifyCount = 0;
		u16Address = 0;
		while(u32MaxReadVerifyCount)
		{
			u32MaxReadVerifyCount --;
			SPIFlash_Read(psSpiFlashHandler, u16Address, &ui8ReadByte, 1);
			
			if ( (ui8ReadByte==0) || (ui8ReadByte==0xff) )
			{
				u16Address ++;
				u32VerifyCount = 0;
				continue;
			}
			
			if ( ui8ReadByte != ui8CheckByte )
			{
				ui8CheckByte = ui8ReadByte;
				u32VerifyCount = 0;
			}
			else
			{
				if ((++u32VerifyCount) == SPIFLASH_CHECK_READ_COUNT)
					break;
			}
		}
	}
}




/*SPI Flash settings*/
#define SPIFLASH_CLOCK (12000000)
// Define the delay time before write instruction of SPI flash
#define SPIFLASH_WRITE_DELAY_TIME     (5*1000) //us
//Define the LDO stable time
#define SPIFLASH_LDO_STABLE_TIME      (0.2*1000)//us
#define SPIFLASH_STABLE_DEAY_TIME     (SPIFLASH_WRITE_DELAY_TIME+SPIFLASH_LDO_STABLE_TIME)
#define SPIFLASH_MAX_VERIFY_COUNT     ((unsigned int)((SPIFLASH_STABLE_DEAY_TIME)/(((double)((32+8)*1000000))/(SPIFLASH_CLOCK))))
#define PIN_SELECT	2	//1:SPI0_PA, 2:SPI0_PC,.....
S_SPIFLASH_HANDLER g_sSPIFlash;


UINT8 SPIFlash_Initiate(void)
{
//	UINT32 u32Clock = 0;
	
#if (PIN_SELECT == 1)
	// SPI0: GPA3=MOSI0, GPA4=MISO0, GPA5=CLK, GPA6=SSB
	SYS->GPA_MFPL = (SYS->GPA_MFPL & ~( SYS_GPA_MFPL_PA3MFP_Msk | SYS_GPA_MFPL_PA4MFP_Msk | SYS_GPA_MFPL_PA5MFP_Msk | SYS_GPA_MFPL_PA6MFP_Msk)) | 
	(SYS_GPA_MFPL_PA6MFP_SPI0_SS0 | SYS_GPA_MFPL_PA5MFP_SPI0_CLK | SYS_GPA_MFPL_PA4MFP_SPI0_MISO0 | SYS_GPA_MFPL_PA3MFP_SPI0_MOSI0 );
#elif (PIN_SELECT == 2)
	// SPI0: GPC7=SSB, GPC10=MOSI0, GPC11=MISO0, GPC12=CLK
//	SYS->GPC_MFPH = (SYS->GPC_MFPH & ~( SYS_GPC_MFPL_PC7MFP_Msk | SYS_GPC_MFPH_PC10MFP_Msk | SYS_GPC_MFPH_PC11MFP_Msk | SYS_GPC_MFPH_PC12MFP_Msk)) | 
//	(SYS_GPC_MFPL_PC7MFP_SPI0_SS0 | SYS_GPC_MFPH_PC10MFP_SPI0_MOSI0 | SYS_GPC_MFPH_PC11MFP_SPI0_MISO0 | SYS_GPC_MFPH_PC12MFP_SPI0_CLK);
  SYS->GPC_MFPL = (SYS->GPC_MFPL & (~SYS_GPC_MFPL_PC7MFP_Msk)) | (SYS_GPC_MFPL_PC7MFP_SPI0_SS0);   
  SYS->GPC_MFPH = (	SYS->GPC_MFPH & ~(SYS_GPC_MFPH_PC10MFP_Msk|SYS_GPC_MFPH_PC11MFP_Msk | SYS_GPC_MFPH_PC12MFP_Msk )) | 
  (SYS_GPC_MFPH_PC10MFP_SPI0_MOSI0|SYS_GPC_MFPH_PC11MFP_SPI0_MISO0 | SYS_GPC_MFPH_PC12MFP_SPI0_CLK );   

#endif	

	CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_HIRC, MODULE_NoMsk);
	CLK_EnableModuleClock(SPI0_MODULE);
	SYS_ResetModule(SPI0_RST);

	SPIFlash_Open(SPI0, SPI_SS0, SPIFLASH_CLOCK, &g_sSPIFlash);
	SPI_Open(SPI0, SPI_MASTER, SPI_MODE_0, 8, SPIFLASH_CLOCK);	
	//u32Clock = SPIFlash_GetSPIClock(&g_sSPIFlash);	

	// Wait SPIFlash LDO & write stable.
	SPIFlash_WaitStable(&g_sSPIFlash, SPIFLASH_MAX_VERIFY_COUNT);	
	SPIFlash_GetChipInfo(&g_sSPIFlash);


	if( g_sSPIFlash.u32FlashSize == 0 )
		return 0;	
	else
		return 1;
}






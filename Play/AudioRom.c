/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) Nuvoton Technology Corp. All rights reserved.                                              */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/	

#include "PlatForm.h"
#include "AudioRom.h"

#if defined ( __CC_ARM )
#pragma O2									// This pragma changes the optimization level, level Om range: O0~O3.
#elif defined ( __ICCARM__ )
#pragma optimize=medium						// optimization level: none, low, medium and high.
#elif defined ( __GNUC__ )
#pragma GCC optimization_level 2			// optimization level range: 0~3.
#endif

//---------------------------------------------------------------------------------------------------------
void AudioRom_GetRomHeader( 
	PFN_AUDIO_DATAREQUEST pfnReadDataCallback,
	UINT32 u32RomStartAddr,
	S_ROM_HEADER *psRomHeader )
{
	pfnReadDataCallback( (PUINT8)psRomHeader, u32RomStartAddr, ROM_HEADER_SIZE );
	psRomHeader->u32AudioStartAddr += u32RomStartAddr;
	psRomHeader->u32SentenceStartAddr += u32RomStartAddr;
}


//---------------------------------------------------------------------------------------------------------
BOOL AudioRom_GetAudioChunkInfo(  
	PFN_AUDIO_DATAREQUEST pfnReadDataCallback,
	UINT32 u32RomStartAddr,
	UINT32 u32AudioID,
	S_ROM_AUDIO_CHUNK_INFO *psAudioChunkInfo )
{
	S_ROM_HEADER sRomHeader;

	AudioRom_GetRomHeader( pfnReadDataCallback, u32RomStartAddr, &sRomHeader );

	u32AudioID = AUDIO_GET_ID(u32AudioID);
	if( u32AudioID < sRomHeader.u32TotalAudioNum )
	{
		// Get audio chunk (or audio file) information.
		
		// The address of Audio chunk (or audio file) information is
		//	sRomHeader.u32AudioStartAddr + (Audio ID)*(ROM_AUDIO_CHINK_INFO_SIZE).
		// Note:
		//	1. sRomHeader.u32AudioStartAddr: The start address of Audio chunk (or audio file) index table
		pfnReadDataCallback( (PUINT8)psAudioChunkInfo, 
			sRomHeader.u32AudioStartAddr + u32AudioID*ROM_AUDIO_CHINK_INFO_SIZE,
			ROM_AUDIO_CHINK_INFO_SIZE );	
		
		psAudioChunkInfo->u32AudioChunkAddr += u32RomStartAddr;

		return TRUE;
	}
	return FALSE;
}

void AudioRom_GetAudioChunkHeaderInfo(
	PFN_AUDIO_DATAREQUEST pfnReadDataCallback, UINT32 u32StartAddr,
	UINT16 *pu16FormatType, UINT32 *pu32TotalSize, UINT16 *pu16SampleRate)
{
	S_AUDIOCHUNK_HEADER sAudioChunkHeader;
	
	pfnReadDataCallback(&sAudioChunkHeader, u32StartAddr, AUDIOCHUNK_HEADER_SIZE );
	*pu16FormatType = sAudioChunkHeader.u16FormatType;
	*pu32TotalSize = sAudioChunkHeader.u32TotalSize;
	*pu16SampleRate = sAudioChunkHeader.u16SmplRate;
}

//---------------------------------------------------------------------------------------------------------
BOOL AudioRom_GerEquStartAddr(
	PFN_AUDIO_DATAREQUEST pfnReadDataCallback,
	UINT32 u32RomStartAddr,
	UINT32 u32EquationID,
	UINT32 *pu32EquAddr
)
{
	S_ROM_HEADER sRomHeader;

	AudioRom_GetRomHeader( pfnReadDataCallback, u32RomStartAddr, &sRomHeader );
	
	// Retrive real equation ID.
	u32EquationID = AUDIO_GET_ID(u32EquationID);
	// Check equation ID is over the maxium equation ID or not
	if ( sRomHeader.u32TotalSentenceNum <= u32EquationID )
		return FALSE;
	
	// Get equation start address from equation index table(old version audio tool didn't support!)
	// The index table use 4 bytes to record the start address of equation.
	// The address of equation information is
	//	(sRomHeader.u32AudioStartAddr + sRomHeader.u32TotalAudioNum*ROM_AUDIO_CHINK_INFO_SIZE) + (equation ID)*4
	// Note:
	//	1. sRomHeader.u32AudioStartAddr: The start address of Audio chunk (or audio file) index table
	//	2. sRomHeader.u32AudioStartAddr+sRomHeader.u32TotalAudioNum*ROM_AUDIO_CHINK_INFO_SIZE: The start address of equation index table
	pfnReadDataCallback( pu32EquAddr,
		(sRomHeader.u32AudioStartAddr + sRomHeader.u32TotalAudioNum*ROM_AUDIO_CHINK_INFO_SIZE) // start address of equation index table
		+ u32EquationID*4, 4 );

	return TRUE;
}

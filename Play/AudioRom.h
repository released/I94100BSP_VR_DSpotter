/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) Nuvoton Technology Corp. All rights reserved.                            			       */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef __AUDIOROM_H__
#define __AUDIOROM_H__

#include "Platform.h"
#include "AudioCommon.h"
#include "NVTTypes.h"
#ifdef  __cplusplus
extern "C"
{
#endif

// -----------------------------------------------------------------------------------------------
//	Description:
//		This function is to get the total count of audio chunk (or audio file) from .ROM file.
//
//	Argument:
//		pfnReadDataCallback [in]:
//			The function to read data from storage
//		u32RomStartAddr [in]:
//			The start address of the .ROM file in storage
//		psRomHeader [out]:
//			The address to store read header form storage
//	Return:
//		none
// -----------------------------------------------------------------------------------------------
__attribute__((always_inline)) __STATIC_INLINE
UINT32 AudioRom_GetAudioNum( 
	PFN_AUDIO_DATAREQUEST pfnReadDataCallback,
	UINT32 u32RomStartAddr )
{
	UINT32 u32AudioNum;

	pfnReadDataCallback( (PUINT8)&u32AudioNum, u32RomStartAddr, sizeof(UINT32) );	

	return u32AudioNum;
}

// -----------------------------------------------------------------------------------------------
//	Description:
//		This function is to get the header of .ROM file.
//
//	Argument:
//		pfnReadDataCallback [in]:
//			The function to read data from storage
//		u32RomStartAddr [in]:
//			The start address of the .ROM file in storage
//		psRomHeader [out]:
//			The address to store read header
//	Return:
//		none
// -----------------------------------------------------------------------------------------------
void AudioRom_GetRomHeader( PFN_AUDIO_DATAREQUEST pfnReadDataCallback,
	UINT32 u32RomStartAddr, S_ROM_HEADER *psRomHeader );

// -----------------------------------------------------------------------------------------------
//	Description:
//		This function is to read the audio file (or audio chunk) information from .ROM file.
//	Argument:
//		pfnReadDataCallback [in]:
//			The function to read data from storage
//		u32RomStartAddr [in]:
//			The start address of the .ROM file in storage
//		u32AudioID [in]:
//			The ID of audio file (or audio chunk) to get its information
//		psAudioChunkInfo [out]:
//			The address to store read audio chunk information
//	Return:
//		TRUE: get audio file information successfully
//		FALE: the audio ID is over the max audio ID of the .ROM file
// -----------------------------------------------------------------------------------------------
BOOL AudioRom_GetAudioChunkInfo( PFN_AUDIO_DATAREQUEST pfnReadDataCallback, 
	UINT32 u32RomStartAddr, UINT32 u32AudioID, S_ROM_AUDIO_CHUNK_INFO *psAudioChunkInfo );

// -----------------------------------------------------------------------------------------------
//	Description:
//		This function is to read the audio chunk information from .ROM file.
//		Audio chunk informaiton includes:
//			1. The audio trunk start address
//			2. The size of audio trunk
//	Argument:
//		pfnReadDataCallback [in]:
//			The function to read data from storage
//		u32RomStartAddr [in]:
//			The start address of the .ROM file in storage
//		u32EquationID [in]:
//			The ID of equation to get the start address of equation information
//		psAudioChunkInfo [out]:
//			The address to store the start address of equation
//	Return:
//		TRUE: get the start address of equation information successfully
//		FALE: the equation ID is over the max equation ID of the .ROM file
// -----------------------------------------------------------------------------------------------
BOOL AudioRom_GerEquStartAddr( PFN_AUDIO_DATAREQUEST pfnReadDataCallback,
	UINT32 u32RomStartAddr,	UINT32 u32EquationID, UINT32 *pu32EquAddr );

// -----------------------------------------------------------------------------------------------
//	Description:
//		This function is to read the header of audio chunk (or file) from .ROM file.
//		Audio chunk header includes:
//			1. The encode format
//			2. The total size of audio trunk
//			3. The sample rate to playback this audio chunk
//		
//	Argument:
//		pfnReadDataCallback [in]:
//			The function to read data from storage
//		u32RomStartAddr [in]:
//			The start address of the .ROM file in storage
//		pu16FormatType [out]:
//			The encode format of this audio chunk
//		pu32TotalSize [out]:
//			The total size of audio trunk
//		pu16SampleRate [out]:
//			The sample rate to playback this audio chunk
//	Return:
//		TRUE: get the start address of equation information successfully
//		FALE: the equation ID is over the max equation ID of the .ROM file
// -----------------------------------------------------------------------------------------------
void AudioRom_GetAudioChunkHeaderInfo(
	PFN_AUDIO_DATAREQUEST pfnReadDataCallback, UINT32 u32StartAddr,
	UINT16 *pu16FormatType, UINT32 *pu32TotalSize, UINT16 *pu16SampleRate);

#ifdef  __cplusplus
}
#endif

#endif

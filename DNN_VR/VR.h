#ifndef __VR_H
#define __VR_H


#include "stdint.h"
#include "base_types.h"

#define Audio_Frame_Len	   160
#define VR_MEMORY_POOL_SIZE (100U * 1024U)
#define VR_STATE_SIZE 96U

typedef struct {
	uint16_t  u16DataCount;
	uint16_t  u16BufCount;
	uint16_t  u16WriteIdx;
	uint16_t  u16ReadIdx;
	int16_t*  pai16Buf;
} S_CIRBUFCTRL;



typedef struct 
{
	HANDLE hCSpotter;
//	int16_t *ai16CyberonInputBuffer;
	
	volatile uint8_t bCybBufReady;//SW
	int8_t  VR_Result;
	int16_t  ai16cy_fifibuf[Audio_Frame_Len*3];
	uint16_t  ai16cy_fifi_inder;
	uint8_t reas;
	int16_t ai16cy_capturebuf[Audio_Frame_Len * 3];
	volatile uint32_t droppedFrames;
	
	
}CY_VR_type;
extern CY_VR_type Cyberon_VR;


extern uint32_t u32CMDDataBegin;

int32_t VR_TakeFrame(int16_t samples[], uint32_t count);
void VR_FlushFrames(void);
uint32_t VR_GetFrameOverruns(void);


int32_t UnpackBin(uint8_t lpbyBin[], uint8_t *lppbyModel[], int32_t nMaxNumModel);//

int32_t AddSample(HANDLE hCSpotter, SHORT *lpsSample, INT nNumSample);

int32_t DoVR_sep(uint32_t lpbyCYBase, uint32_t lpbyModel, uint32_t nTimeOut, int (*funcCheckKeyPress)(void));

int32_t DoVR_sep_MapID(uint32_t lpbyCYBase, uint32_t lpbyModel, uint32_t lpbyMapID, uint32_t nTimeOut, int (*funcCheckKeyPress)(void));

//int32_t DoVR_multi_MapID(uint32_t lpbyBaseModel, uint32_t lppbyModel, uint32_t lppbyMapID, uint32_t nNumModel, uint32_t nTimeOut, int (*funcCheckKeyPress)(void));



#endif


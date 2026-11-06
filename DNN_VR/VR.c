#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "base_types.h"
#include "DSpotterSDKApi.h"
#include "I94100.h"

#include "VR.h"


CY_VR_type Cyberon_VR;



S_CIRBUFCTRL sCirBufCtrl; 
/*---------------------------------------------------------------------------------------------------------*/
/* Define macro & global variables                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define k_nStateSize		(VR_STATE_SIZE)
#define k_nMaxTime			(300)
#define k_nMemSize			(VR_MEMORY_POOL_SIZE)
#define k_nSampleNum		(160 * 3)//add 3 frames//Recording buff size:256, Sample size for CSpotter is 128, so call CSpotter_AddSample 2 times

__align(4) BYTE lpbyMemPool[k_nMemSize];
__align(4) BYTE lpbyState[k_nStateSize] = { 0 };

INT nCurrent_addr = 0x32000;

extern uint32_t u32RecordAddr;//SW
//extern void Record2DF(void);//SW

/** add code start **/
/*
 * Take a stable 480-sample PCM snapshot from the interrupt producer.
 * Restore the previous interrupt mask so callers also work in masked code.
 * Return 1 when copied, 0 when no frame is ready, or -1 for invalid buffers.
 */
int32_t VR_TakeFrame(int16_t samples[], uint32_t count)
{
	uint32_t interrupt_mask;
	int32_t ready;

	if (samples == NULL || count != Audio_Frame_Len * 3U)
		return -1;
	interrupt_mask = __get_PRIMASK();
	__disable_irq();
	ready = Cyberon_VR.bCybBufReady != 0U;
	if (ready) {
		memcpy(samples, Cyberon_VR.ai16cy_fifibuf, count * sizeof(int16_t));
		Cyberon_VR.bCybBufReady = 0U;
	}
	__set_PRIMASK(interrupt_mask);
	return ready;
}

/* Discard PCM recorded during playback or between recognition stages. */
void VR_FlushFrames(void)
{
	uint32_t interrupt_mask;

	interrupt_mask = __get_PRIMASK();
	__disable_irq();
	Cyberon_VR.bCybBufReady = 0U;
	__set_PRIMASK(interrupt_mask);
}

/* A rising counter means the foreground recognizer missed a producer frame. */
uint32_t VR_GetFrameOverruns(void)
{
	return Cyberon_VR.droppedFrames;
}
/** add code end **/

int32_t AddSample(HANDLE hCSpotter, SHORT *lpsSample, INT nNumSample)
{
	INT nRet;
	INT i;
	
	for(i =0; i<k_nSampleNum*2; i+=k_nSampleNum)
	{
		nRet = DSpotter_AddSample((HANDLE)hCSpotter, lpsSample+i, k_nSampleNum);
		if (nRet == DSPOTTER_SUCCESS)
			break;
	}
	
	return nRet;
}



//Add sample and get result
int32_t DoVR_get(uint32_t hDSpotter, uint32_t nTimeOut, int (*funcCheckKeyPress)(void))
{
  INT	nID, nWait;
  INT nRet = 0;
  SHORT *lpsSample;
  INT nNumSample;
  nWait = 0;
  
	while (1)
	{
		if (funcCheckKeyPress && (nID = funcCheckKeyPress()) > 0)
		{
			nRet = nID;
			break;
		}

		//For VR, do preprocess
		nNumSample = Audio_Frame_Len*3;//SW

		if(Cyberon_VR.bCybBufReady)//SW
		{
			lpsSample =  Cyberon_VR.ai16cy_fifibuf;
			
			if (DSpotter_AddSample((HANDLE)hDSpotter, lpsSample, nNumSample) == DSPOTTER_SUCCESS)
			//if (AddSample((HANDLE)hCSpotter, lpsSample, nNumSample) == DSPOTTER_SUCCESS)
			{
				Cyberon_VR.reas = DSpotter_GetResult((HANDLE)hDSpotter);
				nID = Cyberon_VR.reas;
				
				nRet = nID + 1;
				nWait = 0;
				break;
				
			}
			
			nWait++;
			if (nTimeOut > 0 && nWait >= nTimeOut)
			break;
			Cyberon_VR.bCybBufReady=0;   //SW 
		}//SW
	}

//  DSpotter_Release((HANDLE)hDSpotter);
  
//L_ERROR:
  DSpotter_Release((HANDLE)hDSpotter);
  
  return nRet;	
}


int32_t DoVR_get_MapID(uint32_t hDSpotter, uint32_t nTimeOut, int (*funcCheckKeyPress)(void))
{
	INT	nID, nWait;
	INT nRet = 0;
	SHORT *lpsSample, *lpsSample1, *lpsSample2, *lpsSample3;//Mic 1/2/3/4
	INT nNumSample;	
//	INT nRetCode;	
//	INT nEnergy;
	
	nWait = 0;

	DSpotter_Reset((HANDLE)hDSpotter);

	while (1)
	{
		if (funcCheckKeyPress && (nID = funcCheckKeyPress()) > 0)
		{
			nRet = nID;
			break;
		}

		//For VR, do preprocess
		nNumSample = Audio_Frame_Len*3;//SW

		if(Cyberon_VR.bCybBufReady)//SW
		{
			lpsSample =  Cyberon_VR.ai16cy_fifibuf;
			
			if (DSpotter_AddSample((HANDLE)hDSpotter, lpsSample, nNumSample) == DSPOTTER_SUCCESS)
			{
				nRet = DSpotter_GetResultMapID((HANDLE)hDSpotter);//SW 20240429 
				//nRet = nID + 1;
				nWait = 0;
				break;
				
			}
			
			nWait++;
			if (nTimeOut > 0 && nWait >= nTimeOut)
			break;
			Cyberon_VR.bCybBufReady=0;   //SW 
		}//SW
	}
	
	DSpotter_Release((HANDLE)hDSpotter);
	
	return nRet;
	
}

//int32_t DoVR_sep(uint32_t lpbyCYBase, uint32_t lpbyModel, uint32_t bShowOnly, uint32_t nTimeOut, int (*funcCheckKeyPress)(void))
//{
//	HANDLE hCSpotter = NULL;
//	INT nin, nErr = DSPOTTER_ERR_LeaveNoMemory;
//	
//	nin = DSpotter_GetMemoryUsage_Sep((BYTE *)lpbyCYBase, (BYTE *)lpbyModel, k_nMaxTime);
//	if (nin > k_nMemSize)		return -1;
//	
//	hCSpotter = DSpotter_Init_Sep((BYTE *)lpbyCYBase, (BYTE *)lpbyModel, k_nMaxTime, lpbyMemPool, k_nMemSize, lpbyState, k_nStateSize, &nErr);
//	if (hCSpotter == NULL)		return -1;
//	
//	DSpotter_Reset((HANDLE)hCSpotter);
//	
//	return DoVR_get((uint32_t)hCSpotter, bShowOnly, nTimeOut, funcCheckKeyPress);
//}


int32_t UnpackBin(uint8_t lpbyBin[], uint8_t *lppbyModel[], int32_t nMaxNumModel)//SW
{
    uint32_t *lpnBin = (uint32_t *)lpbyBin;
    int32_t nNumBin = lpnBin[0];
    uint32_t *lpnBinSize = lpnBin + 1;
    int32_t i;

    lppbyModel[0] = (uint8_t *)(lpnBinSize + nNumBin);
    for (i = 1; i < nNumBin; i++)
    {
        if (i >= nMaxNumModel)
            break;
        lppbyModel[i] = lppbyModel[i-1] + lpnBinSize[i-1];
    }
    return i;
}



/*
 *DoVR_sep, Recognize single group
 */
int32_t DoVR_sep(uint32_t lpbyCYBase, uint32_t lpbyModel, uint32_t nTimeOut, int (*funcCheckKeyPress)(void))
{
	HANDLE hDSpotter = NULL;
	INT n, nErr = DSPOTTER_ERR_LeaveNoMemory;

	n = DSpotter_GetMemoryUsage_Multi((BYTE *)lpbyCYBase, (BYTE **)&lpbyModel, 1,k_nMaxTime);
	if (n > k_nMemSize)
		return -1;

	hDSpotter = DSpotter_Init_Multi((BYTE *)lpbyCYBase, (BYTE **)&lpbyModel, 1, k_nMaxTime, lpbyMemPool, k_nMemSize, lpbyState, k_nStateSize, &nErr);
	if (hDSpotter == NULL)
		return nErr;//-1;
	
	
	return DoVR_get((uint32_t)hDSpotter, nTimeOut, funcCheckKeyPress);
}

/*
*DoVR_multi , Recognize multi-groups
*/
int32_t DoVR_multi(uint32_t lpbyBaseModel, uint32_t lppbyModel, uint32_t nNumModel, uint32_t nTimeOut, int (*funcCheckKeyPress)(void))
{
	HANDLE hDSpotter = NULL;
	INT n, nErr = DSPOTTER_ERR_LeaveNoMemory;
	
	n = DSpotter_GetMemoryUsage_Multi((BYTE *)lpbyBaseModel, (BYTE **)lppbyModel, nNumModel, k_nMaxTime);
	if (n > k_nMemSize)
		return -1;

	hDSpotter = DSpotter_Init_Multi((BYTE *)lpbyBaseModel, (BYTE **)lppbyModel, nNumModel, k_nMaxTime, lpbyMemPool, k_nMemSize, lpbyState, k_nStateSize, &nErr);
	if (hDSpotter == NULL)
		return -1;
	
	return DoVR_get((uint32_t)hDSpotter, nTimeOut, funcCheckKeyPress);
}


HANDLE VR_init(uint32_t lpbyCYBase, uint32_t lpbyModel, uint32_t nTimeOut, int (*funcCheckKeyPress)(void))
{
	HANDLE hDSpotter = NULL;
	INT nErr = DSPOTTER_ERR_LeaveNoMemory;

	hDSpotter = DSpotter_Init_Multi((BYTE *)lpbyCYBase, (BYTE **)&lpbyModel, 1, k_nMaxTime, lpbyMemPool, k_nMemSize, lpbyState, k_nStateSize, &nErr);
	
	return hDSpotter;
}

/*
 *DoVR_sep_MapID, Recognize single group, use MapID to Mapping result
 */
int32_t DoVR_sep_MapID(uint32_t lpbyCYBase, uint32_t lpbyModel, uint32_t lpbyMapID, uint32_t nTimeOut, int (*funcCheckKeyPress)(void))
{
	HANDLE hDSpotter = NULL;
	INT n, nErr = DSPOTTER_ERR_LeaveNoMemory;
	BYTE **ppTest = (BYTE**)&lpbyModel;

	n = DSpotter_GetMemoryUsage_Multi((BYTE *)lpbyCYBase, (BYTE **)&lpbyModel, 1,k_nMaxTime);
	if (n > k_nMemSize)
		return -1;

	hDSpotter = DSpotter_Init_Multi((BYTE *)lpbyCYBase, (BYTE **)&lpbyModel, 1, k_nMaxTime, lpbyMemPool, k_nMemSize, lpbyState, k_nStateSize, &nErr);
	if (hDSpotter == NULL)
		return -1;
	
	//2.2.2x.x later do not need this
	/*n = DSpotter_SetResultMapID_Multi(hDSpotter, (BYTE **)&lpbyMapID, 1);
	if (n != DSPOTTER_SUCCESS)
	{
		DSpotter_Release(hDSpotter);
		return -1;
	}*/
	
	return DoVR_get_MapID((uint32_t)hDSpotter, nTimeOut, funcCheckKeyPress);
}
//int32_t DoVR_sep_MapID(uint32_t lpbyCYBase, uint32_t lpbyModel, uint32_t lpbyMapID, uint32_t nTimeOut, int (*funcCheckKeyPress)(void))
//{
//	HANDLE hDSpotter = NULL;
//	INT n, nErr = DSPOTTER_ERR_LeaveNoMemory;

//	n = DSpotter_GetMemoryUsage_Multi((BYTE *)lpbyCYBase, (BYTE **)&lpbyModel, 1,k_nMaxTime);
//	if (n > k_nMemSize)
//		return -1;

//	hDSpotter = DSpotter_Init_Multi((BYTE *)lpbyCYBase, (BYTE **)&lpbyModel, 1, k_nMaxTime, lpbyMemPool, k_nMemSize, lpbyState, k_nStateSize, &nErr);
//	if (hDSpotter == NULL)
//		return -1;
//	
//	n = DSpotter_SetResultMapID_Multi(hDSpotter, (BYTE **)&lpbyMapID, 1);
//	if (n != DSPOTTER_SUCCESS)
//	{
//		DSpotter_Release(hDSpotter);
//		return -1;
//	}
//	
//	return DoVR_get_MapID((uint32_t)hDSpotter, nTimeOut, funcCheckKeyPress);
//}

/*
*DoVR_multi_MapID , Recognize multi-groups, also use MapID to Mapping result
*nNumModel for VR / MapID must same, otherwise API will return error
*/
int32_t DoVR_multi_MapID(uint32_t lpbyBaseModel, uint32_t **lppbyModel, uint32_t lppbyMapID, uint32_t nNumModel, uint32_t nTimeOut, int (*funcCheckKeyPress)(void))
{
	HANDLE hDSpotter = NULL;
	INT n, nErr = DSPOTTER_ERR_LeaveNoMemory;
	//uint32_t **ppTest = (uint32_t**)&lppbyModel;

	n = DSpotter_GetMemoryUsage_Multi((BYTE *)lpbyBaseModel, (BYTE **)lppbyModel, nNumModel, k_nMaxTime);

	if (n > k_nMemSize)
		return -1;

	hDSpotter = DSpotter_Init_Multi((BYTE *)lpbyBaseModel, (BYTE **)lppbyModel, nNumModel, k_nMaxTime, lpbyMemPool, k_nMemSize, lpbyState, k_nStateSize, &nErr);
	if (hDSpotter == NULL)
		return -1;
		
	return DoVR_get_MapID((uint32_t)hDSpotter, nTimeOut, funcCheckKeyPress);
}




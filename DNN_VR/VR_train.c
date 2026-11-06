#include <stdio.h>
#include <string.h>
#include "I94100.h"
#include "NVTTypes.h"
#include "base_types.h"
//#include "DSpotterSDTrainApi.h"
#include "DSpotterVoiceSDTrainApi.h"
#include "DataFlashApi.h"
#include "VR_train.h"
#include "VR.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/

//void Wave_StartRecord(void);
//void Wave_StopRecord(void);
//int32_t Wave_GetSample_4ch(int16_t **lppsSample1, int16_t **lppsSample2, int16_t **lppsSample3, int16_t **lppsSample4);
//int32_t Wave_UnlockSample_4ch(int16_t **lppsSample);
int32_t Wave_CheckCpuOverload(void);
void Wave_ClearCpuOverload(void);

#define CLR_LED(PORT, IOPIN)			GPIO_SET_OUT_DATA(PORT,GPIO_GET_OUT_DATA(PORT)&(~IOPIN))
#define SET_LED(PORT, IOPIN)			GPIO_SET_OUT_DATA(PORT,GPIO_GET_OUT_DATA(PORT)|IOPIN)

/*---------------------------------------------------------------------------------------------------------*/
/* Define macro & global variables                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define k_nUttrBufSize			0x4000//16KB
#define k_nMemSize					(100*1024)
#define k_nSampleNum				(160 * 3)//add 3 frames
#define k_nFrameSize				(160)

#define CLR_LED(PORT, IOPIN)			GPIO_SET_OUT_DATA(PORT,GPIO_GET_OUT_DATA(PORT)&(~IOPIN))
#define SET_LED(PORT, IOPIN)			GPIO_SET_OUT_DATA(PORT,GPIO_GET_OUT_DATA(PORT)|IOPIN)

//Share same buff with VR
extern __align(4) BYTE lpbyMemPool[k_nMemSize];

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
//Flash Erase Wrtie move outside SDK
int32_t DoVR_train_MapID(uint32_t lpbyCYBase, uint32_t lpbyTrimap, uint32_t lpbySDModel, uint32_t nSDModelSize, uint32_t lpbySDBuffer, uint32_t nBufSize, int nMapID, uint32_t nTimeOut, int (*funcCheckKeyPress)(void))
{
	HANDLE hDSpotter = NULL;
	INT n, nErr = DSPOTTER_ERR_LeaveNoMemory;
	INT	nID, nWait, bStop;
	SHORT *lpsUttrAddr;
	SHORT *lpsSample;//CK, *lpsSample1, *lpsSample2, *lpsSample3;//Mic 1/2/3/4
	INT nNumSample;
	INT nStart, nEnd;

//==refer to SW's SD project==
	// Enable ISP function
	SYS_UnlockReg() ;
	FMC_Open();

	// Prepare flash storage for uttr buffer
	DataFlash_Erase((BYTE *)lpbySDBuffer, nBufSize);
//===========================
	
	lpsUttrAddr = (SHORT *)lpbySDBuffer;

	// Start training
	n = DSpotterVoiceSD_GetMemoryUsage((BYTE *)lpbyCYBase, (BYTE *)lpbyTrimap);
	if (n > k_nMemSize)
		goto L_ERROR;

	hDSpotter = DSpotterVoiceSD_Init((BYTE *)lpbyCYBase, (BYTE *)lpbyTrimap, lpbyMemPool, k_nMemSize, &nErr);
	if (hDSpotter == NULL)
		goto L_ERROR;
		
	bStop = FALSE;
		
	if ((nErr = DSpotterVoiceSD_AddUttrStart(hDSpotter, lpsUttrAddr, k_nUttrBufSize)) != DSPOTTER_SUCCESS)
		goto L_ERROR;

//	SET_LED(PA,BIT4);
//	Wave_StartRecord();CK

	nWait = 0;
	nStart = nEnd = 0;

	while (1)
	{
		if (funcCheckKeyPress && (nID = funcCheckKeyPress()) > 0)
		{
			bStop = TRUE;
			break;
		}
		
		lpsSample =  Cyberon_VR.ai16cy_fifibuf;
		nNumSample = PCM_SAMPLE_NUM;
//		nNumSample = Wave_GetSample_4ch(&lpsSample, &lpsSample1, &lpsSample2, &lpsSample3);CK
			
//		if (nNumSample < 0)
//			break;
//		if (nNumSample == 0)
//			continue;
//		if (nWait < 20)
//		{
////			Wave_UnlockSample_4ch(&lpsSample);CK		
//				
//			nWait++;
//			continue;
//		}
		if(Cyberon_VR.bCybBufReady)//SW
		{
			if ((nErr = DSpotterVoiceSD_AddSample(hDSpotter, lpsSample, nNumSample)) != DSPOTTER_ERR_NeedMoreSample)
				break;

//		Wave_UnlockSample_4ch(&lpsSample);CK
			
			nWait++;
			if (nTimeOut > 0 && nWait >= nTimeOut && nStart == 0)
				goto L_ERROR;
			Cyberon_VR.bCybBufReady=0;   //CK
		}
	}

//	Wave_StopRecord();CK
		
//	CLR_LED(PA,BIT4);

		if ((nErr = DSpotterVoiceSD_AddUttrEnd(hDSpotter)) != DSPOTTER_SUCCESS)
			goto L_ERROR;
		DSpotterVoiceSD_GetUttrEPD(hDSpotter, &nStart, &nEnd);

		if (bStop)
		{
			goto L_ERROR;
		}
	
		if ((nErr = DSpotterVoiceSD_TrainWord_WithMapID(hDSpotter, (char *)lpbySDModel, nSDModelSize, &n, nMapID)) != DSPOTTER_SUCCESS)
			goto L_ERROR;
	
		nErr = 0;


L_ERROR:

//	CLR_LED(PA,BIT4);
//	Wave_StopRecord();CK
	DSpotterVoiceSD_Release(hDSpotter);
		
	// Disable ISP function
	FMC_Close();
	SYS_LockReg();

	return nErr;
}

//This function is use for delete whole SD model
int32_t DoVR_DeleteWord_All(uint32_t lpbyCYBase, uint32_t lpbyTrimap, uint32_t lpbySDModel)
{
	HANDLE hDSpotter = NULL;
	INT n, nErr = DSPOTTER_ERR_LeaveNoMemory;
	int i,nSDNum = DSpotter_GetNumWord((BYTE *)lpbySDModel);

	// Enable ISP function//CK follow SW's SD project
	SYS_UnlockReg();
	FMC_Open();
	
//	#ifdef DEBUG_ENABLE_SEMIHOSTX
		printf("Number of SD %d\n",nSDNum);
//	#endif
	//Init 
	n = DSpotterVoiceSD_GetMemoryUsage((BYTE *)lpbyCYBase, (BYTE *)lpbyTrimap);
	if (n > k_nMemSize)
		goto L_ERROR;
//#ifdef DEBUG_ENABLE_SEMIHOSTX
		printf("SD Memory %d\n",n);
//#endif
	hDSpotter = DSpotterVoiceSD_Init((BYTE *)lpbyCYBase, (BYTE *)lpbyTrimap, lpbyMemPool, k_nMemSize, &nErr);
	if (hDSpotter == NULL)
		goto L_ERROR;
	nSDNum /=3;
//#ifdef DEBUG_ENABLE_SEMIHOSTX
		printf("SD init %d %d\n",nErr,nSDNum);
//#endif
	//SD is 1 Cmd + 2 GW ,nSDNum /3 = nIdx
	for(i = nSDNum-1;i >= 0;i--)
		nErr = DSpotterVoiceSD_DeleteWord(hDSpotter, (char *)lpbySDModel, i, 0);
//#ifdef DEBUG_ENABLE_SEMIHOSTX
		printf("DSpotterVoiceSD_DeleteWord %d %d\n",i,nErr);
//#endif	
L_ERROR:
	DSpotterVoiceSD_Release(hDSpotter);

	// Disable ISP function//CK follow SW's SD project
	FMC_Close();
	SYS_LockReg();
	
	return nErr;
}

//This function is use for delete all SD with MapID pass in
int32_t DoVR_DeleteWord_MapID(uint32_t lpbyCYBase, uint32_t lpbyTrimap, uint32_t lpbySDModel, int32_t nMapID)
{
	HANDLE hDSpotter = NULL;
	INT n, nErr = DSPOTTER_ERR_LeaveNoMemory;
	SHORT *psMapID;
	//SD is 1 Cmd + 2 GW, nSDNum will be Multiples of 3
	INT i,nSDNum = DSpotter_GetNumWord((BYTE *)lpbySDModel);

	// Enable ISP function//CK follow SW's SD project
	SYS_UnlockReg();
	FMC_Open();
	
//	#ifdef DEBUG_ENABLE_SEMIHOST
		printf("Current Number of SD %d\n",nSDNum);
		printf("Delete MapID %d SD \n",nMapID);
//	#endif
	//Init 
	n = DSpotterVoiceSD_GetMemoryUsage((BYTE *)lpbyCYBase, (BYTE *)lpbyTrimap);
	if (n > k_nMemSize)
		goto L_ERROR;

	hDSpotter = DSpotterVoiceSD_Init((BYTE *)lpbyCYBase, (BYTE *)lpbyTrimap, lpbyMemPool, k_nMemSize, &nErr);
	if (hDSpotter == NULL)
		goto L_ERROR;
	
	DSpotterVoiceSD_DeleteWord_ByMapID(hDSpotter, (char *)lpbySDModel, nMapID, 0);
	
L_ERROR:
	DSpotterVoiceSD_Release(hDSpotter);
	
	#ifdef DEBUG_ENABLE_SEMIHOST
		nSDNum = DSpotter_GetNumWord((BYTE *)lpbySDModel);
		printf("After Delete Number of SD %d\n",nSDNum);
	#endif
  if (nSDNum <= 0)
		memset((void*)lpbySDModel, 0, sizeof(lpbySDModel));

  // Disable ISP function//CK follow SW's SD project
	FMC_Close();
	SYS_LockReg();
  
	return nErr;
}


int32_t DoVR_train_Offline(uint32_t lpbyCYBase, uint32_t lpbyTrimap, uint32_t lpbySDModel, uint32_t nSDModelSize, uint32_t lpbySDBuffer, uint32_t nBufSize, uint32_t lpsPCM, uint32_t nPCMSize)
{
	HANDLE hDSpotter = NULL;
	INT n, nErr = DSPOTTER_ERR_LeaveNoMemory;
	INT	bStop;
	SHORT *lpsUttrAddr;
	SHORT *lpsSample = (SHORT *)lpsPCM;
	//INT nNumSample;
	INT nStart, nEnd;
	INT i;

	// Enable ISP function
	SYS_UnlockReg() ;
	FMC_Open();

	// Prepare flash storage for uttr buffer
	DataFlash_Erase((BYTE *)lpbySDBuffer, nBufSize);

	lpsUttrAddr = (SHORT *)lpbySDBuffer;

	// Start training
	n = DSpotterVoiceSD_GetMemoryUsage((BYTE *)lpbyCYBase, (BYTE *)lpbyTrimap);
	if (n > k_nMemSize)
		goto L_ERROR;

	hDSpotter = DSpotterVoiceSD_Init((BYTE *)lpbyCYBase, (BYTE *)lpbyTrimap, lpbyMemPool, k_nMemSize, &nErr);
	if (hDSpotter == NULL)
		goto L_ERROR;
		
	bStop = FALSE;
		
	if ((nErr = DSpotterVoiceSD_AddUttrStart(hDSpotter, lpsUttrAddr, k_nUttrBufSize)) != DSPOTTER_SUCCESS)
		goto L_ERROR;

//	SET_LED(PA,BIT4);

	nStart = nEnd = 0;

	for (i = 0; i + k_nFrameSize < nPCMSize; i += k_nFrameSize)
	{
		if ((nErr = DSpotterVoiceSD_AddSample(hDSpotter, &lpsSample[i], k_nFrameSize)) != DSPOTTER_ERR_NeedMoreSample)
			break;
		DSpotterVoiceSD_GetUttrEPD(hDSpotter, &nStart, &nEnd);
	}

//	CLR_LED(PA,BIT4);

	if ((nErr = DSpotterVoiceSD_AddUttrEnd(hDSpotter)) != DSPOTTER_SUCCESS)
		goto L_ERROR;
	DSpotterVoiceSD_GetUttrEPD(hDSpotter, &nStart, &nEnd);

	if (bStop)
	{
		goto L_ERROR;
	}
	
	if ((nErr = DSpotterVoiceSD_TrainWord(hDSpotter, (char *)lpbySDModel, nSDModelSize, &n)) != DSPOTTER_SUCCESS)
		goto L_ERROR;
	
	nErr = 0;

	FMC_Close();
	SYS_LockReg();

L_ERROR:

//	CLR_LED(PA,BIT4);
	DSpotterVoiceSD_Release(hDSpotter);

 	// Disable ISP function
	FMC_Close();
	SYS_LockReg();

	return nErr;
}

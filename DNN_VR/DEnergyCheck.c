//#include <stdlib.h>
//#include <string.h>
//#include "bsr_mem.h"
#include "DEnergyCheck.h"

#define RING_BUFFER_SUCCESS						(0)
#define RING_BUFFER_ERR_OUT_OF_MEMORY			(-1)
#define RING_BUFFER_ERR_ILLEGAL_PARAM			(-2)
#define RING_BUFFER_ERR_NO_ENOUGH_DATA			(-3)
#define RING_BUFFER_ERR_PUT_DISABLE				(-4)
#define RING_BUFFER_ERR_PUT_NEED_DISABLE		(-5)

//For Calculate Energy
extern int DSPTR_010000(int v);

#define kVarShifter				6
static INT CalculateEnergy( SHORT* samples , INT frameSize )
{
	INT i , val , squareSum = 0 ;

	for( i = 0 ; i < frameSize ; ++i )
	{
		val        = samples[i] >> 4 ;
		squareSum += val * val ;
	}
	
	val = DSPTR_010000( squareSum ) >> kVarShifter ;

	return val ;
}

int DEnergyCheck_GetMemUsage()
{
	int nRingBufferSize = 100 * sizeof(INT);
	return (int)sizeof(RingBuffer_DE) + (int)sizeof(int) + nRingBufferSize + (int)sizeof(int);
}

int DEnergyCheck_Init(void *lpMem, int nMemSize, HANDLE *lphRingBuffer)
{
	RingBuffer_DE *poRingBuffer;
	int nRingBufferSize = 100 * sizeof(INT);

	if (lphRingBuffer == NULL || nRingBufferSize <= 0)
		return RING_BUFFER_ERR_ILLEGAL_PARAM;
	else
		*lphRingBuffer = NULL;

	if (lpMem == NULL)
	{
		return RING_BUFFER_ERR_OUT_OF_MEMORY;
	}
	else
	{
		size_t uMem;
		size_t n;

		if (nMemSize < DEnergyCheck_GetMemUsage())
			return RING_BUFFER_ERR_ILLEGAL_PARAM;

		// For INT alignment
		uMem = (size_t)lpMem;
		n = uMem % sizeof(int);
		if (n != 0)
			poRingBuffer = (RingBuffer_DE *)(uMem + (sizeof(int) - n));
		else
			poRingBuffer = (RingBuffer_DE *)uMem;
		memset(poRingBuffer, 0, sizeof(RingBuffer_DE));
		poRingBuffer->bUseMalloc = FALSE;

		// For INT alignment
		uMem = (size_t)((char*)poRingBuffer + sizeof(RingBuffer_DE));
		n = uMem % sizeof(int);
		if (n != 0)
			poRingBuffer->lpBuffer = (BYTE *)(uMem + (sizeof(int) - n));
		else
			poRingBuffer->lpBuffer = (BYTE *)uMem;
	}

	poRingBuffer->nBufferSize = nRingBufferSize;
	poRingBuffer->nStart = 0;
	poRingBuffer->nEnd = 0;
	poRingBuffer->nDataSize = 0;
	poRingBuffer->bEnablePut = TRUE;

	*lphRingBuffer = (HANDLE)poRingBuffer;

	return RING_BUFFER_SUCCESS;
}

int DEnergyCheck_Release(HANDLE hRingBuffer)
{
	RingBuffer_DE *poRingBuffer = (RingBuffer_DE *)hRingBuffer;

	if (poRingBuffer == NULL)
		return RING_BUFFER_ERR_ILLEGAL_PARAM;

	poRingBuffer->nBufferSize = 0;
	poRingBuffer->nStart = 0;
	poRingBuffer->nEnd = 0;
	poRingBuffer->nDataSize = 0;
	if (poRingBuffer->bUseMalloc)
	{
		//SAFE_FREE(poRingBuffer->lpBuffer);
		//free(poRingBuffer);
	}
	else
	{
		poRingBuffer->lpBuffer = NULL;
	}

	return RING_BUFFER_SUCCESS;
}

int RingBufferIsFull_DE(HANDLE hRingBuffer)
{
	RingBuffer_DE *poRingBuffer = (RingBuffer_DE *)hRingBuffer;

	if (poRingBuffer == NULL)
		return RING_BUFFER_ERR_ILLEGAL_PARAM;

	return (poRingBuffer->nDataSize >= poRingBuffer->nBufferSize) ? 1 : 0;
}

int RingBufferPutData_DE(HANDLE hRingBuffer, const void *lpData, int nDataSize)
{
	RingBuffer_DE *poRingBuffer = (RingBuffer_DE *)hRingBuffer;

	if (poRingBuffer == NULL || lpData == NULL || nDataSize < 0)
		return RING_BUFFER_ERR_ILLEGAL_PARAM;

	if (!poRingBuffer->bEnablePut)
		return RING_BUFFER_ERR_PUT_DISABLE;

	// Check RingBuffer has enough free space.
	if (nDataSize > poRingBuffer->nBufferSize - poRingBuffer->nDataSize)
		return RING_BUFFER_ERR_NO_ENOUGH_DATA;

	if (poRingBuffer->nBufferSize - poRingBuffer->nEnd >= nDataSize)
		memcpy(poRingBuffer->lpBuffer + poRingBuffer->nEnd, lpData, (UINT)nDataSize);
	else
	{
		int n1 = poRingBuffer->nBufferSize - poRingBuffer->nEnd;
		int n2 = nDataSize - n1;
		memcpy(poRingBuffer->lpBuffer + poRingBuffer->nEnd, lpData, (UINT)n1);
		memcpy(poRingBuffer->lpBuffer, (char *)lpData + n1, (UINT)n2);
	}

	poRingBuffer->nEnd += nDataSize;
	poRingBuffer->nDataSize += nDataSize;

	if (poRingBuffer->nEnd >= poRingBuffer->nBufferSize)
	{
		poRingBuffer->nEnd -= poRingBuffer->nBufferSize;
		poRingBuffer->bOverEnd = TRUE;
	}

	return RING_BUFFER_SUCCESS;
}

int RingBufferGetData_DE(HANDLE hRingBuffer, void *lpData, int nDataSize)
{
	RingBuffer_DE *poRingBuffer = (RingBuffer_DE *)hRingBuffer;

	// Check parameter
	if (poRingBuffer == NULL || lpData == NULL || nDataSize < 0)
		return RING_BUFFER_ERR_ILLEGAL_PARAM;
	
	// Check RingBuffer has enough data.
	if (poRingBuffer->nDataSize < nDataSize)
		return RING_BUFFER_ERR_NO_ENOUGH_DATA;

	if (poRingBuffer->nBufferSize - poRingBuffer->nStart >= nDataSize)
		memcpy(lpData, poRingBuffer->lpBuffer + poRingBuffer->nStart, (UINT)nDataSize);
	else
	{
		int n1 = poRingBuffer->nBufferSize - poRingBuffer->nStart;
		int n2 = nDataSize - n1;
		memcpy(lpData, poRingBuffer->lpBuffer + poRingBuffer->nStart, (UINT)n1);
		memcpy((char *)lpData + n1, poRingBuffer->lpBuffer, (UINT)n2);
	}

	poRingBuffer->nStart += nDataSize;
	poRingBuffer->nDataSize -= nDataSize;

	if (poRingBuffer->nStart >= poRingBuffer->nBufferSize)
		poRingBuffer->nStart -= poRingBuffer->nBufferSize;

	return RING_BUFFER_SUCCESS;
}

int RingBufferGetDataSize_DE(HANDLE hRingBuffer)
{
	RingBuffer_DE *poRingBuffer = (RingBuffer_DE *)hRingBuffer;

	if (poRingBuffer == NULL)
		return RING_BUFFER_ERR_ILLEGAL_PARAM;

	return poRingBuffer->nDataSize;
}

//same as DSpotter_AddSample()
int DEnergyCheck_AddSample(HANDLE hRingBuffer, SHORT* lpSample, INT nFrameSize)
{
	INT nBGEnergy = 0, nFullEnergy = 0;
	
	//Caculate Energy & Add to RingBuffer
	nBGEnergy = CalculateEnergy(lpSample, nFrameSize);
	//pop out data when full
	if(RingBufferIsFull_DE(hRingBuffer))
		RingBufferGetData_DE(hRingBuffer, (void*)&nFullEnergy, sizeof(INT));
	RingBufferPutData_DE(hRingBuffer, (void*)&nBGEnergy, sizeof(INT));
	
	return 0;
}

int DEnergyCheck_GetEnergyRatio(HANDLE hRingBuffer, INT nDura, INT nEndSil, INT nFrameSize)
{
	INT nBGEnergy = 0, nCmdEnergy = 0;
	//INT nDura, nEndSil, nNetLatency;
	INT pEnergyBuffer[300];
	INT nRecFrameSize = 0, nTotalFrameSize = 0, nWordDuraFrmaeSize;
	INT nTotalCmdRec = 0, nAverageEnergyDiff = 0;
	INT i,j;
	
	//Add Dynamic Energy Check
	nTotalFrameSize = RingBufferGetDataSize_DE(hRingBuffer);
	RingBufferGetData_DE(hRingBuffer, (void*)pEnergyBuffer, nTotalFrameSize);
	//put data back
	RingBufferPutData_DE(hRingBuffer, (void*)pEnergyBuffer, nTotalFrameSize);
	

	nRecFrameSize = (nDura + nEndSil) / nFrameSize;
	nWordDuraFrmaeSize = nDura / nFrameSize;
	nTotalFrameSize /= sizeof(INT);
	
	if(nRecFrameSize < nTotalFrameSize)
	{
		nBGEnergy = 0;
		for(i = (nTotalFrameSize - nRecFrameSize),j=0; j < 20; i--,j++)//about half sec background
		{
			if(i>=0)
				nBGEnergy += pEnergyBuffer[i];
			else
				nBGEnergy += pEnergyBuffer[0];
		}
		nBGEnergy /= 20;
	}
	else
	{
		nBGEnergy = 1;
	}

	nCmdEnergy = 0;
	for(i = (nTotalFrameSize - nRecFrameSize),j=0; j < nWordDuraFrmaeSize; i++,j++)
	{
		nCmdEnergy += pEnergyBuffer[i];
		//if(pEnergyBuffer[j] > nCmdEnergy)
		//	nCmdEnergy = pEnergyBuffer[j];
	}
	nCmdEnergy /= nWordDuraFrmaeSize;

	return (nCmdEnergy * 100 / nBGEnergy);
}


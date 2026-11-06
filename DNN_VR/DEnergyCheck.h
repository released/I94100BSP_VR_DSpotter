#ifndef _D_ENERGY_CHECK_H_
#define _D_ENERGY_CHECK_H_

#include "base_types.h"

#define ENERGY_CHECK_SUCCESS						(0)
#define ENERGY_CHECK_ERR_OUT_OF_MEMORY				(-1)
#define ENERGY_CHECK_ERR_ILLEGAL_PARAM				(-2)

//For store Energy
typedef struct tagRingBuffer_DE
{
	BYTE *lpBuffer;  // Memory buffer.
	int nBufferSize; // The size of lpBuffer.
	int nStart;      // The start index of data.
	int nEnd;        // The end index of data.
	int nDataSize;   // The size of data. If nEnd > nStart, nCount = nEnd - nStart;
	int bUseMalloc; // TRUE if lpBuffer us malloc(), FALSE if use outside memory.
	int bEnablePut; // Enable/Disable the put function.
	int bOverEnd;   // The enqueue data size is large than nBufferSize;
} RingBuffer_DE;

#ifdef __cplusplus
extern "C" {
#endif

/* Return the memory usage of Energy Check.
 * Return the memory usage in bytes current is 440 bytes*/
int DEnergyCheck_GetMemUsage();

/* Init.
 *   lpMem(in): The outside memory. Ring buffer will use malloc() if it is NULL.
 *   nMemSize: The memory size of lpMem.
 *   lphEnergyCheck(out): The handle of Energy Check.
 * Return the memory usage in bytes*/
int DEnergyCheck_Init(void *lpMem, int nMemSize, HANDLE *lphEnergyCheck);

/* Release the Energy Check.
 *   hEnergyCheck(in): The handle of Energy Check.
 * Return ENERGY_CHECK_SUCCESS or other error value. */
int DEnergyCheck_Release(HANDLE hEnergyCheck);

/* Add voice data for calculate Energy.
 *   hEnergyCheck(in): The handle of Energy Check.
 *	 lpSample(in): Audio data, same as DSpotter_AddSample.
 *   nFrameSize(in): frame size, should be 480
 * Return ENERGY_CHECK_SUCCESS or other error value. */
int DEnergyCheck_AddSample(HANDLE hEnergyCheck, SHORT* lpSample, INT nFrameSize);

/* Calculate background Energy and command energy difference.
 *   hEnergyCheck(in): The handle of Energy Check.
 *	 nDura(in): value from DSpotter_GetResultEPD() pnWordDura when DSpotter_AddSample return DSPOTTER_SUCCESS.
 *	 nEndSil(in): value from DSpotter_GetResultEPD() pnEndSil when DSpotter_AddSample return DSPOTTER_SUCCESS.
 *   nFrameSize(in): frame size, should be 480
 * Return Energy Difference. */
int DEnergyCheck_GetEnergyRatio(HANDLE hEnergyCheck, INT nDura, INT nEndSil, INT nFrameSize);


#ifdef __cplusplus
}
#endif

#endif

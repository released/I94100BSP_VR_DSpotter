#ifndef __DMA_H__
#define __DMA_H__
#include "c_stdfx.h"
#include "I94100.h"




extern void NVIC_DMA_Init(void);
extern void PDMA_IRQHandler(void);
extern void PDMA_Init(PDMA_CONFIG_T *pdma_ctl);

void DPWM_PDMA_Start(uint8_t si);

#endif

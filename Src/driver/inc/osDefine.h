#ifndef __osDefine_H__
#define __osDefine_H__

#include "stdint.h" 
#include "SysClk.h"
#include "SysTick.h"
#include "I94100.h"
//#include "hwh_uart.h"
//#include "hwh_gpio.h"
#include "pdma_config.h"
#include "DMA.h"
#include "peripheral.h"
#include "HIRC_AutoTrim.h"
#include "AudioClass.h"


#define ENABLE_OSDebug 1
#if ENABLE_OSDebug
#define OS_Printf printf
#else
#define OS_Printf
#endif

#endif

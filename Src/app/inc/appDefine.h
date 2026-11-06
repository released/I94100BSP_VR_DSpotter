#ifndef __appDefine_H__
#define __appDefine_H__

#include "usbd_audio.h"
#include "pdma_config.h"
#include "coder_88C22_88L21_Dmic_Dpwm_8315.h"
#include "appConsole.h"
#include "appAudioLine.h"

// Supported EVB:  
// 1. NT_I941  
// 2. NT_I941_AUD_AMIC
// 3. NT_I941_AUD_DMIC
// 4. CUSTOM (User-defined)
#define NT_I941
//#define NT_I941_AUD_AMIC

#if !defined(NT_I941) && !defined(NT_I941_AUD_AMIC) && !defined(NT_I941_AUD_DMIC)
   #if defined(CUSTOM)
    #warning "User must check all Custom part is correct"
   #else
	#error "Must set a board"
   #endif
#endif

#ifdef NT_I941
#define NAU88C22
#define MIC_PDMA_16BIT  0   // 0:88c22, 1:88L21, 2:DMIC
#define SPK_PDMA_16BIT  0   // 0:88c22
#endif
#ifdef NT_I941_AUD_AMIC
#define NAU88L21
#define MIC_PDMA_16BIT  1   // 0:88c22, 1:88L21, 2:DMIC
#define SPK_PDMA_16BIT  0   // 0:88c22
#endif
#ifdef NT_I941_AUD_DMIC
#define NAU88L21
#define MIC_PDMA_16BIT  2   // 0:88c22, 1:88L21, 2:DMIC
#define SPK_PDMA_16BIT  0   // 0:88c22

#endif


/***************************************/
//V0.00.01 完成初版，集成AUXIN\MIC通道选择
//V0.00.02 增加USB播放功能，播放存在异常
//V0.00.03 修复AUXIN选择BUG，修复USB播放问题
//V0.00.04 修复USB播放回采功能
//V0.00.05 修复USB录音通道反向问题
//V0.00.06 修复USB音频播放回录导致的音频超限（强制使用L声道，R声道存在问题）
//V0.00.07 修复USB音频播放回录左右声道音频大小问题
//V0.00.08 优化内存占用问题
//V0.00.09 加入DMIC
//V0.00.10 加入DPWM
//V0.00.11 优化PDMA，解决播放和录音存在的不同步导致噪音
//V0.00.12 解决DPWM播放始终FS不同步问题，在48M时FS需要为500
//V0.00.13 系统主时钟采用HXT 12.288M倍频为196.608M，USB采用内部48M，外设I2S0/DMIC/DPWM/I2S1 均采用PCLK 98.304M;
//         DMIC 的MCLK和PCLK DIV根据时钟源，重新计算
//V0.00.14 完成SPI2_I2S 替换UAC，但UAC功能只能录音，不能放音，用于监听调试
//V0.00.15 NAU88C22和NAU88L21 Codec register調適, 移除Colsonic相關檔案保留VR

/***************************************/
#define DemoVal			("V0.00.15")

/**************************************/



//外设音频输入输出选择
//#define SPK_PDMA_16BIT  0   // 0:88c22, 2:DPWM
//#define MIC_PDMA_16BIT  0   // 0:88c22, 1:88L21, 2:DMIC

#define DMIC_volume  4

#define Frame_Len 320

#endif

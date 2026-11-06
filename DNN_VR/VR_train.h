#include <stdint.h>
#include "I94100.h"

#define PCM_SAMPLE_NUM 480//(160 * 3)//(256)

#define KEYCODE_0							0x1000
#define KEYCODE_1							0x1001
#define KEYCODE_2							0x1002

//Not using flash but use RAM to train VoiceSD
#define VOICE_TAG_MODEL_SIZE     0x1000		// use 4KB in first attemp
__align(4) static BYTE g_byaSDModel[VOICE_TAG_MODEL_SIZE];     // The memory for store DSpotter SD trained model, 300 bytes header, 100~300 bytes for every voice tag.
//4K Flash Page Size for store SD model outside SDK
#define SDMODEL_ADDR					0x60000		// 0x4E000, 296KB, init addr in data flash
#define SDMODEL_SIZE					0x1000		// 16KB, SD model maximum size
//#define SDTRAIN_ADDR					SDMODEL_ADDR + SDMODEL_SIZE
#define SDTRAIN_BUFSIZE				0x4000		// 16KB, 1 sentences of 3 seconds
__align(4) static BYTE g_byaSDTrain[SDTRAIN_BUFSIZE];     // The memory for store DSpotter SD trained model, 300 bytes header, 100~300 bytes for every voice tag.


//extern uint32_t u32RecordAddr;
//extern int16_t ai16CyberonInputBuffer[PCM_SAMPLE_NUM];

//extern int32_t DoVR_sep(uint32_t lpbyCYBase, uint32_t lpbyModel, uint32_t nTimeOut, int (*funcCheckKeyPress)(void));
extern int32_t DoVR_multi(uint32_t lpbyBaseModel, uint32_t lppbyModel, uint32_t nNumModel, uint32_t nTimeOut, int (*funcCheckKeyPress)(void));

//extern int32_t DoVR_sep_MapID(uint32_t lpbyCYBase, uint32_t lpbyModel, uint32_t lpbyMapID, uint32_t nTimeOut, int (*funcCheckKeyPress)(void));
extern int32_t DoVR_multi_MapID(uint32_t lpbyBaseModel, uint32_t **lppbyModel, uint32_t lppbyMapID, uint32_t nNumModel, uint32_t nTimeOut, int (*funcCheckKeyPress)(void));

extern int32_t DoVR_train_MapID(uint32_t lpbyCYBase, uint32_t lpbyTrimap, uint32_t lpbySDModel, uint32_t nSDModelSize, uint32_t lpbySDBuffer, uint32_t nBufSize, int nMapID, uint32_t nTimeOut, int (*funcCheckKeyPress)(void));
extern int32_t DoVR_train_Offline(uint32_t lpbyCYBase, uint32_t lpbyTrimap, uint32_t lpbySDModel, uint32_t nSDModelSize, uint32_t lpbySDBuffer, uint32_t nBufSize, uint32_t lpsPCM, uint32_t nPCMSize);
extern int32_t DoVR_DeleteWord_MapID(uint32_t lpbyCYBase, uint32_t lpbyTrimap, uint32_t lpbySDModel, int32_t nMapID);

/********************************************************************************
*     main.c
*********************************************************************************/
#include "c_stdfx.h"
#include "osDefine.h"
#include "appDefine.h"
#include "VR.h"
#include "DSpotterSDKApi.h"
#include "AirconModels.h"
#include "AirconCommands.h"
#include "AirconVR.h"
#include "AirconFeedback.h"
#include "AirconAudio.h"
#include "SPIFlash.h"
#include "PlayNuOne.h"

#define DEBUG_UART

#ifdef DEBUG_UART
#define DBG_PRT   printf
#else
#define DBG_PRT(...)
#endif

#define DATAFLASH_ADDR AIRCON_MODEL_ADDRESS
#define DATAFLASH_BYTES (AIRCON_AUDIO_ADDRESS - DATAFLASH_ADDR)

/** add code start **/
static volatile AIRCON_LED_FEEDBACK decode_led;

/* LED state is also advanced by the timer; update it in one critical section. */
static void SetDecodeStage(uint32_t control_stage)
{
    uint32_t interrupt_mask;

    interrupt_mask = __get_PRIMASK();
    __disable_irq();
    PB0 = AirconFeedback_SetStage(&decode_led, control_stage);
    __set_PRIMASK(interrupt_mask);
}

/*
 * Feedback runs after recognition has released both SDK handles. The same
 * MapID selects a shared Mandarin reply for either recognition language.
 */
static void AcknowledgeDecode(int32_t map_id)
{
    uint32_t interrupt_mask;
    AIRCON_REPLY reply;
    extern void reset_pdma_spk(uint8_t ch);

    interrupt_mask = __get_PRIMASK();
    __disable_irq();
    PB0 = AirconFeedback_Start(&decode_led);
    __set_PRIMASK(interrupt_mask);
    if (!AirconAudio_Reply(map_id, &reply)) {
        DBG_PRT("[FEEDBACK_ERROR] reason=reply_map map_id=%ld\n", (long)map_id);
        return;
    }
    DBG_PRT("[AUDIO_REPLY] map_id=%ld count=%lu first=%u last=%u\n",
        (long)map_id, (unsigned long)reply.count,
        (unsigned int)reply.indexes[0],
        (unsigned int)reply.indexes[reply.count - 1U]);
    reset_pdma_spk(SPK_PDMA_CH);
    if (!PlayNuOneSequence(reply.indexes, reply.count))
        DBG_PRT("[FEEDBACK_ERROR] reason=audio_init map_id=%ld\n", (long)map_id);
}
/** add code end **/

void USBUAC_Init()
{
    UAC_Init();
    UAC_Start();
//    HIRC_AutoTrim_RefSof();
}

void NVIC_Init()
{
    NVIC_SetPriority(USBD_IRQn, 1);
    NVIC_SetPriority(PDMA_IRQn, 1);
}

void GPB_Init(void)
{
	//IO
    /* Configure PB.0 as Output mode for LED control */

	GPIO_SetMode(PB,BIT0|BIT1|BIT2,GPIO_MODE_OUTPUT);
//	SET_LED(PB,BIT0);
}

void UART0_Init(void)
{
    /* Enable UART module clock */
    CLK_EnableModuleClock(UART0_MODULE);

    /* Select UART module clock source as HXT and UART module clock divider as 1 */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));
    /* Reset UART module */
    SYS_ResetModule(UART0_RST);
    UART_Open(UART0, 115200);

    /* Enable PDMA clock source */
    CLK_EnableModuleClock(PDMA_MODULE);

    /* Set PB multi-function pins for UART0 RXD(PB.9) and TXD(PB.8) */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB8MFP_Msk | SYS_GPB_MFPH_PB9MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB8MFP_UART0_TXD | SYS_GPB_MFPH_PB9MFP_UART0_RXD);

//  UART0->FUNCSEL = UART_FUNCSEL_LINE;
//  UART0->ALTCTL |= UART_ALTCTL_LINRXEN_Msk;

    UART_ClearIntFlag(UART0,UART_INTSTS_BUFERRINT_Msk);
    UART_ClearIntFlag(UART0,UART_INTSTS_RLSINT_Msk);
    UART_ClearIntFlag(UART0,UART_INTSTS_MODEMINT_Msk);
    UART_ClearIntFlag(UART0,UART_INTSTS_RXTOINT_Msk);
    UART0->INTEN |=(1<<0);
    UART0->TOUT |= (0x02<<0);
//    NVIC_EnableIRQ(UART0_IRQn);

}
uart_type ap_uart0;
void uart_time_task(void)
{
    if(ap_uart0.uart_rx_time)
		ap_uart0.uart_rx_time--;
}

void time1_init(void)
{	uint8_t u8Lock;
	
	CLK_EnableModuleClock(TMR1_MODULE);
		// Enable HXT clock for TIMER0 and TIMER3.
	u8Lock = SYS_Unlock();
	CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
	CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
	SYS_Lock(u8Lock);
	
	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HXT, 0);
	TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);//
	TIMER_EnableInt(TIMER1);
	NVIC_EnableIRQ(TMR1_IRQn);
	TIMER_Start(TIMER1);
}
extern void time_cyek(void);
extern void LED_display_time(void);
void TMR1_IRQHandler(void)
{
   if(TIMER_GetIntFlag(TIMER1) == 1)
   {
      /* Clear Timer0 time-out interrupt flag */
      TIMER_ClearIntFlag(TIMER1);
			uart_time_task();	//UART		 
            PB0 = AirconFeedback_Tick1ms(&decode_led);
   }
}

/** add code start **/
static void PrintRecognitionDetails(const AIRCON_MODELS *models,
                                   uint32_t stage, int32_t outcome,
                                   const AIRCON_RECOGNITION *recognition)
{
#if AIRCON_RESULT_DIAGNOSTICS
    uint32_t i;
    const AIRCON_RESULT_DETAIL *detail;

    DBG_PRT("[VR_DECISION] stage=%lu outcome=%ld frames=%lu settle_frames=%lu filtered=%lu score_resolved=%lu\n",
        (unsigned long)stage, (long)outcome,
        (unsigned long)recognition->frames, (unsigned long)AIRCON_SETTLE_FRAMES,
        (unsigned long)recognition->filtered_count,
        (unsigned long)recognition->score_resolved);
    for (i = 0U; i < models->language_count; ++i) {
        detail = &recognition->candidates[i];
        DBG_PRT("[VR_CANDIDATE] language=%s matched=%u map_id=%ld index=%ld frame=%lu filtered=%lu",
            AirconCommands_Languages(1U << (models->languages[i].language - 1U)),
            detail->matched_frame != 0U ? 1U : 0U, (long)detail->map_id,
            (long)detail->word_index, (unsigned long)detail->matched_frame,
            (unsigned long)detail->filtered);
        DBG_PRT(" confi=%ld sg=%ld fil=%ld score_ret=%ld",
            (long)detail->confidence, (long)detail->sg_diff,
            (long)detail->fil, (long)detail->score_return);
        DBG_PRT(" dura=%ld end_sil=%ld latency=%ld epd_ret=%ld\n",
            (long)detail->word_duration, (long)detail->end_silence,
            (long)detail->network_latency, (long)detail->epd_return);
    }
#else
    (void)models;
    (void)stage;
    (void)outcome;
    (void)recognition;
#endif
}

/*
 * Application integration point: command describes the action and optional
 * temperature/timer value; wake_id identifies its room/living/global zone.
 * Implement the actual UART/GPIO/control protocol here as required.
 */
static void PrintAirconCommand(int32_t map_id, uint32_t languages, int32_t wake_id)
{
	AIRCON_COMMAND command;

	if (!AirconCommands_Decode(map_id, &command) || command.is_wake)
		return;
	DBG_PRT("[AIRCON] language=%s zone=%s map_id=%ld category=%s command=%s",
		AirconCommands_Languages(languages), AirconCommands_Zone(wake_id),
		(long)map_id, command.category, command.command);
	if (command.has_value)
		DBG_PRT(" value=%ld unit=%s", (long)command.value, command.unit);
	DBG_PRT("\n");
	/* Product GPIO and other air-conditioner interfaces may be added here. */
}
/** add code end **/

/*
 * Runtime ownership: main owns wake/control transitions and audio playback;
 * interrupt handlers provide PCM acquisition, DMA service and LED timing.
 * See README.md for the module and state flow charts.
 */
int main(void)
{
	AIRCON_MODELS models;
	AIRCON_AUDIO audio;
	AIRCON_RECOGNITION recognition;
	uint32_t stage;
	uint32_t memory_reported;
	uint32_t i;
	int32_t wake_id;
	int32_t nRet;
	
	SysClk_Initiate();

	NVIC_Init();

	AudioCodec_Init();    

	GPB_Init();
	SetDecodeStage(0U);
	UART0_Init();
	time1_init();
//	PB1=PB2=0;
	
	GPIO_SetMode(PB,BIT0,GPIO_MODE_OUTPUT);
	
	
	/* Validate both Flash regions before starting microphone recognition. */
	nRet = AirconModels_Load((const uint8_t *)DATAFLASH_ADDR, DATAFLASH_BYTES, &models);
	if (nRet != 0) {
		DBG_PRT("[MODEL_ERROR] code=%ld address=0x%08lx; program AirconDataFlash.bin at 0x00015000\n",
			(long)nRet, (unsigned long)DATAFLASH_ADDR);
		goto L_ERROR;
	}
	nRet = AirconAudio_Load((const uint8_t *)AIRCON_AUDIO_ADDRESS,
		AIRCON_AUDIO_CAPACITY, &audio);
	if (nRet != 0) {
		DBG_PRT("[AUDIO_ERROR] code=%ld address=0x%08lx; program AirconDataFlash.bin at 0x00015000\n",
			(long)nRet, (unsigned long)AIRCON_AUDIO_ADDRESS);
		goto L_ERROR;
	}
	PlayNuOneSetAudioSize(audio.rom_bytes);
	g_sApp.Mic_NuOne.sNuOneApp.u32TotalAudioNum = audio.audio_count;
	DBG_PRT("[VR_SDK] %s\n", DSpotter_Ver());
	DBG_PRT("[APP] release=V005 feedback=full_reply_led led=PB0 audio=internal\n");
	DBG_PRT("[VR_DIAG] result_details=%u settle_frames=%lu\n",
		(unsigned int)AIRCON_RESULT_DIAGNOSTICS, (unsigned long)AIRCON_SETTLE_FRAMES);
	DBG_PRT("[VR_SCORE] control_filter=%u min_confi=%ld min_sg=%ld arbitration=%u winner_min=%ld confi_gap=%ld sg_gap=%ld\n",
		(unsigned int)AIRCON_CONTROL_SCORE_FILTER,
		(long)AIRCON_CONTROL_MIN_CONFIDENCE, (long)AIRCON_CONTROL_MIN_SG_DIFF,
		(unsigned int)AIRCON_CONFLICT_SCORE_ARBITRATION,
		(long)AIRCON_CONFLICT_MIN_CONFIDENCE,
		(long)AIRCON_CONFLICT_CONFIDENCE_GAP, (long)AIRCON_CONFLICT_SG_GAP);
	DBG_PRT("[AUDIO_READY] address=0x%08lx rom_bytes=%lu clips=%lu bps=1.0\n",
		(unsigned long)AIRCON_AUDIO_ADDRESS, (unsigned long)audio.rom_bytes,
		(unsigned long)audio.audio_count);
	DBG_PRT("[VR_CONFIG] control_timeout_s=%lu control_timeout_frames=%lu\n",
		(unsigned long)AIRCON_CONTROL_TIMEOUT_SECONDS,
		(unsigned long)AIRCON_CONTROL_TIMEOUT_FRAMES);
	for (i = 0; i < models.language_count; ++i) {
		DBG_PRT("[MODEL] language=%s groups=%lu controls=%lu\n",
			AirconCommands_Languages(1U << (models.languages[i].language - 1U)),
			(unsigned long)models.languages[i].group_count,
			(unsigned long)(models.languages[i].group_count - 1U));
	}
	
	{
		SYS_SET_TRIMHIRC_LOOPSEL(SYS_IRCTCTL_LOOPSEL_4);
		SYS_SET_TRIMHIRC_RETRYCNT(SYS_IRCTCTL_RETRYCNT_64);
		SYS_ENABLE_TRIMHIRC_CLKERRSTOP();
		SYS_SET_TRIMHIRC_REFCLK(SYS_IRCTCTL_REFCLK_USBSOF);
	}

	Cyberon_VR.ai16cy_fifi_inder =0;
	
	USBUAC_Init();		//	USB Device
	
	Systick_Init();		//	系统时钟中断必须在USBUAC后进行初始化
	stage = AIRCON_STAGE_WAKE;
	wake_id = 0;
	memory_reported = 0U;
	DBG_PRT("[VR_READY] languages=%lu pool=%lu uart=115200\n",
		(unsigned long)models.language_count, (unsigned long)VR_MEMORY_POOL_SIZE);

	while(1)
	{
		/* Playback and recognition alternate to avoid recognizing our reply. */
		if(g_sApp.Mic_NuOne.sNuOneApp.bPCMPlaying==0)
		{
			MIC_Start();

			nRet = AirconVR_Recognize(&models, stage,
				stage == AIRCON_STAGE_WAKE ? 0U : AIRCON_CONTROL_TIMEOUT_FRAMES, &recognition);
			PrintRecognitionDetails(&models, stage, nRet, &recognition);
			if ((memory_reported & (1U << stage)) == 0U || nRet < 0) {
				DBG_PRT("[VR_MEMORY] stage=%lu total=%lu available=%lu",
					(unsigned long)stage, (unsigned long)recognition.memory_total,
					(unsigned long)VR_MEMORY_POOL_SIZE);
				for (i = 0; i < models.language_count; ++i) {
					DBG_PRT(" %s=%lu",
						AirconCommands_Languages(1U << (models.languages[i].language - 1U)),
						(unsigned long)recognition.memory_required[i]);
				}
				DBG_PRT("\n");
				memory_reported |= 1U << stage;
			}
			/* Recoverable stream/conflict errors rearm wake; other errors stop. */
			if (nRet < 0) {
				DBG_PRT("[VR_ERROR] code=%ld frames=%lu drops=%lu\n", (long)nRet,
					(unsigned long)recognition.frames, (unsigned long)recognition.audio_drops);
				if (nRet != AIRCON_ERR_CONFLICT && nRet != AIRCON_ERR_AUDIO_DROP)
					goto L_ERROR;
				stage = AIRCON_STAGE_WAKE;
				wake_id = 0;
				SetDecodeStage(0U);
				continue;
			}
			/* A control timeout also clears the previously selected zone. */
			if (nRet == 0) {
				DBG_PRT("[VR_TIMEOUT]\n");
				stage = AIRCON_STAGE_WAKE;
				wake_id = 0;
				SetDecodeStage(0U);
				continue;
			}
			/* A successful control retains the wake zone for the next command. */
			if (stage == AIRCON_STAGE_WAKE) {
				wake_id = nRet;
				stage = AIRCON_STAGE_CONTROL;
				SetDecodeStage(1U);
				DBG_PRT("[WAKE] language=%s zone=%s map_id=%ld\n",
					AirconCommands_Languages(recognition.language_mask),
					AirconCommands_Zone(wake_id), (long)wake_id);
			} else {
				PrintAirconCommand(nRet, recognition.language_mask, wake_id);
			}
			/* The next control window begins after this whole reply finishes. */
			AcknowledgeDecode(nRet);
		}
		else if(g_sApp.Mic_NuOne.sNuOneApp.bPCMPlaying)					//Playing message handling
		{
				PlayLoop();
		}
	}
	
	
L_ERROR:
	SetDecodeStage(0U);
	DBG_PRT("[VR_STOPPED]\n");
    while(1);
}

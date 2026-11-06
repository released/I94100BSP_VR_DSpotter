#ifndef AIRCON_VR_H
#define AIRCON_VR_H

#include "AirconModels.h"

#define AIRCON_STAGE_WAKE 0U
#define AIRCON_STAGE_CONTROL 1U
/* Three further PCM frames allow the other engine to agree or disagree. */
#define AIRCON_SETTLE_FRAMES 3U

/* Control-stage quality gates. Wake-stage thresholds stay in the native models. */
#ifndef AIRCON_CONTROL_SCORE_FILTER
#define AIRCON_CONTROL_SCORE_FILTER 1U
#endif
#ifndef AIRCON_CONTROL_MIN_CONFIDENCE
#define AIRCON_CONTROL_MIN_CONFIDENCE 5
#endif
#ifndef AIRCON_CONTROL_MIN_SG_DIFF
#define AIRCON_CONTROL_MIN_SG_DIFF 15
#endif

/* Resolve different MapIDs only when one candidate clearly leads both scores. */
#ifndef AIRCON_CONFLICT_SCORE_ARBITRATION
#define AIRCON_CONFLICT_SCORE_ARBITRATION 1U
#endif
#ifndef AIRCON_CONFLICT_MIN_CONFIDENCE
#define AIRCON_CONFLICT_MIN_CONFIDENCE 25
#endif
#ifndef AIRCON_CONFLICT_CONFIDENCE_GAP
#define AIRCON_CONFLICT_CONFIDENCE_GAP 20
#endif
#ifndef AIRCON_CONFLICT_SG_GAP
#define AIRCON_CONFLICT_SG_GAP 10
#endif

#if AIRCON_CONTROL_MIN_CONFIDENCE < 0 || AIRCON_CONTROL_MIN_SG_DIFF < 0 \
    || AIRCON_CONFLICT_MIN_CONFIDENCE < 0 \
    || AIRCON_CONFLICT_CONFIDENCE_GAP < 1 || AIRCON_CONFLICT_SG_GAP < 1
#error Invalid control score thresholds
#endif

/* Capture SDK result details for UART output after recognition has stopped. */
#ifndef AIRCON_RESULT_DIAGNOSTICS
#define AIRCON_RESULT_DIAGNOSTICS 0U
#endif

/* User setting: seconds without a successful control command before rearming wake. */
#ifndef AIRCON_CONTROL_TIMEOUT_SECONDS
#define AIRCON_CONTROL_TIMEOUT_SECONDS 60U
#endif

/* Each recognition frame contains 480 samples at 16000 Hz. */
#define AIRCON_PCM_FRAME_MS 30U
#define AIRCON_CONTROL_TIMEOUT_FRAMES \
    ((AIRCON_CONTROL_TIMEOUT_SECONDS * 1000UL + AIRCON_PCM_FRAME_MS - 1U) \
     / AIRCON_PCM_FRAME_MS)

#if AIRCON_CONTROL_TIMEOUT_SECONDS < 1 || AIRCON_CONTROL_TIMEOUT_SECONDS > 4294967UL
#error AIRCON_CONTROL_TIMEOUT_SECONDS must fit a positive 32-bit millisecond interval
#endif

/* Per-engine evidence; score/EPD fields are populated by enabled features. */
typedef struct {
    int32_t map_id;
    int32_t word_index;
    int32_t confidence;
    int32_t sg_diff;
    int32_t fil;
    int32_t score_return;
    int32_t word_duration;
    int32_t end_silence;
    int32_t network_latency;
    int32_t epd_return;
    uint32_t matched_frame;
    uint32_t filtered;
} AIRCON_RESULT_DETAIL;

/* One call's outcome and memory usage; language_mask bit 0=ZHTW, bit 1=TW. */
typedef struct {
    int32_t map_id;
    uint32_t language_mask;
    uint32_t memory_required[AIRCON_MAX_LANGUAGES];
    uint32_t memory_total;
    uint32_t frames;
    uint32_t audio_drops;
    uint32_t filtered_count;
    uint32_t score_resolved;
    AIRCON_RESULT_DETAIL candidates[AIRCON_MAX_LANGUAGES];
} AIRCON_RECOGNITION;

/* timeout_frames=0 waits indefinitely; the control default is defined above. */
int32_t AirconVR_Recognize(const AIRCON_MODELS *models, uint32_t stage,
                          uint32_t timeout_frames, AIRCON_RECOGNITION *result);

#endif

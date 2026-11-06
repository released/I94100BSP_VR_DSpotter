#ifndef AIRCON_VR_H
#define AIRCON_VR_H

#include "AirconModels.h"

#define AIRCON_STAGE_WAKE 0U
#define AIRCON_STAGE_CONTROL 1U
#define AIRCON_SETTLE_FRAMES 3U

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

typedef struct {
    int32_t map_id;
    uint32_t language_mask;
    uint32_t memory_required[AIRCON_MAX_LANGUAGES];
    uint32_t memory_total;
    uint32_t frames;
    uint32_t audio_drops;
} AIRCON_RECOGNITION;

int32_t AirconVR_Recognize(const AIRCON_MODELS *models, uint32_t stage,
                          uint32_t timeout_frames, AIRCON_RECOGNITION *result);

#endif

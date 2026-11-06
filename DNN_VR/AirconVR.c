#include <stddef.h>
#include <string.h>
#include "DSpotterSDKApi.h"
#include "VR.h"
#include "AirconVR.h"
#include "AirconCommands.h"

extern BYTE lpbyMemPool[VR_MEMORY_POOL_SIZE];
extern BYTE lpbyState[VR_STATE_SIZE];

static union {
    uint32_t alignment;
    BYTE bytes[VR_STATE_SIZE];
} second_state;

static union {
    uint32_t alignment;
    SHORT samples[Audio_Frame_Len * 3];
} pcm_frame;

int32_t AirconVR_Recognize(const AIRCON_MODELS *models, uint32_t stage,
                          uint32_t timeout_frames, AIRCON_RECOGNITION *result)
{
    HANDLE handles[AIRCON_MAX_LANGUAGES];
    BYTE *selected[AIRCON_MAX_LANGUAGES][AIRCON_MAX_GROUPS];
    INT selected_count[AIRCON_MAX_LANGUAGES];
    uint32_t allocations[AIRCON_MAX_LANGUAGES];
    uint32_t matched[AIRCON_MAX_LANGUAGES];
    uint32_t i;
    uint32_t j;
    uint32_t offset;
    uint32_t drop_start;
    uint32_t settle;
    uint32_t had_candidate;
    uint32_t all_matched;
    INT needed;
    INT sdk_error;
    INT map_id;
    INT sdk_result;
    int32_t outcome;
    int32_t frame_result;
    BYTE *preserve;
    AIRCON_COMMAND command;

    if (result == NULL)
        return AIRCON_ERR_BUNDLE;
    memset(result, 0, sizeof(*result));
    memset(handles, 0, sizeof(handles));
    memset(matched, 0, sizeof(matched));
    if (models == NULL || models->language_count == 0U
        || models->language_count > AIRCON_MAX_LANGUAGES
        || stage > AIRCON_STAGE_CONTROL)
        return AIRCON_ERR_BUNDLE;
    for (i = 0; i < models->language_count; ++i) {
        if (models->languages[i].group_count < 2U
            || models->languages[i].group_count > AIRCON_MAX_GROUPS
            || models->languages[i].base == NULL
            || models->languages[i].language < 1U
            || models->languages[i].language > 2U)
            return AIRCON_ERR_PACK;
        selected_count[i] = 0;
        for (j = 0; j < models->languages[i].group_count; ++j) {
            if ((stage == AIRCON_STAGE_WAKE && j != 0U)
                || (stage == AIRCON_STAGE_CONTROL && j == 0U))
                continue;
            selected[i][selected_count[i]] = (BYTE *)models->languages[i].groups[j];
            if (selected[i][selected_count[i]] == NULL
                || DSpotter_GetNumWord(selected[i][selected_count[i]]) <= 0)
                return AIRCON_ERR_PACK;
            ++selected_count[i];
        }
        needed = DSpotter_GetMemoryUsage_Multi((BYTE *)models->languages[i].base,
                    selected[i], selected_count[i], 300);
        if (needed <= 0)
            return needed < 0 ? needed : AIRCON_ERR_MEMORY;
        result->memory_required[i] = (uint32_t)needed;
        allocations[i] = ((uint32_t)needed + 3U) & ~3U;
        if (allocations[i] > 0xFFFFFFFFUL - result->memory_total) {
            result->memory_total = 0xFFFFFFFFUL;
            return AIRCON_ERR_MEMORY;
        }
        result->memory_total += allocations[i];
    }
    if (result->memory_total > VR_MEMORY_POOL_SIZE)
        return AIRCON_ERR_MEMORY;
    offset = 0U;
    outcome = 0;
    for (i = 0; i < models->language_count; ++i) {
        preserve = i == 0U ? lpbyState : second_state.bytes;
        sdk_error = DSPOTTER_SUCCESS;
        handles[i] = DSpotter_Init_Multi((BYTE *)models->languages[i].base,
                     selected[i], selected_count[i], 300, lpbyMemPool + offset,
                     (INT)allocations[i], preserve, VR_STATE_SIZE, &sdk_error);
        if (handles[i] == NULL) {
            outcome = sdk_error < 0 ? sdk_error : AIRCON_ERR_MEMORY;
            goto cleanup;
        }
        offset += allocations[i];
        sdk_error = DSpotter_Reset(handles[i]);
        if (sdk_error != DSPOTTER_SUCCESS) {
            outcome = sdk_error < 0 ? sdk_error : AIRCON_ERR_PACK;
            goto cleanup;
        }
    }
    VR_FlushFrames();
    drop_start = VR_GetFrameOverruns();
    settle = AIRCON_SETTLE_FRAMES;
    for (;;) {
        frame_result = VR_TakeFrame(pcm_frame.samples, Audio_Frame_Len * 3U);
        if (frame_result < 0) {
            outcome = frame_result;
            break;
        }
        if (frame_result == 0)
            continue;
        result->audio_drops = VR_GetFrameOverruns() - drop_start;
        if (result->audio_drops != 0U) {
            outcome = AIRCON_ERR_AUDIO_DROP;
            break;
        }
        ++result->frames;
        had_candidate = result->map_id != 0;
        for (i = 0; i < models->language_count; ++i) {
            if (matched[i] != 0U)
                continue;
            sdk_result = DSpotter_AddSample(handles[i], pcm_frame.samples,
                                            Audio_Frame_Len * 3);
            if (sdk_result == DSPOTTER_ERR_NeedMoreSample)
                continue;
            if (sdk_result == DSPOTTER_ERR_Rejected
                || sdk_result == DSPOTTER_ERR_NoVoiceDetect) {
                sdk_error = DSpotter_Reset(handles[i]);
                if (sdk_error == DSPOTTER_SUCCESS)
                    continue;
                outcome = sdk_error < 0 ? sdk_error : AIRCON_ERR_PACK;
                goto cleanup;
            }
            if (sdk_result != DSPOTTER_SUCCESS) {
                outcome = sdk_result < 0 ? sdk_result : AIRCON_ERR_PACK;
                goto cleanup;
            }
            map_id = DSpotter_GetResultMapID(handles[i]);
            if (!AirconCommands_Decode(map_id, &command)
                || (stage == AIRCON_STAGE_WAKE && !command.is_wake)
                || (stage == AIRCON_STAGE_CONTROL && command.is_wake)) {
                outcome = AIRCON_ERR_MAP_ID;
                goto cleanup;
            }
            result->language_mask |= 1U << (models->languages[i].language - 1U);
            matched[i] = 1U;
            if (result->map_id != 0 && result->map_id != map_id) {
                result->map_id = 0;
                outcome = AIRCON_ERR_CONFLICT;
                goto cleanup;
            }
            result->map_id = map_id;
        }
        result->audio_drops = VR_GetFrameOverruns() - drop_start;
        if (result->audio_drops != 0U) {
            outcome = AIRCON_ERR_AUDIO_DROP;
            break;
        }
        all_matched = 1U;
        for (i = 0; i < models->language_count; ++i) {
            if (matched[i] == 0U)
                all_matched = 0U;
        }
        if (result->map_id != 0) {
            if (had_candidate && settle > 0U)
                --settle;
            if (all_matched || settle == 0U) {
                outcome = result->map_id;
                break;
            }
        } else if (timeout_frames > 0U && result->frames >= timeout_frames) {
            break;
        }
    }

cleanup:
    for (i = 0; i < models->language_count; ++i) {
        if (handles[i] != NULL)
            DSpotter_Release(handles[i]);
    }
    return outcome;
}

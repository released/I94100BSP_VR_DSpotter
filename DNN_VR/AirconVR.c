#include <stddef.h>
#include <string.h>
#include "DSpotterSDKApi.h"
#include "VR.h"
#include "AirconVR.h"
#include "AirconCommands.h"

extern BYTE lpbyMemPool[VR_MEMORY_POOL_SIZE];
extern BYTE lpbyState[VR_STATE_SIZE];

/* The engines share the PCM input but require separate persistent SDK states. */
static union {
    uint32_t alignment;
    BYTE bytes[VR_STATE_SIZE];
} second_state;

/* Copy one producer frame before feeding it to either language engine. */
static union {
    uint32_t alignment;
    SHORT samples[Audio_Frame_Len * 3];
} pcm_frame;

#if AIRCON_CONFLICT_SCORE_ARBITRATION
/* Accept a disagreement only when both confidence and SG have a clear lead. */
static int ClearlyStronger(const AIRCON_RESULT_DETAIL *winner,
                           const AIRCON_RESULT_DETAIL *other)
{
    if (winner->score_return < 0 || other->score_return < 0
        || winner->confidence < AIRCON_CONFLICT_MIN_CONFIDENCE
        || winner->confidence < AIRCON_CONFLICT_CONFIDENCE_GAP
        || winner->sg_diff < AIRCON_CONTROL_MIN_SG_DIFF
        || winner->sg_diff < AIRCON_CONFLICT_SG_GAP)
        return 0;
    return other->confidence
                <= winner->confidence - AIRCON_CONFLICT_CONFIDENCE_GAP
        && other->sg_diff <= winner->sg_diff - AIRCON_CONFLICT_SG_GAP;
}
#endif

/*
 * Run one wake/control window. Both SDK engines receive the same PCM frames;
 * no spoken language selector or sequential language retry is required.
 * Positive return = accepted MapID, zero = timeout, negative = SDK/app error.
 * All SDK handles are released before returning, including every error path.
 * This loop performs recognition only; UART, LED and audio run in main().
 */
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
#if AIRCON_RESULT_DIAGNOSTICS || AIRCON_CONTROL_SCORE_FILTER || AIRCON_CONFLICT_SCORE_ARBITRATION
    INT confidence;
    INT sg_diff;
    INT fil;
    AIRCON_RESULT_DETAIL *detail;
#endif
#if AIRCON_RESULT_DIAGNOSTICS
    INT word_duration;
    INT end_silence;
    INT network_latency;
#endif

    if (result == NULL)
        return AIRCON_ERR_BUNDLE;
    memset(result, 0, sizeof(*result));
    memset(handles, 0, sizeof(handles));
    memset(matched, 0, sizeof(matched));
    if (models == NULL || models->language_count == 0U
        || models->language_count > AIRCON_MAX_LANGUAGES
        || stage > AIRCON_STAGE_CONTROL)
        return AIRCON_ERR_BUNDLE;
    /* Group 0 is wake; control activates all remaining category groups. */
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
        /* Budget every engine before assigning aligned slices of one pool. */
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
    /* A failed second initialization still releases the first handle below. */
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
    /* Pending microphone data can contain the previous reply or old stage. */
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
        /* Count consumed 30 ms PCM frames, not empty polling iterations. */
        ++result->frames;
        had_candidate = result->map_id != 0;
        for (i = 0; i < models->language_count; ++i) {
            if (matched[i] != 0U)
                continue;
            sdk_result = DSpotter_AddSample(handles[i], pcm_frame.samples,
                                            Audio_Frame_Len * 3);
            if (sdk_result == DSPOTTER_ERR_NeedMoreSample)
                continue;
            /* Ordinary rejection rearms only this engine in the same window. */
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
#if AIRCON_RESULT_DIAGNOSTICS || AIRCON_CONTROL_SCORE_FILTER || AIRCON_CONFLICT_SCORE_ARBITRATION
            detail = &result->candidates[i];
            detail->map_id = map_id;
            detail->matched_frame = result->frames;
            detail->filtered = 0U;
            /* Query the SDK result before its score; preserve that call order. */
            detail->word_index = DSpotter_GetResult(handles[i]);
            confidence = sg_diff = fil = 0;
            detail->score_return = DSpotter_GetResultScore(handles[i],
                                        &confidence, &sg_diff, &fil);
            detail->confidence = confidence;
            detail->sg_diff = sg_diff;
            detail->fil = fil;
#endif
#if AIRCON_RESULT_DIAGNOSTICS
            word_duration = end_silence = network_latency = 0;
            detail->epd_return = DSpotter_GetResultEPD(handles[i],
                                &word_duration, &end_silence, &network_latency);
            detail->word_duration = word_duration;
            detail->end_silence = end_silence;
            detail->network_latency = network_latency;
#endif
            if (!AirconCommands_Decode(map_id, &command)
                || (stage == AIRCON_STAGE_WAKE && !command.is_wake)
                || (stage == AIRCON_STAGE_CONTROL && command.is_wake)) {
                outcome = AIRCON_ERR_MAP_ID;
                goto cleanup;
            }
#if AIRCON_CONTROL_SCORE_FILTER
            /* A weak control candidate must not restart the 60-second window. */
            if (stage == AIRCON_STAGE_CONTROL
                && (detail->score_return < 0
                    || detail->confidence < AIRCON_CONTROL_MIN_CONFIDENCE
                    || detail->sg_diff < AIRCON_CONTROL_MIN_SG_DIFF)) {
                detail->filtered = 1U;
                ++result->filtered_count;
                sdk_error = DSpotter_Reset(handles[i]);
                if (sdk_error != DSPOTTER_SUCCESS) {
                    outcome = sdk_error < 0 ? sdk_error : AIRCON_ERR_PACK;
                    goto cleanup;
                }
                continue;
            }
#endif
            /* Freeze an accepted engine while the other finishes its vote. */
            result->language_mask |= 1U << (models->languages[i].language - 1U);
            matched[i] = 1U;
            if (result->map_id != 0 && result->map_id != map_id) {
#if AIRCON_CONFLICT_SCORE_ARBITRATION
                for (j = 0U; j < models->language_count; ++j) {
                    if (j != i && matched[j] != 0U)
                        break;
                }
                if (stage == AIRCON_STAGE_CONTROL && j < models->language_count) {
                    if (ClearlyStronger(&result->candidates[i], &result->candidates[j])) {
                        result->map_id = map_id;
                        result->language_mask = 1U << (models->languages[i].language - 1U);
                        result->score_resolved = 1U;
                        continue;
                    }
                    if (ClearlyStronger(&result->candidates[j], &result->candidates[i])) {
                        result->language_mask = 1U << (models->languages[j].language - 1U);
                        result->score_resolved = 1U;
                        continue;
                    }
                }
#endif
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
        /* Allow up to AIRCON_SETTLE_FRAMES after the first accepted candidate. */
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
    /* main() starts playback only after both handles have released the pool. */
    for (i = 0; i < models->language_count; ++i) {
        if (handles[i] != NULL)
            DSpotter_Release(handles[i]);
    }
    return outcome;
}

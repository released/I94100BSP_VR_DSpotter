#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DSpotterSDKApi.h"
#include "VR.h"
#include "AirconModels.h"
#include "AirconCommands.h"
#include "AirconVR.h"
#include "AirconFeedback.h"

static unsigned int checks;
#define CHECK(condition) do { ++checks; if (!(condition)) return __LINE__; } while (0)

BYTE lpbyMemPool[VR_MEMORY_POOL_SIZE];
BYTE lpbyState[VR_STATE_SIZE];

typedef struct {
    unsigned int index;
    unsigned int received;
} MOCK_ENGINE;

static MOCK_ENGINE engines[2];
static BYTE bases[2][4] = {{1, 0, 0, 0}, {2, 0, 0, 0}};
static BYTE words[2][3][4] = {{{1}}, {{1}}};
static INT required[2];
static uint32_t fire_at[2];
static INT map_ids[2];
static INT confidence_scores[2];
static INT sg_scores[2];
static uint32_t frame_number;
static uint32_t frame_limit;
static uint32_t drops;
static uint32_t drop_at;
static uint32_t drop_in_sdk;
static uint32_t init_fail;
static uint32_t fatal_sdk;
static uint32_t reject_once;
static INT diagnostics_error;
static uint32_t init_calls;
static uint32_t release_calls;
static uint32_t reset_calls;
static uint32_t reset_fail_at;
static uint32_t selected_counts[2];
static BYTE *memory_addresses[2];
static BYTE *preserve_addresses[2];
static int mock_failure;

static void Setup(AIRCON_MODELS *models)
{
    uint32_t i;
    uint32_t j;

    memset(models, 0, sizeof(*models));
    memset(engines, 0, sizeof(engines));
    memset(lpbyMemPool, 0xA5, sizeof(lpbyMemPool));
    frame_number = drops = drop_at = drop_in_sdk = init_fail = fatal_sdk = reject_once = 0U;
    diagnostics_error = 0;
    frame_limit = 30U;
    init_calls = release_calls = reset_calls = 0U;
    reset_fail_at = 0U;
    mock_failure = 0;
    models->language_count = 2U;
    for (i = 0; i < 2U; ++i) {
        engines[i].index = i;
        required[i] = 10001 + (INT)i;
        fire_at[i] = 0U;
        map_ids[i] = 128;
        confidence_scores[i] = 250 + (INT)i;
        sg_scores[i] = 40 + (INT)i;
        models->languages[i].language = i + 1U;
        models->languages[i].base = bases[i];
        models->languages[i].group_count = 3U;
        for (j = 0; j < 3U; ++j) {
            words[i][j][0] = (BYTE)(j + 1U);
            models->languages[i].groups[j] = words[i][j];
        }
    }
}

INT DSpotter_GetNumWord(BYTE *model)
{
    return model != NULL && model[0] != 0U ? 1 : 0;
}

INT DSpotter_GetMemoryUsage_Multi(BYTE *base, BYTE *models[], INT count, INT max_time)
{
    uint32_t index;

    index = base[0] - 1U;
    selected_counts[index] = (uint32_t)count;
    if (models == NULL || max_time != 300)
        mock_failure = __LINE__;
    return required[index];
}

HANDLE DSpotter_Init_Multi(BYTE *base, BYTE *models[], INT count, INT max_time,
                          BYTE *memory, INT size, BYTE *preserve, INT preserve_size, INT *error)
{
    uint32_t index;

    index = base[0] - 1U;
    ++init_calls;
    if (models == NULL || count < 1 || max_time != 300 || preserve_size != VR_STATE_SIZE
        || size < required[index])
        mock_failure = __LINE__;
    memory_addresses[index] = memory;
    preserve_addresses[index] = preserve;
    if (init_fail == index + 1U) {
        *error = DSPOTTER_ERR_LoadModelFailed;
        return NULL;
    }
    *error = DSPOTTER_SUCCESS;
    return &engines[index];
}

INT DSpotter_Reset(HANDLE handle)
{
    if (handle == NULL)
        mock_failure = __LINE__;
    ++reset_calls;
    if (reset_fail_at != 0U && reset_calls == reset_fail_at)
        return DSPOTTER_ERR_LoadModelFailed;
    return DSPOTTER_SUCCESS;
}

INT DSpotter_Release(HANDLE handle)
{
    if (handle == NULL)
        mock_failure = __LINE__;
    ++release_calls;
    return DSPOTTER_SUCCESS;
}

INT DSpotter_AddSample(HANDLE handle, SHORT *samples, INT count)
{
    MOCK_ENGINE *engine;
    INT i;

    engine = (MOCK_ENGINE *)handle;
    ++engine->received;
    if (count != Audio_Frame_Len * 3)
        mock_failure = __LINE__;
    for (i = 0; i < count; ++i) {
        if (samples[i] != (SHORT)(frame_number + (uint32_t)i))
            mock_failure = __LINE__;
    }
    if (drop_in_sdk && frame_number == 1U && engine->index == 0U)
        ++drops;
    if (fatal_sdk && engine->index == 0U)
        return DSPOTTER_ERR_LoadModelFailed;
    if (reject_once && frame_number == 1U)
        return DSPOTTER_ERR_Rejected;
    if (fire_at[engine->index] == frame_number)
        return DSPOTTER_SUCCESS;
    return DSPOTTER_ERR_NeedMoreSample;
}

INT DSpotter_GetResultMapID(HANDLE handle)
{
    return map_ids[((MOCK_ENGINE *)handle)->index];
}

INT DSpotter_GetResult(HANDLE handle)
{
    return 100 + (INT)((MOCK_ENGINE *)handle)->index;
}

INT DSpotter_GetResultScore(HANDLE handle, INT *confidence, INT *sg_diff, INT *fil)
{
    INT index;

    if (diagnostics_error)
        return -9001;
    index = (INT)((MOCK_ENGINE *)handle)->index;
    *confidence = confidence_scores[index];
    *sg_diff = sg_scores[index];
    *fil = -2485;
    return *confidence;
}

INT DSpotter_GetResultEPD(HANDLE handle, INT *duration, INT *end_silence, INT *latency)
{
    if (diagnostics_error)
        return -9002;
    *duration = 40 + (INT)((MOCK_ENGINE *)handle)->index;
    *end_silence = 8;
    *latency = 2;
    return 0;
}

int32_t VR_TakeFrame(int16_t samples[], uint32_t count)
{
    uint32_t i;

    ++frame_number;
    if (frame_number > frame_limit)
        return AIRCON_ERR_AUDIO_DROP;
    if (drop_at == frame_number)
        ++drops;
    for (i = 0; i < count; ++i)
        samples[i] = (SHORT)(frame_number + i);
    return 1;
}

void VR_FlushFrames(void) {}
uint32_t VR_GetFrameOverruns(void) { return drops; }

static void Put32(BYTE *data, uint32_t value)
{
    data[0] = (BYTE)value;
    data[1] = (BYTE)(value >> 8);
    data[2] = (BYTE)(value >> 16);
    data[3] = (BYTE)(value >> 24);
}

static void MakeBundle(BYTE data[104])
{
    uint32_t i;
    uint32_t offset;

    memset(data, 0, 104U);
    Put32(data, AIRCON_BUNDLE_MAGIC);
    Put32(data + 4, 1U);
    Put32(data + 8, 104U);
    Put32(data + 12, 2U);
    for (i = 0; i < 2U; ++i) {
        offset = 48U + i * 28U;
        Put32(data + 16U + i * 16U, i + 1U);
        Put32(data + 20U + i * 16U, offset);
        Put32(data + 24U + i * 16U, 28U);
        Put32(data + offset, 3U);
        Put32(data + offset + 4U, 4U);
        Put32(data + offset + 8U, 4U);
        Put32(data + offset + 12U, 4U);
        memset(data + offset + 16U, (int)i + 1, 12U);
        Put32(data + 28U + i * 16U, AirconModels_Crc32(data + offset, 28U));
    }
}

static int TestModels(void)
{
    BYTE data[104];
    AIRCON_MODELS models;

    CHECK(AirconModels_Crc32((const BYTE *)"123456789", 9U) == 0xCBF43926UL);
    MakeBundle(data);
    CHECK(AirconModels_Load(data, sizeof(data), &models) == 0);
    CHECK(models.language_count == 2U && models.languages[0].group_count == 2U);
    CHECK(models.languages[0].base == data + 64 && models.languages[1].groups[1] == data + 100);
    CHECK(AirconModels_Load(NULL, 104U, &models) == AIRCON_ERR_BUNDLE);
    CHECK(AirconModels_Load(data, 15U, &models) == AIRCON_ERR_BUNDLE);
    CHECK(AirconModels_Load(data, 103U, &models) == AIRCON_ERR_BUNDLE);
    CHECK(AirconModels_Load(data, 104U, NULL) == AIRCON_ERR_BUNDLE);
    Put32(data, 0U);
    CHECK(AirconModels_Load(data, 104U, &models) == AIRCON_ERR_BUNDLE);
    MakeBundle(data); Put32(data + 4, 2U);
    CHECK(AirconModels_Load(data, 104U, &models) == AIRCON_ERR_BUNDLE);
    MakeBundle(data); Put32(data + 12, 3U);
    CHECK(AirconModels_Load(data, 104U, &models) == AIRCON_ERR_BUNDLE);
    MakeBundle(data); Put32(data + 20, 44U);
    CHECK(AirconModels_Load(data, 104U, &models) == AIRCON_ERR_BUNDLE);
    MakeBundle(data); Put32(data + 24, 0xFFFFFFFCUL);
    CHECK(AirconModels_Load(data, 104U, &models) == AIRCON_ERR_BUNDLE);
    MakeBundle(data); Put32(data + 32, 1U);
    CHECK(AirconModels_Load(data, 104U, &models) == AIRCON_ERR_BUNDLE);
    MakeBundle(data); Put32(data + 16, 3U);
    CHECK(AirconModels_Load(data, 104U, &models) == AIRCON_ERR_BUNDLE);
    MakeBundle(data); data[100] ^= 1U;
    CHECK(AirconModels_Load(data, 104U, &models) == AIRCON_ERR_CRC);
    MakeBundle(data); Put32(data + 48, 11U);
    Put32(data + 28, AirconModels_Crc32(data + 48, 28U));
    CHECK(AirconModels_Load(data, 104U, &models) == AIRCON_ERR_PACK);
    MakeBundle(data); Put32(data + 52, 0U);
    Put32(data + 28, AirconModels_Crc32(data + 48, 28U));
    CHECK(AirconModels_Load(data, 104U, &models) == AIRCON_ERR_PACK);
    MakeBundle(data); Put32(data + 56, 2U);
    Put32(data + 28, AirconModels_Crc32(data + 48, 28U));
    CHECK(AirconModels_Load(data, 104U, &models) == AIRCON_ERR_PACK);
    CHECK(models.language_count == 0U);
    return 0;
}

static int TestCommands(void)
{
    AIRCON_COMMAND command;
    int32_t i;

    for (i = 1; i <= 19; ++i) {
        CHECK(AirconCommands_Decode(i, &command));
        CHECK(command.command != NULL && !command.is_wake && !command.has_value);
    }
    for (i = 116; i <= 130; ++i) {
        CHECK(AirconCommands_Decode(i, &command));
        CHECK(command.value == i - 100 && strcmp(command.unit, "C") == 0);
    }
    for (i = 201; i <= 212; ++i) {
        CHECK(AirconCommands_Decode(i, &command));
        CHECK(command.value == i - 200 && strcmp(command.command, "TIMER_OFF") == 0);
    }
    for (i = 1001; i <= 1003; ++i) {
        CHECK(AirconCommands_Decode(i, &command) && command.is_wake);
    }
    CHECK(!AirconCommands_Decode(-1, &command));
    CHECK(!AirconCommands_Decode(0, &command));
    CHECK(!AirconCommands_Decode(115, &command));
    CHECK(!AirconCommands_Decode(131, &command));
    CHECK(!AirconCommands_Decode(200, &command));
    CHECK(!AirconCommands_Decode(213, &command));
    CHECK(!AirconCommands_Decode(1004, &command));
    CHECK(!AirconCommands_Decode(1, NULL));
    CHECK(strcmp(AirconCommands_Zone(1001), "ROOM") == 0);
    CHECK(strcmp(AirconCommands_Zone(1002), "LIVING") == 0);
    CHECK(strcmp(AirconCommands_Zone(1003), "GLOBAL") == 0);
    CHECK(strcmp(AirconCommands_Languages(3U), "ZHTW+TAIWANESE") == 0);
    return 0;
}

static int TestRecognition(void)
{
    AIRCON_MODELS models;
    AIRCON_RECOGNITION result;
    int32_t status;
    uint32_t configured_frames;
    uint32_t configured_seconds;

    Setup(&models); fire_at[0] = 2U; fire_at[1] = 4U;
    status = AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 10U, &result);
    CHECK(status == 128 && result.language_mask == 3U && result.frames == 4U);
    CHECK(selected_counts[0] == 2U && selected_counts[1] == 2U);
    CHECK(memory_addresses[1] - memory_addresses[0] == 10004);
    CHECK(preserve_addresses[0] != preserve_addresses[1]);
    CHECK(release_calls == 2U && mock_failure == 0);
#if AIRCON_RESULT_DIAGNOSTICS
    CHECK(result.candidates[0].map_id == 128 && result.candidates[1].map_id == 128);
    CHECK(result.candidates[0].word_index == 100 && result.candidates[1].word_index == 101);
    CHECK(result.candidates[0].matched_frame == 2U && result.candidates[1].matched_frame == 4U);
    CHECK(result.candidates[0].confidence == 250 && result.candidates[1].confidence == 251);
    CHECK(result.candidates[0].sg_diff == 40 && result.candidates[1].sg_diff == 41);
    CHECK(result.candidates[0].fil == -2485 && result.candidates[1].fil == -2485);
    CHECK(result.candidates[0].score_return == 250 && result.candidates[1].score_return == 251);
    CHECK(result.candidates[0].word_duration == 40 && result.candidates[1].word_duration == 41);
    CHECK(result.candidates[0].end_silence == 8 && result.candidates[1].network_latency == 2);
    CHECK(result.candidates[0].epd_return == 0 && result.candidates[1].epd_return == 0);
#endif

    Setup(&models); fire_at[1] = 2U; map_ids[1] = 1002;
    status = AirconVR_Recognize(&models, AIRCON_STAGE_WAKE, 0U, &result);
    CHECK(status == 1002 && result.language_mask == 2U);
    CHECK(selected_counts[0] == 1U && selected_counts[1] == 1U && mock_failure == 0);
#if AIRCON_RESULT_DIAGNOSTICS
    CHECK(result.candidates[0].matched_frame == 0U && result.candidates[0].confidence == 0);
    CHECK(result.candidates[1].map_id == 1002 && result.candidates[1].word_index == 101);
#endif

    Setup(&models); fire_at[0] = 2U;
    status = AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 3U, &result);
    CHECK(status == 128 && result.frames == 5U && result.language_mask == 1U);
    CHECK(engines[0].received == 2U && engines[1].received == 5U && mock_failure == 0);

    Setup(&models); fire_at[0] = 2U; fire_at[1] = 3U; map_ids[1] = 212;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 10U, &result) == AIRCON_ERR_CONFLICT);
    CHECK(result.map_id == 0 && release_calls == 2U);
#if AIRCON_RESULT_DIAGNOSTICS
    CHECK(result.candidates[0].map_id == 128 && result.candidates[1].map_id == 212);
#endif

    Setup(&models);
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == 0);
    CHECK(result.frames == 5U && release_calls == 2U && mock_failure == 0);
#if AIRCON_RESULT_DIAGNOSTICS
    CHECK(result.candidates[0].matched_frame == 0U && result.candidates[1].matched_frame == 0U);
    CHECK(result.candidates[1].map_id == 0 && result.candidates[1].word_duration == 0);

    Setup(&models); diagnostics_error = 1; fire_at[0] = 1U; fire_at[1] = 1U;
#if AIRCON_CONTROL_SCORE_FILTER
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == 0);
    CHECK(result.filtered_count == 2U && result.language_mask == 0U);
#else
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == 128);
#endif
    CHECK(result.candidates[0].score_return == -9001 && result.candidates[1].epd_return == -9002);
    CHECK(result.candidates[0].confidence == 0 && result.candidates[1].word_duration == 0);
    CHECK(release_calls == 2U && mock_failure == 0);
#endif

    Setup(&models);
    configured_frames = AIRCON_CONTROL_TIMEOUT_FRAMES;
    configured_seconds = AIRCON_CONTROL_TIMEOUT_SECONDS;
    frame_limit = configured_frames + 1U;
    CHECK(configured_frames * AIRCON_PCM_FRAME_MS >= configured_seconds * 1000UL);
    CHECK((configured_frames - 1U) * AIRCON_PCM_FRAME_MS < configured_seconds * 1000UL);
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL,
                            AIRCON_CONTROL_TIMEOUT_FRAMES, &result) == 0);
    CHECK(result.frames == AIRCON_CONTROL_TIMEOUT_FRAMES
          && release_calls == 2U && mock_failure == 0);

    Setup(&models); required[0] = required[1] = 60000;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == AIRCON_ERR_MEMORY);
    CHECK(result.memory_total == 120000U && init_calls == 0U && lpbyMemPool[0] == 0xA5);
    Setup(&models); required[1] = DSPOTTER_ERR_LeaveNoMemory;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == DSPOTTER_ERR_LeaveNoMemory);
    CHECK(init_calls == 0U);

    Setup(&models); init_fail = 2U;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == DSPOTTER_ERR_LoadModelFailed);
    CHECK(release_calls == 1U);
    Setup(&models); drop_at = 2U; fire_at[0] = 2U;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == AIRCON_ERR_AUDIO_DROP);
    CHECK(result.map_id == 0 && result.audio_drops == 1U && release_calls == 2U);
    Setup(&models); drop_in_sdk = 1U; fire_at[0] = 1U;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == AIRCON_ERR_AUDIO_DROP);
    CHECK(release_calls == 2U);

    Setup(&models); fatal_sdk = 1U;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == DSPOTTER_ERR_LoadModelFailed);
    CHECK(release_calls == 2U);
    Setup(&models); fire_at[0] = 1U; map_ids[0] = -1;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == AIRCON_ERR_MAP_ID);
    Setup(&models); fire_at[0] = 1U; map_ids[0] = 1001;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == AIRCON_ERR_MAP_ID);
    Setup(&models); reject_once = 1U; fire_at[0] = 2U; fire_at[1] = 2U;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == 128);
    CHECK(reset_calls == 4U && mock_failure == 0);

    Setup(&models); models.language_count = 1U; fire_at[0] = 1U;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == 128);
    CHECK(result.frames == 1U && release_calls == 1U);
    Setup(&models); models.language_count = 3U;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == AIRCON_ERR_BUNDLE);
    Setup(&models); models.languages[0].groups[1] = NULL;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == AIRCON_ERR_PACK);
    CHECK(AirconVR_Recognize(&models, 9U, 5U, &result) == AIRCON_ERR_BUNDLE);
    return 0;
}

static int TestScorePolicy(void)
{
#if AIRCON_CONTROL_SCORE_FILTER || AIRCON_CONFLICT_SCORE_ARBITRATION
    AIRCON_MODELS models;
    AIRCON_RECOGNITION result;
    int32_t status;

#if AIRCON_CONTROL_SCORE_FILTER
    /* Mandarin cancellation versus the low-score Taiwanese warm-mode candidate. */
    Setup(&models); fire_at[0] = fire_at[1] = 2U;
    map_ids[0] = 19; map_ids[1] = 4;
    confidence_scores[0] = 46; sg_scores[0] = 48;
    confidence_scores[1] = 0; sg_scores[1] = 10;
    status = AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 15U, &result);
    CHECK(status == 19 && result.language_mask == 1U);
    CHECK(result.filtered_count == 1U && result.candidates[1].filtered == 1U);
    CHECK(reset_calls == 3U && release_calls == 2U && mock_failure == 0);

    /* A weak early result must not end the window before a later valid result. */
    Setup(&models); fire_at[1] = 2U; fire_at[0] = 7U;
    map_ids[0] = 19; map_ids[1] = 4;
    confidence_scores[0] = 38; sg_scores[0] = 56;
    confidence_scores[1] = 0; sg_scores[1] = 10;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 15U, &result) == 19);
    CHECK(result.frames == 10U && result.language_mask == 1U && result.filtered_count == 1U);

    /* Apply the same rule when Taiwanese has the later, correct result. */
    Setup(&models); fire_at[0] = 2U; fire_at[1] = 7U;
    map_ids[0] = 4; map_ids[1] = 19;
    confidence_scores[0] = 0; sg_scores[0] = 10;
    confidence_scores[1] = 38; sg_scores[1] = 56;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 15U, &result) == 19);
    CHECK(result.frames == 10U && result.language_mask == 2U && result.filtered_count == 1U);

    Setup(&models); fire_at[1] = 2U; map_ids[1] = 4;
    confidence_scores[1] = 7; sg_scores[1] = 24;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 10U, &result) == 4);
    CHECK(result.language_mask == 2U && result.filtered_count == 0U);

    Setup(&models); fire_at[0] = 1U; confidence_scores[0] = 4; sg_scores[0] = 30;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == 0);
    CHECK(result.frames == 5U && result.map_id == 0 && result.filtered_count == 1U);
    Setup(&models); fire_at[0] = 1U; confidence_scores[0] = 20; sg_scores[0] = 14;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == 0);
    CHECK(result.language_mask == 0U && result.filtered_count == 1U);
    Setup(&models); fire_at[0] = 1U; confidence_scores[0] = 5; sg_scores[0] = 15;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == 128);
    CHECK(result.filtered_count == 0U);

    Setup(&models); fire_at[1] = 1U; map_ids[1] = 1002;
    confidence_scores[1] = -1; sg_scores[1] = 27;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_WAKE, 10U, &result) == 1002);
    CHECK(result.language_mask == 2U && result.filtered_count == 0U);

    Setup(&models); fire_at[0] = 1U; confidence_scores[0] = 0; sg_scores[0] = 10;
    reset_fail_at = 3U;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 5U, &result) == DSPOTTER_ERR_LoadModelFailed);
    CHECK(result.map_id == 0 && release_calls == 2U);
#endif

#if AIRCON_CONFLICT_SCORE_ARBITRATION
    /* Mandarin warm mode versus a different Taiwanese power command. */
    Setup(&models); fire_at[0] = 2U; fire_at[1] = 4U;
    map_ids[0] = 4; map_ids[1] = 1;
    confidence_scores[0] = 48; sg_scores[0] = 59;
    confidence_scores[1] = 23; sg_scores[1] = 45;
    status = AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 10U, &result);
    CHECK(status == 4 && result.language_mask == 1U && result.score_resolved == 1U);
    CHECK(result.candidates[0].map_id == 4 && result.candidates[1].map_id == 1);

    Setup(&models); fire_at[0] = 2U; fire_at[1] = 4U;
    map_ids[0] = 1; map_ids[1] = 4;
    confidence_scores[0] = 23; sg_scores[0] = 45;
    confidence_scores[1] = 48; sg_scores[1] = 59;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 10U, &result) == 4);
    CHECK(result.language_mask == 2U && result.score_resolved == 1U);

    Setup(&models); fire_at[0] = fire_at[1] = 2U;
    map_ids[0] = 4; map_ids[1] = 1;
    confidence_scores[0] = 25; sg_scores[0] = 35;
    confidence_scores[1] = 5; sg_scores[1] = 25;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 10U, &result) == 4);
    CHECK(result.language_mask == 1U && result.score_resolved == 1U);

    Setup(&models); fire_at[0] = fire_at[1] = 2U;
    map_ids[0] = 4; map_ids[1] = 1;
    confidence_scores[0] = 42; sg_scores[0] = 60;
    confidence_scores[1] = 23; sg_scores[1] = 45;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 10U, &result) == AIRCON_ERR_CONFLICT);
    CHECK(result.map_id == 0 && result.score_resolved == 0U);
    Setup(&models); fire_at[0] = fire_at[1] = 2U;
    map_ids[0] = 4; map_ids[1] = 1;
    confidence_scores[0] = 48; sg_scores[0] = 54;
    confidence_scores[1] = 23; sg_scores[1] = 45;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 10U, &result) == AIRCON_ERR_CONFLICT);
    CHECK(result.map_id == 0 && release_calls == 2U);

    /* Same-MapID agreement and wake conflicts retain their existing meaning. */
    Setup(&models); fire_at[0] = fire_at[1] = 2U; map_ids[0] = map_ids[1] = 3;
    confidence_scores[0] = 19; sg_scores[0] = 23;
    confidence_scores[1] = 28; sg_scores[1] = 43;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_CONTROL, 10U, &result) == 3);
    CHECK(result.language_mask == 3U && result.score_resolved == 0U);
    Setup(&models); fire_at[0] = fire_at[1] = 2U; map_ids[0] = 1001; map_ids[1] = 1002;
    confidence_scores[0] = 48; sg_scores[0] = 59;
    confidence_scores[1] = 23; sg_scores[1] = 45;
    CHECK(AirconVR_Recognize(&models, AIRCON_STAGE_WAKE, 10U, &result) == AIRCON_ERR_CONFLICT);
    CHECK(result.map_id == 0 && result.score_resolved == 0U);
#endif
#endif
    return 0;
}

static int TestFeedback(void)
{
    AIRCON_LED_FEEDBACK state;
    uint32_t stage;
    uint32_t phase;
    uint32_t tick;
    uint32_t idle;
    uint32_t expected;

    for (stage = 0U; stage < 2U; ++stage) {
        idle = stage ? 0U : 1U;
        CHECK(AirconFeedback_SetStage(&state, stage) == idle);
        CHECK(AirconFeedback_Tick1ms(&state) == idle);
        CHECK(AirconFeedback_Start(&state) == (idle ^ 1U));
        for (phase = 0U; phase < 4U; ++phase) {
            expected = (phase & 1U) ? idle : (idle ^ 1U);
            for (tick = 0U; tick < 99U; ++tick)
                AirconFeedback_Tick1ms(&state);
            CHECK(state.pin_level == expected && state.ticks_remaining == 1U);
            expected = phase == 3U ? idle : (expected ^ 1U);
            CHECK(AirconFeedback_Tick1ms(&state) == expected);
        }
        CHECK(state.phases_remaining == 0U && state.ticks_remaining == 0U);
        CHECK(AirconFeedback_Tick1ms(&state) == idle);
    }
    AirconFeedback_SetStage(&state, 1U);
    AirconFeedback_Start(&state);
    for (tick = 0U; tick < 150U; ++tick)
        AirconFeedback_Tick1ms(&state);
    CHECK(AirconFeedback_Start(&state) == 1U);
    CHECK(state.ticks_remaining == 100U && state.phases_remaining == 4U);
    CHECK(AirconFeedback_SetStage(&state, 0U) == 1U);
    CHECK(state.ticks_remaining == 0U && state.phases_remaining == 0U);
    CHECK(AirconFeedback_Tick1ms(&state) == 1U);
    return 0;
}

static int TestRealBundle(const char *path)
{
    FILE *file;
    BYTE *data;
    long size;
    AIRCON_MODELS models;
    int32_t status;

    file = fopen(path, "rb");
    CHECK(file != NULL);
    CHECK(fseek(file, 0L, SEEK_END) == 0);
    size = ftell(file);
    CHECK(size > 0 && size <= 0x6B000L);
    CHECK(fseek(file, 0L, SEEK_SET) == 0);
    data = (BYTE *)malloc((size_t)size);
    CHECK(data != NULL);
    CHECK(fread(data, 1U, (size_t)size, file) == (size_t)size);
    fclose(file);
    status = AirconModels_Load(data, (uint32_t)size, &models);
    CHECK(status == 0 && models.language_count == 2U);
    CHECK(models.languages[0].language == AIRCON_LANGUAGE_ZHTW);
    CHECK(models.languages[1].language == AIRCON_LANGUAGE_TAIWANESE);
    CHECK(models.languages[0].group_count == 9U && models.languages[1].group_count == 9U);
    printf("PASS actual bilingual bundle: %ld bytes, groups=%lu+%lu\n", size,
           (unsigned long)models.languages[0].group_count,
           (unsigned long)models.languages[1].group_count);
    free(data);
    return 0;
}

int main(int argc, char *argv[])
{
    int result;

    result = TestModels();
    if (!result) result = TestCommands();
    if (!result) result = TestRecognition();
    if (!result) result = TestScorePolicy();
    if (!result) result = TestFeedback();
    if (!result && argc == 2) result = TestRealBundle(argv[1]);
    if (result) {
        printf("FAIL at line %d, checks=%u\n", result, checks);
        return 1;
    }
    printf("PASS %u checks: pack bounds/CRC, MapID routing, shared PCM, arbitration, memory, cleanup and LED timing\n", checks);
    return 0;
}

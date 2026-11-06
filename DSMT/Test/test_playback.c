#include <stdio.h>
#include <string.h>

uint32_t g_u32SPKBuffer[2][320];
static uint32_t starts;
static uint32_t stops;
static uint32_t current;
static uint32_t produced;
static uint32_t lengths[23];
static uint32_t init_order[4];
static uint32_t init_count;
static int fail_init;
static int fail_decode;
static uint16_t sample_rate;
static unsigned int checks;

#define CHECK(x) do { ++checks; if (!(x)) return __LINE__; } while (0)

static void Setup(void)
{
    uint32_t i;

    memset(&g_sApp, 0, sizeof(g_sApp));
    memset(g_u32SPKBuffer, 0x55, sizeof(g_u32SPKBuffer));
    for (i = 0; i < 23U; ++i) lengths[i] = 3U;
    lengths[21] = 4U;
    g_sApp.Mic_NuOne.sNuOneApp.u32TotalAudioNum = 23U;
    starts = stops = init_count = produced = 0U;
    fail_init = -1;
    fail_decode = -1;
    sample_rate = 16000U;
}

void SPK_Start(void) { ++starts; }
void SPK_Stop(void) { ++stops; }

BOOL AudioRom_GetAudioChunkInfo(TEST_READ read, uint32_t base, uint32_t index,
                               S_ROM_AUDIO_CHUNK_INFO *info)
{
    (void)read;
    (void)base;
    info->u32AudioChunkAddr = index;
    return index < 23U;
}

UINT32 NuOneEx_DecodeInitiate(UINT8 *work, UINT8 *temp, uint32_t address, TEST_READ read)
{
    (void)work;
    (void)temp;
    (void)read;
    if (init_count < 4U) init_order[init_count] = address;
    ++init_count;
    current = address;
    produced = 0U;
    return address == (uint32_t)fail_init ? 0U : sample_rate;
}

BOOL NuOneEx_DecodeIsEnd(UINT8 *work)
{
    (void)work;
    return produced >= lengths[current];
}

INT32 NuOneEx_DecodeProcess(UINT8 *work, UINT8 *temp, INT16 *out, TEST_READ read, TEST_EVENT event)
{
    uint32_t i;
    int16_t value;

    (void)work;
    (void)temp;
    (void)read;
    (void)event;
    if (current == (uint32_t)fail_decode) return -2;
    value = (int16_t)((current + 1U) * 100U + produced);
    for (i = 0; i < 320U; ++i) out[i] = value;
    ++produced;
    return 320;
}

static int Drain(uint16_t *frames, uint32_t *count)
{
    uint32_t iterations;
    uint32_t half;

    iterations = 0U;
    while (g_sApp.Mic_NuOne.sNuOneApp.bPCMPlaying && iterations < 100U) {
        half = PDMA1CallBackCount & 1U;
        frames[iterations] = (uint16_t)g_u32SPKBuffer[half][0];
        ++iterations;
        ++PDMA1CallBackCount;
        g_sApp.Mic_NuOne.sNuOneApp.bBufferEmpty = TRUE;
        PlayLoop();
    }
    *count = iterations;
    CHECK(iterations < 100U);
    return 0;
}

static int TestPlayback(void)
{
    uint8_t sequence[2];
    uint16_t frames[100];
    uint32_t count;
    uint32_t i;
    uint8_t buffer[4];

    Setup();
    sequence[0] = 1U;
    sequence[1] = 21U;
    CHECK(PlayNuOneSequence(sequence, 2U));
    CHECK(g_sApp.Mic_NuOne.sNuOneApp.bPCMPlaying);
    CHECK(!PlayNuOneSequence(sequence, 2U));
    CHECK(Drain(frames, &count) == 0);
    CHECK(count == 13U && starts == 1U && stops == 1U && init_count == 2U);
    CHECK(init_order[0] == 1U && init_order[1] == 21U);
    for (i = 0; i < 3U; ++i) CHECK(frames[i] == 200U + i);
    for (i = 3U; i < 9U; ++i) CHECK(frames[i] == 0U);
    for (i = 9U; i < 13U; ++i) CHECK(frames[i] == 2200U + i - 9U);
    Setup();
    CHECK(PlayNuOnebyIdx(0U));
    CHECK(Drain(frames, &count) == 0 && count == 3U && starts == 1U && stops == 1U);
    Setup();
    lengths[0] = 1U;
    CHECK(PlayNuOnebyIdx(0U));
    CHECK(g_u32SPKBuffer[1][0] == 0U);
    CHECK(Drain(frames, &count) == 0 && count == 1U && stops == 1U);
    Setup();
    CHECK(!PlayNuOneSequence(NULL, 2U));
    CHECK(!PlayNuOneSequence(sequence, 0U));
    CHECK(!PlayNuOneSequence(sequence, 3U));
    CHECK(!PlayNuOnebyIdx(23U));
    CHECK(starts == 0U);
    fail_init = 1;
    CHECK(!PlayNuOneSequence(sequence, 2U));
    CHECK(!g_sApp.Mic_NuOne.sNuOneApp.bPCMPlaying && starts == 0U);
    Setup();
    fail_init = 21;
    CHECK(PlayNuOneSequence(sequence, 2U));
    CHECK(Drain(frames, &count) == 0 && stops == 1U && init_count == 2U);
    CHECK(count == 3U);
    Setup();
    fail_decode = 21;
    CHECK(PlayNuOneSequence(sequence, 2U));
    CHECK(Drain(frames, &count) == 0 && stops == 1U);
    Setup();
    sample_rate = 8000U;
    CHECK(!PlayNuOnebyIdx(0U));
    CHECK(starts == 0U);
    memset(buffer, 0x55, sizeof(buffer));
    PlayNuOneSetAudioSize(100U);
    CHECK(ReadDataCallback(buffer, AUDIOROM_STORAGE_START_ADDR - 1U, 1U) == 0U);
    CHECK(ReadDataCallback(buffer, AUDIOROM_STORAGE_START_ADDR + 99U, 2U) == 0U);
    CHECK(ReadDataCallback(buffer, 0xFFFFFFFFUL, 2U) == 0U);
    CHECK(ReadDataCallback(NULL, AUDIOROM_STORAGE_START_ADDR, 1U) == 0U);
    CHECK(buffer[0] == 0x55U);
    return 0;
}

int main(void)
{
    int result;

    result = TestPlayback();
    if (result) {
        printf("FAIL playback line %d, checks=%u\n", result, checks);
        return 1;
    }
    printf("PASS %u playback checks: continuous DMA, prefix gap, final drain, failures and flash read bounds\n", checks);
    return 0;
}

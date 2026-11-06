#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AirconAudio.h"
#include "AirconModels.h"

static unsigned int checks;
#define CHECK(x) do { ++checks; if (!(x)) return __LINE__; } while (0)

static uint32_t Get32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void Put32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint8_t *ReadFile(const char *path, uint32_t *bytes)
{
    FILE *file;
    long size;
    uint8_t *data;

    file = fopen(path, "rb");
    if (file == NULL)
        return NULL;
    fseek(file, 0L, SEEK_END);
    size = ftell(file);
    rewind(file);
    if (size <= 0L || size > 0x80000L) {
        fclose(file);
        return NULL;
    }
    data = (uint8_t *)malloc((size_t)size);
    if (data == NULL || fread(data, 1U, (size_t)size, file) != (size_t)size) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *bytes = (uint32_t)size;
    return data;
}

static void RefreshCrc(uint8_t *image)
{
    Put32(image + 12, AirconModels_Crc32(image + 16, Get32(image + 8)));
}

static int TestAudio(uint8_t *data, uint32_t bytes)
{
    AIRCON_AUDIO audio;
    uint8_t *copy;
    uint32_t rom_bytes;
    uint32_t offset;

    CHECK(bytes <= AIRCON_AUDIO_CAPACITY);
    CHECK(AirconAudio_Load(data, bytes, &audio) == 0);
    CHECK(audio.audio_count == 23U && audio.rom == data + 16);
    rom_bytes = audio.rom_bytes;
    CHECK(AirconAudio_Load(NULL, bytes, &audio) == AIRCON_AUDIO_ERR_HEADER);
    CHECK(audio.rom == NULL);
    CHECK(AirconAudio_Load(data, bytes, NULL) == AIRCON_AUDIO_ERR_HEADER);
    CHECK(AirconAudio_Load(data, 15U, &audio) == AIRCON_AUDIO_ERR_HEADER);
    CHECK(AirconAudio_Load(data, 16U + rom_bytes - 1U, &audio) == AIRCON_AUDIO_ERR_SIZE);
    copy = (uint8_t *)malloc(bytes);
    CHECK(copy != NULL);
    memcpy(copy, data, bytes);
    copy[0] ^= 1U;
    CHECK(AirconAudio_Load(copy, bytes, &audio) == AIRCON_AUDIO_ERR_HEADER);
    memcpy(copy, data, bytes);
    Put32(copy + 4, 2U);
    CHECK(AirconAudio_Load(copy, bytes, &audio) == AIRCON_AUDIO_ERR_HEADER);
    memcpy(copy, data, bytes);
    Put32(copy + 8, 0xFFFFFFFFUL);
    CHECK(AirconAudio_Load(copy, bytes, &audio) == AIRCON_AUDIO_ERR_SIZE);
    memcpy(copy, data, bytes);
    copy[bytes - 4U] ^= 1U;
    CHECK(AirconAudio_Load(copy, bytes, &audio) == AIRCON_AUDIO_ERR_CRC);
    memcpy(copy, data, bytes);
    Put32(copy + 16, 24U);
    RefreshCrc(copy);
    CHECK(AirconAudio_Load(copy, bytes, &audio) == AIRCON_AUDIO_ERR_INDEX);
    memcpy(copy, data, bytes);
    Put32(copy + 24, 0xFFFFFFF0UL);
    RefreshCrc(copy);
    CHECK(AirconAudio_Load(copy, bytes, &audio) == AIRCON_AUDIO_ERR_INDEX);
    memcpy(copy, data, bytes);
    Put32(copy + 32, 201U);
    RefreshCrc(copy);
    CHECK(AirconAudio_Load(copy, bytes, &audio) == AIRCON_AUDIO_ERR_INDEX);
    memcpy(copy, data, bytes);
    Put32(copy + 36, 0xFFFFFFFFUL);
    RefreshCrc(copy);
    CHECK(AirconAudio_Load(copy, bytes, &audio) == AIRCON_AUDIO_ERR_INDEX);
    offset = 16U + Get32(data + 32);
    memcpy(copy, data, bytes);
    copy[offset + 2U] = 0U;
    RefreshCrc(copy);
    CHECK(AirconAudio_Load(copy, bytes, &audio) == AIRCON_AUDIO_ERR_FORMAT);
    memcpy(copy, data, bytes);
    Put32(copy + offset + 8U, 256U);
    RefreshCrc(copy);
    CHECK(AirconAudio_Load(copy, bytes, &audio) == AIRCON_AUDIO_ERR_FORMAT);
    memcpy(copy, data, bytes);
    Put32(copy + offset + 4U, 0xFFFFFFFFUL);
    RefreshCrc(copy);
    CHECK(AirconAudio_Load(copy, bytes, &audio) == AIRCON_AUDIO_ERR_FORMAT);
    free(copy);
    return 0;
}

static int TestReplies(void)
{
    AIRCON_REPLY reply;
    int32_t id;

    for (id = 1; id <= 19; ++id) {
        CHECK(AirconAudio_Reply(id, &reply) == 1);
        CHECK(reply.count == 2U && reply.indexes[0] == 1U
            && reply.indexes[1] == id + 1);
    }
    for (id = 116; id <= 130; ++id) {
        CHECK(AirconAudio_Reply(id, &reply) == 1);
        CHECK(reply.count == 2U && reply.indexes[1] == 21U);
    }
    for (id = 201; id <= 212; ++id) {
        CHECK(AirconAudio_Reply(id, &reply) == 1);
        CHECK(reply.count == 2U && reply.indexes[1] == 22U);
    }
    for (id = 1001; id <= 1003; ++id) {
        CHECK(AirconAudio_Reply(id, &reply) == 1);
        CHECK(reply.count == 1U && reply.indexes[0] == 0U);
    }
    CHECK(AirconAudio_Reply(0, &reply) == 0 && reply.count == 0U);
    CHECK(AirconAudio_Reply(115, &reply) == 0);
    CHECK(AirconAudio_Reply(131, &reply) == 0);
    CHECK(AirconAudio_Reply(200, &reply) == 0);
    CHECK(AirconAudio_Reply(213, &reply) == 0);
    CHECK(AirconAudio_Reply(1, NULL) == 0);
    return 0;
}

int main(int argc, char *argv[])
{
    uint8_t *audio;
    uint8_t *combined;
    uint32_t audio_bytes;
    uint32_t combined_bytes;
    uint32_t model_bytes;
    uint32_t i;
    uint32_t gap_ok;
    AIRCON_MODELS models;
    int result;

    if (argc != 3)
        return 2;
    audio = ReadFile(argv[1], &audio_bytes);
    combined = ReadFile(argv[2], &combined_bytes);
    CHECK(audio != NULL && combined != NULL);
    result = TestAudio(audio, audio_bytes);
    if (!result) result = TestReplies();
    if (result) {
        printf("FAIL audio line %d, checks=%u\n", result, checks);
        return 1;
    }
    CHECK(combined_bytes == AIRCON_AUDIO_ADDRESS - AIRCON_MODEL_ADDRESS + audio_bytes);
    CHECK(AirconModels_Load(combined, AIRCON_AUDIO_ADDRESS - AIRCON_MODEL_ADDRESS, &models) == 0);
    CHECK(memcmp(combined + AIRCON_AUDIO_ADDRESS - AIRCON_MODEL_ADDRESS, audio, audio_bytes) == 0);
    model_bytes = Get32(combined + 8);
    gap_ok = 1U;
    for (i = model_bytes; i < AIRCON_AUDIO_ADDRESS - AIRCON_MODEL_ADDRESS; ++i)
        if (combined[i] != 0xFFU) gap_ok = 0U;
    CHECK(gap_ok != 0U);
    free(audio);
    free(combined);
    printf("PASS %u audio checks: image CRC/bounds/bitrate, all MapIDs, combined DataFlash and erased gap\n", checks);
    return 0;
}

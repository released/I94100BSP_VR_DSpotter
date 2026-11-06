#include <stddef.h>
#include <string.h>
#include "AirconAudio.h"
#include "AirconModels.h"

static uint32_t Read32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8)
        | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static uint32_t Read16(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8);
}

/*
 * The ACR1 wrapper contains version, ROM length and CRC32. Validate every
 * audio chunk before enabling playback: NuOne, 16 kHz, 320-bit frames
 * (1.0 bit/sample). The decoder later reads the validated ROM in Flash.
 */
int32_t AirconAudio_Load(const uint8_t *image, uint32_t available,
                        AIRCON_AUDIO *audio)
{
    const uint8_t *rom;
    const uint8_t *chunk;
    uint32_t bytes;
    uint32_t count;
    uint32_t index;
    uint32_t cursor;
    uint32_t offset;
    uint32_t size;
    uint32_t payload;
    uint32_t i;

    if (audio == NULL)
        return AIRCON_AUDIO_ERR_HEADER;
    memset(audio, 0, sizeof(*audio));
    if (image == NULL || available < AIRCON_AUDIO_HEADER_BYTES)
        return AIRCON_AUDIO_ERR_HEADER;
    if (Read32(image) != AIRCON_AUDIO_MAGIC
        || Read32(image + 4) != AIRCON_AUDIO_VERSION)
        return AIRCON_AUDIO_ERR_HEADER;
    bytes = Read32(image + 8);
    if (bytes < 16U || bytes > available - AIRCON_AUDIO_HEADER_BYTES)
        return AIRCON_AUDIO_ERR_SIZE;
    rom = image + AIRCON_AUDIO_HEADER_BYTES;
    if (AirconModels_Crc32(rom, bytes) != Read32(image + 12))
        return AIRCON_AUDIO_ERR_CRC;
    count = Read32(rom);
    index = Read32(rom + 8);
    if (count != AIRCON_AUDIO_COUNT || Read32(rom + 4) != 0U
        || index != 16U || count > (bytes - index) / 8U)
        return AIRCON_AUDIO_ERR_INDEX;
    cursor = index + count * 8U;
    for (i = 0U; i < count; ++i) {
        offset = Read32(rom + index + i * 8U);
        size = Read32(rom + index + i * 8U + 4U);
        if (offset != cursor || size < 12U || size > bytes - cursor)
            return AIRCON_AUDIO_ERR_INDEX;
        chunk = rom + offset;
        payload = Read32(chunk + 4);
        if (Read16(chunk) != 0x0040U || Read16(chunk + 2) != 16000U
            || Read32(chunk + 8) != 320U || payload == 0U
            || payload % 40U != 0U || payload > size - 12U)
            return AIRCON_AUDIO_ERR_FORMAT;
        cursor += size;
    }
    if (cursor != bytes)
        return AIRCON_AUDIO_ERR_SIZE;
    audio->rom = rom;
    audio->rom_bytes = bytes;
    audio->audio_count = count;
    return 0;
}

/*
 * Wake uses clip 0. Controls queue clip 1 (the shared prefix) and a body.
 * Temperature and timer values remain in UART while their replies share
 * clips 21 and 22. Keep this mapping synchronized with ReplyProfile.json,
 * the Audio Tool index header, and the packaging validation rules.
 */
int32_t AirconAudio_Reply(int32_t map_id, AIRCON_REPLY *reply)
{
    uint32_t body;

    if (reply == NULL)
        return 0;
    memset(reply, 0, sizeof(*reply));
    if (map_id >= 1001 && map_id <= 1003) {
        reply->indexes[0] = 0U;
        reply->count = 1U;
        return 1;
    }
    if (map_id >= 1 && map_id <= 19)
        body = (uint32_t)map_id + 1U;
    else if (map_id >= 116 && map_id <= 130)
        body = 21U;
    else if (map_id >= 201 && map_id <= 212)
        body = 22U;
    else
        return 0;
    reply->indexes[0] = 1U;
    reply->indexes[1] = (uint8_t)body;
    reply->count = 2U;
    return 1;
}

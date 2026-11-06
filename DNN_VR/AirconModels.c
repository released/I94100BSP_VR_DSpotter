#include <stddef.h>
#include <string.h>
#include "AirconModels.h"

/* Flash image fields are little-endian; byte reads avoid unaligned accesses. */
static uint32_t Read32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8)
        | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

/* Match the reflected CRC-32 stored by the Windows bundle builder. */
uint32_t AirconModels_Crc32(const uint8_t *data, uint32_t size)
{
    uint32_t crc;
    uint32_t i;
    uint32_t bit;

    crc = 0xFFFFFFFFUL;
    for (i = 0; i < size; ++i) {
        crc ^= data[i];
        for (bit = 0; bit < 8U; ++bit) {
            if ((crc & 1U) != 0U)
                crc = (crc >> 1) ^ 0xEDB88320UL;
            else
                crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

/*
 * A native pack contains a chunk count, chunk lengths, then chunk bytes.
 * Chunk zero is CYBase; following chunks retain the DSMT group order.
 * Return Flash pointers rather than copying acoustic models into SRAM.
 */
static int32_t LoadPack(const uint8_t *data, uint32_t size,
                        AIRCON_LANGUAGE_MODELS *language)
{
    uint32_t count;
    uint32_t offset;
    uint32_t length;
    uint32_t i;

    if (size < 16U)
        return AIRCON_ERR_PACK;
    count = Read32(data);
    if (count < 3U || count > AIRCON_MAX_GROUPS + 1U)
        return AIRCON_ERR_PACK;
    offset = (count + 1U) * 4U;
    if (offset > size)
        return AIRCON_ERR_PACK;
    for (i = 0; i < count; ++i) {
        length = Read32(data + 4U + i * 4U);
        if (length < 4U || (length & 3U) != 0U || length > size - offset)
            return AIRCON_ERR_PACK;
        if (i == 0U)
            language->base = data + offset;
        else
            language->groups[i - 1U] = data + offset;
        offset += length;
    }
    if (offset != size)
        return AIRCON_ERR_PACK;
    language->group_count = count - 1U;
    return 0;
}

/*
 * Validate the ACB1 header and each language directory entry before use.
 * Each entry is {language, offset, length, CRC32}; offsets must describe
 * consecutive native packs within the model partition. Group zero in each
 * resulting language is the wake group used by AirconVR_Recognize().
 */
int32_t AirconModels_Load(const uint8_t *data, uint32_t available,
                         AIRCON_MODELS *models)
{
    uint32_t total;
    uint32_t count;
    uint32_t offset;
    uint32_t expected_offset;
    uint32_t length;
    uint32_t language;
    uint32_t crc;
    uint32_t used_languages;
    uint32_t i;
    int32_t result;
    const uint8_t *entry;

    if (models == NULL)
        return AIRCON_ERR_BUNDLE;
    memset(models, 0, sizeof(*models));
    if (data == NULL || available < 16U)
        return AIRCON_ERR_BUNDLE;
    if (Read32(data) != AIRCON_BUNDLE_MAGIC || Read32(data + 4U) != 1U)
        return AIRCON_ERR_BUNDLE;
    total = Read32(data + 8U);
    count = Read32(data + 12U);
    if (count == 0U || count > AIRCON_MAX_LANGUAGES)
        return AIRCON_ERR_BUNDLE;
    expected_offset = 16U + count * 16U;
    if (total > available || total < expected_offset || (total & 3U) != 0U)
        return AIRCON_ERR_BUNDLE;
    used_languages = 0U;
    for (i = 0; i < count; ++i) {
        entry = data + 16U + i * 16U;
        language = Read32(entry);
        offset = Read32(entry + 4U);
        length = Read32(entry + 8U);
        crc = Read32(entry + 12U);
        if (language != AIRCON_LANGUAGE_ZHTW && language != AIRCON_LANGUAGE_TAIWANESE)
            return AIRCON_ERR_BUNDLE;
        if ((used_languages & (1U << language)) != 0U)
            return AIRCON_ERR_BUNDLE;
        used_languages |= 1U << language;
        if (offset != expected_offset || length > total - expected_offset)
            return AIRCON_ERR_BUNDLE;
        if (AirconModels_Crc32(data + offset, length) != crc)
            return AIRCON_ERR_CRC;
        models->languages[i].language = language;
        result = LoadPack(data + offset, length, &models->languages[i]);
        if (result != 0)
            return result;
        expected_offset += length;
    }
    if (expected_offset != total)
        return AIRCON_ERR_BUNDLE;
    models->language_count = count;
    return 0;
}

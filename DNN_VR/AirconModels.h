#ifndef AIRCON_MODELS_H
#define AIRCON_MODELS_H

#include <stdint.h>

#define AIRCON_MAX_LANGUAGES 2U
#define AIRCON_MAX_GROUPS 9U
#define AIRCON_BUNDLE_MAGIC 0x31424341UL
#define AIRCON_LANGUAGE_ZHTW 1U
#define AIRCON_LANGUAGE_TAIWANESE 2U

#define AIRCON_ERR_BUNDLE (-1101)
#define AIRCON_ERR_PACK (-1102)
#define AIRCON_ERR_CRC (-1103)
#define AIRCON_ERR_MEMORY (-1104)
#define AIRCON_ERR_AUDIO_DROP (-1105)
#define AIRCON_ERR_CONFLICT (-1106)
#define AIRCON_ERR_MAP_ID (-1107)

/* Model pointers refer to memory-mapped Flash and do not own allocations. */
typedef struct {
    uint32_t language;
    const uint8_t *base;
    const uint8_t *groups[AIRCON_MAX_GROUPS];
    uint32_t group_count;
} AIRCON_LANGUAGE_MODELS;

typedef struct {
    uint32_t language_count;
    AIRCON_LANGUAGE_MODELS languages[AIRCON_MAX_LANGUAGES];
} AIRCON_MODELS;

uint32_t AirconModels_Crc32(const uint8_t *data, uint32_t size);
/* available is the partition size, not the size of the entire MCU Flash. */
int32_t AirconModels_Load(const uint8_t *data, uint32_t available,
                         AIRCON_MODELS *models);

#endif

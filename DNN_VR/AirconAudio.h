#ifndef AIRCON_AUDIO_H
#define AIRCON_AUDIO_H

#include <stdint.h>

#define AIRCON_MODEL_ADDRESS 0x00015000UL
#define AIRCON_AUDIO_ADDRESS 0x00070000UL
#define AIRCON_FLASH_END 0x00080000UL
#define AIRCON_AUDIO_CAPACITY (AIRCON_FLASH_END - AIRCON_AUDIO_ADDRESS)
#define AIRCON_AUDIO_HEADER_BYTES 16U
#define AIRCON_AUDIO_MAGIC 0x31524341UL
#define AIRCON_AUDIO_VERSION 1U
#define AIRCON_AUDIO_COUNT 23U
#define AIRCON_REPLY_MAX_PARTS 2U
#define AIRCON_REPLY_GAP_FRAMES 6U

#define AIRCON_AUDIO_ERR_HEADER (-1201)
#define AIRCON_AUDIO_ERR_SIZE (-1202)
#define AIRCON_AUDIO_ERR_CRC (-1203)
#define AIRCON_AUDIO_ERR_INDEX (-1204)
#define AIRCON_AUDIO_ERR_FORMAT (-1205)

typedef struct {
    const uint8_t *rom;
    uint32_t rom_bytes;
    uint32_t audio_count;
} AIRCON_AUDIO;

typedef struct {
    uint8_t indexes[AIRCON_REPLY_MAX_PARTS];
    uint32_t count;
} AIRCON_REPLY;

int32_t AirconAudio_Load(const uint8_t *image, uint32_t available,
                        AIRCON_AUDIO *audio);
int32_t AirconAudio_Reply(int32_t map_id, AIRCON_REPLY *reply);

#endif

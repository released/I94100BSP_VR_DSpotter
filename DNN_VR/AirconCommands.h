#ifndef AIRCON_COMMANDS_H
#define AIRCON_COMMANDS_H

#include <stdint.h>

/* Numeric value/unit are meaningful only when has_value is nonzero. */
typedef struct {
    const char *category;
    const char *command;
    const char *unit;
    int32_t value;
    uint32_t has_value;
    uint32_t is_wake;
} AIRCON_COMMAND;

/* Return 1 for a known action or wake ID, 0 for an unknown ID/NULL output. */
int32_t AirconCommands_Decode(int32_t map_id, AIRCON_COMMAND *command);
const char *AirconCommands_Zone(int32_t wake_id);
const char *AirconCommands_Languages(uint32_t mask);

#endif

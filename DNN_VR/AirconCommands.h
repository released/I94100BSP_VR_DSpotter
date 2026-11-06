#ifndef AIRCON_COMMANDS_H
#define AIRCON_COMMANDS_H

#include <stdint.h>

typedef struct {
    const char *category;
    const char *command;
    const char *unit;
    int32_t value;
    uint32_t has_value;
    uint32_t is_wake;
} AIRCON_COMMAND;

int32_t AirconCommands_Decode(int32_t map_id, AIRCON_COMMAND *command);
const char *AirconCommands_Zone(int32_t wake_id);
const char *AirconCommands_Languages(uint32_t mask);

#endif

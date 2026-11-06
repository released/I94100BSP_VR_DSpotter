#include <stddef.h>
#include <string.h>
#include "AirconCommands.h"

typedef struct {
    const char *category;
    const char *command;
} FIXED_COMMAND;

static const FIXED_COMMAND fixed_commands[] = {
    {"Power", "POWER_ON"}, {"Power", "POWER_OFF"},
    {"Mode", "MODE_COOL"}, {"Mode", "MODE_HEAT"},
    {"Mode", "MODE_DRY"}, {"Mode", "MODE_AUTO"},
    {"Temperature", "TEMP_UP"}, {"Temperature", "TEMP_DOWN"},
    {"Fan", "FAN_UP"}, {"Fan", "FAN_DOWN"}, {"Fan", "FAN_AUTO"},
    {"Swing", "SWING_AUTO"}, {"Swing", "SWING_HOLD"},
    {"Health", "NANOE_ON"}, {"Health", "NANOE_OFF"},
    {"Volume", "VOICE_VOL_UP"}, {"Volume", "VOICE_VOL_DOWN"},
    {"Volume", "VOICE_MUTE"}, {"Timer", "TIMER_CANCEL"}
};

int32_t AirconCommands_Decode(int32_t map_id, AIRCON_COMMAND *command)
{
    if (command == NULL)
        return 0;
    memset(command, 0, sizeof(*command));
    command->unit = "none";
    if (map_id >= 1 && map_id <= 19) {
        command->category = fixed_commands[map_id - 1].category;
        command->command = fixed_commands[map_id - 1].command;
        return 1;
    }
    if (map_id >= 116 && map_id <= 130) {
        command->category = "Temperature";
        command->command = "TEMP_SET";
        command->value = map_id - 100;
        command->unit = "C";
        command->has_value = 1U;
        return 1;
    }
    if (map_id >= 201 && map_id <= 212) {
        command->category = "Timer";
        command->command = "TIMER_OFF";
        command->value = map_id - 200;
        command->unit = "h";
        command->has_value = 1U;
        return 1;
    }
    if (map_id >= 1001 && map_id <= 1003) {
        command->category = "Wake";
        command->command = "WAKE";
        command->is_wake = 1U;
        return 1;
    }
    return 0;
}

const char *AirconCommands_Zone(int32_t wake_id)
{
    switch (wake_id) {
    case 1001: return "ROOM";
    case 1002: return "LIVING";
    case 1003: return "GLOBAL";
    default: return "UNSELECTED";
    }
}

const char *AirconCommands_Languages(uint32_t mask)
{
    switch (mask) {
    case 1U: return "ZHTW";
    case 2U: return "TAIWANESE";
    case 3U: return "ZHTW+TAIWANESE";
    default: return "UNKNOWN";
    }
}

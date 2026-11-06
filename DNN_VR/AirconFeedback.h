#ifndef AIRCON_FEEDBACK_H
#define AIRCON_FEEDBACK_H

#include <stdint.h>

#define AIRCON_LED_PHASE_MS 100U
#define AIRCON_LED_PHASE_COUNT 4U

typedef struct {
    uint32_t idle_level;
    uint32_t pin_level;
    uint32_t ticks_remaining;
    uint32_t phases_remaining;
} AIRCON_LED_FEEDBACK;

/* Foreground callers must exclude the timer interrupt while updating state. */
uint32_t AirconFeedback_SetStage(volatile AIRCON_LED_FEEDBACK *state,
                                uint32_t control_stage);
uint32_t AirconFeedback_Start(volatile AIRCON_LED_FEEDBACK *state);
uint32_t AirconFeedback_Tick1ms(volatile AIRCON_LED_FEEDBACK *state);

#endif

#include "AirconFeedback.h"

/* PB0 is active low: wake idle is off, control idle is on. */
uint32_t AirconFeedback_SetStage(volatile AIRCON_LED_FEEDBACK *state,
                                uint32_t control_stage)
{
    state->idle_level = control_stage ? 0U : 1U;
    state->pin_level = state->idle_level;
    state->ticks_remaining = 0U;
    state->phases_remaining = 0U;
    return state->pin_level;
}

/* Four 100 ms phases make acceptance visible from either idle LED level. */
uint32_t AirconFeedback_Start(volatile AIRCON_LED_FEEDBACK *state)
{
    state->pin_level = state->idle_level ^ 1U;
    state->ticks_remaining = AIRCON_LED_PHASE_MS;
    state->phases_remaining = AIRCON_LED_PHASE_COUNT;
    return state->pin_level;
}

/* Called by the 1 ms timer; no delays or audio work run inside this update. */
uint32_t AirconFeedback_Tick1ms(volatile AIRCON_LED_FEEDBACK *state)
{
    if (state->ticks_remaining == 0U)
        return state->pin_level;
    --state->ticks_remaining;
    if (state->ticks_remaining == 0U) {
        --state->phases_remaining;
        if (state->phases_remaining == 0U) {
            state->pin_level = state->idle_level;
        } else {
            state->pin_level ^= 1U;
            state->ticks_remaining = AIRCON_LED_PHASE_MS;
        }
    }
    return state->pin_level;
}

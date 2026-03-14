#include "software_timer.h"

// Calculate the ticks elapsed to create a new frequency (timer_f)
// Expects tick_f to be higher than timer_f
// Expects tick_f in Hz and timer_f in Hz 
static uint32_t calc_ticks_ff(uint32_t tick_f, uint32_t timer_f) {
    return tick_f / timer_f;
}

// Calculate the pacer ticks elapsed to create a new period (timer_t)
// Expects tick_f in Hz and timer_t in ms
static uint32_t calc_ticks_ft(uint32_t tick_f, uint32_t timer_t) {
    return (uint32_t)timer_t * (uint32_t)tick_f / 1000; 
}

// Constructs software timer, uses one of the functions above to calculate period ticks
static software_timer_t construct_stimer(
    uint32_t tick_f, 
    uint32_t timer_u, 
    uint32_t init_ticks,
    software_timer_mode_t mode, 
    uint32_t (*calc_ticks_fn)(uint32_t, uint32_t)
) {
    uint32_t period_ticks = (*calc_ticks_fn)(tick_f, timer_u);

    software_timer_t new_timer = {
        .mode = mode,
        .period_ticks = period_ticks,
        .rollover_ticks = init_ticks + period_ticks,
    };

    return new_timer;
}

// Constructs software timer with specified frequency 
// Expects timer_f and tick_f in Hz
software_timer_t construct_stimer_f(
    uint32_t tick_f, 
    uint32_t timer_f, 
    uint32_t init_ticks,
    software_timer_mode_t mode
) {
    return construct_stimer(tick_f, timer_f, init_ticks, mode, calc_ticks_ff);
}

// Constructs software timer with specified period
// Expects timer_t in ms and tick_f in Hz
software_timer_t construct_stimer_p(
    uint32_t tick_f, 
    uint32_t timer_t, 
    uint32_t init_ticks,
    software_timer_mode_t mode
) {
    return construct_stimer(tick_f, timer_t, init_ticks, mode, calc_ticks_ft);
}


#include <stdio.h>
#include <usb_animation.h>

// Resets a timer to run again at the rate it was initialised with
void reset_stimer(software_timer_t* software_timer, uint32_t current_ticks) {
    software_timer->rollover_ticks = current_ticks + software_timer->period_ticks;

    if (software_timer->mode == DISABLED_ST) {
        software_timer-> mode = ONESHOT_ST;
    }
}

// Return true if the timer has finished
// Also resets periodic timers so they will continue to run
bool is_stimer_finished(software_timer_t* software_timer, uint32_t current_ticks) {
    if (software_timer->mode == DISABLED_ST) {
        return false;
    }

    if (current_ticks >= software_timer->rollover_ticks) {

        if (software_timer->mode == PERIODIC_ST) {
            reset_stimer(software_timer, current_ticks);
        } else {
            software_timer->mode = DISABLED_ST;
        }

        return true;
    }

    return false;
}

// Returns the progress of the timer (0-100)
uint8_t get_stimer_progress(software_timer_t* software_timer, uint32_t current_ticks) {
    uint8_t progress = (uint8_t)(((uint64_t)current_ticks * 100) / software_timer->period_ticks);
    if (progress > 100) {
        progress = 100; // Clamp range
    }

    return progress;
}
#ifndef SOFTWARE_TIMER_H
#define SOFTWARE_TIMER_H

#include <stdint.h>
#include <stdbool.h>

// A oneshot software timer will run once until it reaches rollover_ticks
// Periodic timer will repeat forever
typedef enum {
    ONESHOT_ST,
    PERIODIC_ST,
    DISABLED_ST,
} software_timer_mode_t;

// This shouldn't be manually configured
// Use constructor instead
typedef struct {
    software_timer_mode_t mode;
    uint32_t period_ticks;
    uint32_t rollover_ticks;
} software_timer_t;


software_timer_t construct_stimer_f(
    uint32_t tick_f, 
    uint32_t timer_f, 
    uint32_t init_ticks,
    software_timer_mode_t mode
);

software_timer_t construct_stimer_p(
    uint32_t tick_f, 
    uint32_t timer_t, 
    uint32_t init_ticks,
    software_timer_mode_t mode
);

void reset_stimer(software_timer_t* software_timer, uint32_t current_ticks);
bool is_stimer_finished(software_timer_t* software_timer, uint32_t current_ticks);
uint8_t get_stimer_progress(software_timer_t* software_timer, uint32_t current_ticks);


#endif
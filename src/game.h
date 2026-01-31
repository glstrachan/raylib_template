#pragma once

#include "../external/core.h"

typedef struct {
    Font neutral_font, dialog_font, dialog_font_big;
    int screen_width, screen_height;
} Game_Parameters;

typedef struct {
    int64_t tick;
    int64_t start;
} Game_Timer;

static inline void timer_init(Game_Timer* timer, int64_t ms) {
    timer->tick = ms;
    timer->start = ms;
}

static inline int64_t timer_elapsed(Game_Timer* timer, int64_t ms) {
    return max(timer->start - timer->tick, 0);
}

static inline void timer_reset(Game_Timer* timer) {
    timer->tick = timer->start;
}

static inline bool timer_update(Game_Timer* timer) {
    timer->tick -= (int64_t)(GetFrameTime() * 1000.0f);
    return timer->tick <= 0;
}

static inline bool timer_ready(Game_Timer* timer) {
    return timer->tick <= 0;
}

extern Game_Parameters game_parameters;

extern Oc_Arena frame_arena;
extern Oc_Arena arena;
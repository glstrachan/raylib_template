#pragma once

#include "../external/core.h"

typedef struct {
    Font neutral_font;
    int screen_width, screen_height;
} Game_Parameters;

typedef struct {
    int64_t tick;
} Game_Timer;

static inline void timer_init(Game_Timer* timer, int64_t ms) {
    timer->tick = ms;
}

static inline bool timer_update(Game_Timer* timer, float dt) {
    timer->tick -= (int64_t)(dt * 1000.0f);
    return timer->tick <= 0;
}

static inline bool timer_ready(Game_Timer* timer) {
    return timer->tick <= 0;
}

extern Oc_Arena frame_arena;
extern Oc_Arena arena;
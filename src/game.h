#pragma once


#include "../external/clay/clay_renderer_raylib.h"
#include "../external/core.h"
#include "raylib.h"
#include "font_manager.h"

#include "pick_items.h"
#define STR_TO_CLAY_STRING(str) ({ string __s = (str); ((Clay_String) { .length = __s.len, .chars = __s.ptr }); })

typedef struct {
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

static inline int64_t timer_elapsed(Game_Timer* timer) {
    return max(timer->start - timer->tick, 0);
}

// returns value from 0.0 to 1.0, 0.0. being start of timer, 1.0 being end of timer
static inline float timer_interpolate(Game_Timer* timer) {
    return max(0.0f, (float)(timer->start - timer->tick) / (float)timer->start);
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

typedef struct {
    void (*init)(void);
    bool (*update)(void);
} Minigame;

extern Game_Parameters game_parameters;

extern Oc_Arena frame_arena;
extern Oc_Arena arena;

typedef struct {
    Shader background_shader;
} Game_Shaders;
extern Game_Shaders game_shaders;

#define make_cool_background(...) ({                                                                                                                                                                                                                                               \
    CustomLayoutElement* customBackgroundData = oc_arena_alloc(&frame_arena, sizeof(CustomLayoutElement));                                                                                                                                                                         \
    customBackgroundData->type = CUSTOM_LAYOUT_ELEMENT_TYPE_BACKGROUND;                                                                                                                                                                                                            \
    customBackgroundData->customData.background = (CustomLayoutElement_Background) { .shader = game_shaders.background_shader, .color1 = { 0.196f * 255.0f, 0.196f * 255.0f, 0.196f * 255.0f, 255.0f }, .color2 = { 0.184f * 255.0f, 0.184f * 255.0f, 0.184f * 255.0f, 255.0f }, __VA_ARGS__ }; \
    customBackgroundData;                                                                                                                                                                                                                                                          \
})

typedef struct {
    uint32_t items[4];
    bool used[4];
} Briefcase;

typedef struct {
    Item_Type item;
    Character_Type character;
} Item_Sold;

typedef struct {
    enum {
        GAME_STATE_TUTORIAL,
        GAME_STATE_SELECT_ITEMS,
        GAME_STATE_SELECT_ENCOUNTER,
        GAME_STATE_IN_ENCOUNTER,
        GAME_STATE_DAY_SUMMARY,
        GAME_STATE_PLAYTHROUGH_SUMMARY,
    } state;

    uint32_t next_state;

    Game_Timer transition_timer;
    Texture2D screenshot;

    Briefcase briefcase;
    Item_Type current_item;
    Character_Type current_character;
    void (*encounter)(void);

    uint8_t current_day;

    Item_Sold items_sold_today[4];
    Array(Item_Sold) prev_items_sold;
} Game_State;
extern Game_State game;

void game_go_to_state(uint32_t next_state);

extern void HandleClayErrors(Clay_ErrorData errorData);
extern Font_Manager* global_font_manager;

#define FONT_ROBOTO (1u)
#define FONT_ITIM   (2u)

void day_summary_update(void);

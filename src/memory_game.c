#include "raylib.h"
#include "rlgl.h"
#include "game.h"
#include "../external/clay/clay.h"
#include "../external/clay/clay_renderer_raylib.h"

static float timer;
static uint32_t game_round;
static Vector2 spawn_location;
static int final_location_y;
static float item_spacing = 230.0f;
static uint32_t num_items;
static float time_per_item = 0.7f;
static float extra_display_time = 3.0f;
static float solve_time = 10.0f;

static Texture2D bg_tex;
static int tex_width;
static int tex_height;
static Texture2D robot_light_tex;
static Texture2D robot_top_tex;
static Texture2D robot_center_tex;
static Texture2D robot_bottom_tex;

Enum(Memory_State, uint32_t,
    MEMORY_DISPLAYING,
    MEMORY_SOLVE,
    MEMORY_RESULT
);

static Memory_State state;

static Array(Item_Type) memory_game_items;
static Array(Item_Type) memory_game_items_truth;

// Called once when we enter into a new round
void memory_setup_round() {
    memory_game_items_truth.count = 0;

    for(uint32_t i = 0; i < 5; i++) {
        oc_array_append(&arena, &memory_game_items_truth, GetRandomValue(1, ITEM_COUNT - 1));
    }

    game_round++;
}

void memory_game_init() {
    state = MEMORY_DISPLAYING;
    timer = GetTime();
    game_round = 1;
    memory_setup_round();

    bg_tex = LoadTexture("resources/background.png");
    robot_light_tex = LoadTexture("resources/robot light.png");
    robot_top_tex = LoadTexture("resources/robot top.png");
    robot_center_tex = LoadTexture("resources/robot center.png");
    robot_bottom_tex = LoadTexture("resources/robot bottom.png");

    tex_width = robot_top_tex.width;
    tex_height = robot_top_tex.height;

    spawn_location = (Vector2) { 1920 / 2, 100 + tex_height * 0.5};
    final_location_y = 700;    
}

void memory_draw_items_displaying() {
    uint32_t n = memory_game_items_truth.count;
    float offset_x = (n / 2) * -item_spacing;

    for(uint32_t i = 0; i < n; i++) {
        Texture2D item_texture = item_data[memory_game_items_truth.items[i]].texture;
        Vector2 final_location = (Vector2) {
            (1920 / 2) + offset_x,
            final_location_y
        };

        offset_x += item_spacing;

        float time = GetTime() - timer;
        float interpolation = half_smoothstep(min((GetTime() - timer), 1.0));

        // Before this items turn
        if(time < i * time_per_item) {
            interpolation = 0;
        }

        // This items turn
        if(time > i * time_per_item && time < (i + 1) * time_per_item) {
            interpolation = half_smoothstep(min((fmod(time, time_per_item) / time_per_item), 1.0));
        }

        // Past this items turn
        if(time > (i + 1) * time_per_item) {
            interpolation = 1;
        }

        Vector2 location = (Vector2) {
            Lerp(spawn_location.x, final_location.x, interpolation),
            Lerp(spawn_location.y, final_location.y, half_smoothstep(half_smoothstep(interpolation)))
        };

        // Vector2 location = Vector2Lerp(spawn_location, final_location, interpolation);

        location.x -= item_texture.width * 0.5;
        
        DrawTextureV(item_texture, location, WHITE);
    }
}

// Handles all rearranging logic and drawing
void memory_draw_items_rearranging() {
    // Need to handle three different things

    // if the mouse is pressed and its close to an object

    // if the mouse is held and we are selecting an item

    // if the mouse is released and we are near an item

    // draw all the unselected objects


}

bool memory_game_update() {
    DrawTexture(bg_tex, 0, 0, WHITE);

    DrawTexture(robot_bottom_tex, 1920 / 2 - tex_width * 0.5, 50, WHITE);

    // Draw the items here

    switch(state) {
        case MEMORY_DISPLAYING: {
            memory_draw_items_displaying();
            DrawTexture(robot_light_tex, 1920 / 2 - tex_width * 0.5, 50, RED);
        } break;
        case MEMORY_SOLVE: {
            memory_draw_items_rearranging();
            DrawTexture(robot_light_tex, 1920 / 2 - tex_width * 0.5, 50, GREEN);
        } break;
    }

    float time = GetTime() - timer;

    // Check for transitions
    switch(state) {
        case MEMORY_DISPLAYING: {
            if(time > memory_game_items_truth.count * time_per_item + extra_display_time) {
                state = MEMORY_SOLVE;
                timer = GetTime();
            }
        } break;
        case MEMORY_SOLVE: {
            if(time > solve_time) {
                state = MEMORY_RESULT;
            }
        } break;
        case MEMORY_RESULT: {
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_SPACE)) {
                state = MEMORY_SOLVE;
                timer = GetTime();
                memory_setup_round();
            }
        } break;
    }

    DrawTexture(robot_top_tex, 1920 / 2 - tex_width * 0.5, 50, WHITE);
    DrawTexture(robot_center_tex, 1920 / 2 - tex_width * 0.5, 50, WHITE);
}

Minigame memory_game = {
    .init = memory_game_init,
    .update = memory_game_update,
};

// start of each round
// determine the number of items we want based on the round number
// create a sequence

// enter into display state
// show the sequence
// enter next state when timer runs out

// enter into solve state
// state ends when the timer runs out

// enter into display result/score state
// just show how many they got correct basically
// adjust their score also

// if we have done enough rounds exit
// otherwise setup for next state
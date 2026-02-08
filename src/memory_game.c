#include "raylib.h"
#include "rlgl.h"
#include "game.h"
#include "../external/clay/clay.h"
#include "../external/clay/clay_renderer_raylib.h"

static uint32_t score;
static float timer;
static uint32_t game_round;
static uint32_t num_rounds[4] = {2, 3, 4, 5};
static Vector2 spawn_location;
static int final_location_y;
static float item_spacing = 230.0f;
static float time_per_item = 0.7f;
static float extra_display_time = 3.0f;
static float solve_time_per_item = 2.0f;
static float timer_progress; // goes from 0 to 1
static float selection_radius = 115.0f; // item_spacing / 2;

static bool is_selecting;
static uint32_t selected_index;
static Vector2 initial_location;

static Texture2D bg_tex;
static int tex_width;
static int tex_height;
static Texture2D robot_light_tex;
static Texture2D robot_top_tex;
static Texture2D robot_center_tex;
static Texture2D robot_bottom_tex;

static Shader progressShader;

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
    memory_game_items.count = 0;

    // TODO: Vary the number of items based on day and round
    for(uint32_t i = 0; i < num_rounds[3] + game_round; i++) {
        oc_array_append(&arena, &memory_game_items_truth, GetRandomValue(1, ITEM_COUNT - 1));
        oc_array_append(&arena, &memory_game_items, memory_game_items_truth.items[i]);
    }

    // TODO: copy the truth array into the normal array and shuffle it
    for(int32_t i = memory_game_items.count - 1; i > 0; i--) {
        uint32_t j = GetRandomValue(0, i);
        Item_Type temp = memory_game_items.items[j];
        memory_game_items.items[j] = memory_game_items.items[i];
        memory_game_items.items[i] = temp;
    }
    
    game_round++;
}

void memory_game_init() {
    score = 0;
    state = MEMORY_DISPLAYING;
    timer = GetTime();
    game_round = 0;
    memory_setup_round();
    is_selecting = false;

    bg_tex = LoadTexture("resources/background.png");
    robot_light_tex = LoadTexture("resources/robot light.png");
    robot_top_tex = LoadTexture("resources/robot top.png");
    robot_center_tex = LoadTexture("resources/robot center.png");
    robot_bottom_tex = LoadTexture("resources/robot bottom.png");

    progressShader = LoadShader(NULL, "resources/memory_progress_shader.fs");

    tex_width = robot_top_tex.width;
    tex_height = robot_top_tex.height;

    spawn_location = (Vector2) { 1920 / 2, 100 + tex_height * 0.5};
    final_location_y = 700;    
}

void memory_draw_items_displaying() {
    uint32_t n = memory_game_items_truth.count;
    float offset_x = (n / 2) * -item_spacing;\

    timer_progress = 1.0 - (GetTime() - timer) / (extra_display_time + n * time_per_item);

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

    // TODO: Draw minigame prompt here (Remember the order of the items)
    // Also draw the timer bar

    CLAY_AUTO_ID({
        .floating = { .offset = { 0, -50 }, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_BOTTOM, CLAY_ATTACH_POINT_CENTER_BOTTOM } },
        .layout = {
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            .padding = {10, 25, 0, 0},
            .sizing = {
                .width = CLAY_SIZING_FIT(),
                .height = CLAY_SIZING_FIT()
            },
        },
        .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
        .custom = { .customData = make_cool_background() },
        .cornerRadius = CLAY_CORNER_RADIUS(6),
    }) {
        CLAY_TEXT((CLAY_STRING("Memorize the order of the items")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
    }
}

// Handles all rearranging logic and drawing
void memory_draw_items_rearranging() {
    timer_progress = 1.0 - (GetTime() - timer) / (memory_game_items_truth.count * solve_time_per_item);

    // Need to handle three different things

    Vector2 mouse = (Vector2) { (float)GetMouseX(), (float)GetMouseY() };
    uint32_t n = memory_game_items.count;
    float offset_x = (n / 2) * -item_spacing;

    // if the mouse is pressed and its close to an object
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for(uint32_t i = 0; i < n; i++) {
            Texture2D item_texture = item_data[memory_game_items.items[i]].texture;

            Vector2 item_location = (Vector2) {
                (1920 / 2) + offset_x,
                final_location_y + item_texture.height * 0.5
            };

            offset_x += item_spacing;

            float dist = Vector2Distance(mouse, item_location);

            if(dist < selection_radius) {
                printf("Here");
                is_selecting = true;
                selected_index = i;
                initial_location = mouse;
                break;
            }
        }
    }

    // if the mouse is released and we are near an item
    if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && is_selecting) {
        offset_x = (n / 2) * -item_spacing;

        // also check for a swap
        for(uint32_t i = 0; i < n; i++) {
            Texture2D item_texture = item_data[memory_game_items.items[i]].texture;

            Vector2 item_location = (Vector2) {
                (1920 / 2) + offset_x,
                final_location_y + item_texture.height * 0.5
            };

            offset_x += item_spacing;

            float dist = Vector2Distance(mouse, item_location);

            if(dist < selection_radius) {
                Item_Type temp = memory_game_items.items[i];
                memory_game_items.items[i] = memory_game_items.items[selected_index];
                memory_game_items.items[selected_index] = temp;
            }
        }

        is_selecting = false;
    }

    // draw all the objects
    offset_x = (n / 2) * -item_spacing;

    for(uint32_t i = 0; i < n; i++) {
        Texture2D item_texture = item_data[memory_game_items.items[i]].texture;

        Vector2 item_location = (Vector2) {
            (1920 / 2) + offset_x - item_texture.width * 0.5,
            final_location_y
        };

        Vector2 item_location2 = (Vector2) {
            (1920 / 2) + offset_x,
            final_location_y + item_texture.height * 0.5
        };

        offset_x += item_spacing;

        if(is_selecting && selected_index == i) {
            continue;
        }

        DrawCircle(item_location2.x, item_location2.y, selection_radius - 10, (Color) {77, 107, 226, 255 * (0.3 + 0.5 * (sin(GetTime() * 2.0) * 0.5 + 0.5))});
        DrawTextureV(item_texture, item_location, WHITE);
    }

    if(is_selecting) {
        offset_x = (n / 2) * -item_spacing + item_spacing * selected_index;

        Texture2D item_texture = item_data[memory_game_items.items[selected_index]].texture;

        Vector2 item_location = (Vector2) {
            (1920 / 2) + offset_x - item_texture.width * 0.5,
            final_location_y
        };

        Vector2 delta = Vector2Subtract(mouse, initial_location);
        item_location = Vector2Add(item_location, delta);

        DrawTextureV(item_texture, item_location, WHITE);
    }

    CLAY_AUTO_ID({
        .floating = { .offset = { 0, -50 }, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_BOTTOM, CLAY_ATTACH_POINT_CENTER_BOTTOM } },
        .layout = {
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            .padding = {10, 25, 0, 0},
            .sizing = {
                .width = CLAY_SIZING_FIT(),
                .height = CLAY_SIZING_FIT()
            },
        },
        .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
        .custom = { .customData = make_cool_background() },
        .cornerRadius = CLAY_CORNER_RADIUS(6),
    }) {
        CLAY_TEXT((CLAY_STRING("Left Click and Drag to Reorder the items to their original order")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
    }
}

void memory_draw_items_result() {
    timer_progress = 1.0;

    uint32_t n = memory_game_items.count;
    float offset_x = (n / 2) * -item_spacing;

    int num_correct = 0;

    for(uint32_t i = 0; i < n; i++) {
        Texture2D item_texture = item_data[memory_game_items.items[i]].texture;

        Vector2 item_location = (Vector2) {
            (1920 / 2) + offset_x - item_texture.width * 0.5,
            final_location_y
        };

        Vector2 item_location2 = (Vector2) {
            (1920 / 2) + offset_x,
            final_location_y + item_texture.height * 0.5
        };

        offset_x += item_spacing;

        if(memory_game_items.items[i] == memory_game_items_truth.items[i]) {
            num_correct++;
        }

        Color color = memory_game_items.items[i] != memory_game_items_truth.items[i] ? (Color) {255, 0, 0, 100} : (Color) {0, 255, 0, 100};

        DrawCircle(item_location2.x, item_location2.y, selection_radius - 10, color);
        DrawTextureV(item_texture, item_location, WHITE);
    }

    // TODO: Draw minigame prompt here (Rearrange the items by dragging)
    CLAY_AUTO_ID({
        .floating = { .offset = { 0, -50 }, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_BOTTOM, CLAY_ATTACH_POINT_CENTER_BOTTOM } },
        .layout = {
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            .padding = {10, 25, 0, 0},
            .sizing = {
                .width = CLAY_SIZING_FIT(),
                .height = CLAY_SIZING_FIT()
            },
        },
        .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
        .custom = { .customData = make_cool_background() },
        .cornerRadius = CLAY_CORNER_RADIUS(6),
    }) {
        CLAY_TEXT((oc_format(&frame_arena, "{}/{} correct. Press Space to Continue", num_correct, n)), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
    }

    // Also draw the timer bar
}

bool memory_game_update() {
    game_objective_widget(lit("Play the memory ordering game"));

    DrawTexture(bg_tex, 0, 0, WHITE);

    DrawTexture(robot_bottom_tex, 1920 / 2 - tex_width * 0.5, 50, WHITE);

    // Draw the items here

    switch(state) {
        case MEMORY_DISPLAYING: {
            memory_draw_items_displaying();
            DrawTexture(robot_light_tex, 1920 / 2 - tex_width * 0.5, 50, (Color) {255, 0, 0, 255});
        } break;
        case MEMORY_SOLVE: {
            memory_draw_items_rearranging();
            DrawTexture(robot_light_tex, 1920 / 2 - tex_width * 0.5, 50, (Color) {0, 255, 0, 255});
        } break;
        case MEMORY_RESULT: {
            memory_draw_items_result();
            DrawTexture(robot_light_tex, 1920 / 2 - tex_width * 0.5, 50, (Color) {0, 0, 255, 255});
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
            if(time > memory_game_items_truth.count * solve_time_per_item) {
                state = MEMORY_RESULT;
                is_selecting = false;
                
                // Calculate score once on transition
                int num_correct = 0;
                for(uint32_t i = 0; i < memory_game_items.count; i++) {
                    if(memory_game_items.items[i] == memory_game_items_truth.items[i]) {
                        num_correct++;
                    }
                }
                score += num_correct * 100;
            }
        } break;
        case MEMORY_RESULT: {
            if(IsKeyDown(KEY_SPACE)) {
                state = MEMORY_DISPLAYING;
                timer = GetTime();

                if(game_round == num_rounds[game.current_day]) {
                    return true;
                }

                memory_setup_round();
            }
        } break;
    }

    DrawTexture(robot_top_tex, 1920 / 2 - tex_width * 0.5, 50, WHITE);

    // Draw the progress bar
    int progressLoc = GetShaderLocation(progressShader, "progress");
    SetShaderValue(progressShader, progressLoc, &timer_progress, SHADER_UNIFORM_FLOAT);

    BeginShaderMode(progressShader);
        int width = 255;
        SetShapesTexture((Texture2D) { 0, width, 100, 0, 1 }, (Rectangle) {0, 0, width, 100});
        DrawRectangle(1920 / 2 - width / 2, 1080 / 2 - 200, width, 100, WHITE);
    EndShaderMode();

    DrawTexture(robot_center_tex, 1920 / 2 - tex_width * 0.5, 50, WHITE);

    // Draw the score
    CLAY_AUTO_ID({
        .floating = { .offset = { 16, 100 }, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_RIGHT_TOP, CLAY_ATTACH_POINT_RIGHT_TOP } },
        .layout = {
            .padding = {10, 25, 0, 0},
            .sizing = {
                .width = CLAY_SIZING_FIT(),
                .height = CLAY_SIZING_FIT()
            },
        },
        .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
        .custom = { .customData = make_cool_background() },
        .cornerRadius = CLAY_CORNER_RADIUS(6),
    }) {
        CLAY_TEXT((oc_format(&frame_arena, "Score: {}", score)), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
    }

    return false;
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

// TODO:
// Add text prompts
// For displaying: tell them to remember the order + also add a timer
// for solving: tell them to rearrange by dragging + also add a timer
// for result: show them their score + tell them to press space to advance

// add scoring logic
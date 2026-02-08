#include "raylib.h"
#include "rlgl.h"

#define OC_CORE_IMPLEMENTATION
#include "entity.h"
#include "game.h"
#include "encounter.h"
#include "dialog.h"
#include "characters.h"
#include "pick_items.h"
#include "font_manager.h"

#define CLAY_IMPLEMENTATION
#include "../external/clay/clay.h"
// #include "../external/clay/clay_renderer_raylib.c"

// #include "memory_game.h"

void memory_game_init();
void memory_game_update();

Oc_Arena arena = { 0 };
Oc_Arena frame_arena = { 0 };

void HandleClayErrors(Clay_ErrorData errorData) {
    // See the Clay_ErrorData struct for more information
    printf("%s", errorData.errorText.chars);
    switch(errorData.errorType) {
        default: break;
    }
}

static Game_Timer show_fail_timer;
static Texture2D bg_tex;
Game_Parameters game_parameters;

Game_State game = { 0 };
Game_Shaders game_shaders = { 0 };

Sound start_sounds[4];

extern Minigame memory_game;
extern Minigame smile_game;
extern Minigame shotgun_game;
extern Minigame rhythm_game;

static Sound failSound;
static Sound successSound;

static float score_needed = 0.7;

void game_go_to_state(uint32_t next_state) {
    switch (next_state) {
    // case GAME_STATE_TRANSITION: {
    //     Image screenshot = LoadImageFromScreen();
    //     game.screenshot = LoadTextureFromImage(screenshot);
    //     timer_init(&game.transition_timer, 2000);
    // } break;
    case GAME_STATE_SELECT_ITEMS: {
        choose_pickable();
    } break;
    case GAME_STATE_SELECT_ENCOUNTER: {
        pick_encounter();
    } break;
    case GAME_STATE_START_DAY: {
        memset(&game.items_sold_today, 0, sizeof(game.items_sold_today));
        game_go_to_state(GAME_STATE_SELECT_ENCOUNTER);
        return;
    } break;
    case GAME_STATE_IN_ENCOUNTER: {
        game.minigame_scores = 0.0f;
        game.minigame_count = 0;
        encounter_start(game.encounter);
    } break;
    case GAME_STATE_DONE_ENCOUNTER: {
        if (game.minigame_scores < score_needed) {
            PlaySound(failSound);
            timer_init(&show_fail_timer, 4000);
        } else {
            PlaySound(successSound);
            game.current_encounter++;
            game.briefcase.used[game.current_item_index] = 1;

            if (game.current_encounter >= 4) {
                game_go_to_state(GAME_STATE_DAY_SUMMARY);
            } else {
                game_go_to_state(GAME_STATE_SELECT_ENCOUNTER);
            }
            return;
        }
    } break;
    case GAME_STATE_DAY_SUMMARY: break;
    default: oc_assert(false);
    }

    game.state = next_state;
}

void game_update() {
    switch (game.state) {
    case GAME_STATE_SELECT_ITEMS: {
        pick_items_update();
    } break;
    case GAME_STATE_SELECT_ENCOUNTER: {
        pick_encounter_update();
    } break;
    case GAME_STATE_IN_ENCOUNTER: {
        // TODO: Move this into dialog or smth so we can scriptably change the background
        DrawTexture(bg_tex, 0, 0, WHITE);
        characters_draw(CHARACTERS_SALESMAN, CHARACTERS_LEFT);
        characters_draw(game.current_character, CHARACTERS_RIGHT);

        encounter_update();
        if (encounter_is_done()) {
            game_go_to_state(GAME_STATE_DONE_ENCOUNTER);
        }
    } break;
    case GAME_STATE_DAY_SUMMARY: {
        day_summary_update();
    } break;
    case GAME_STATE_DONE_ENCOUNTER: {
        if (game.minigame_scores < (float)game.minigame_count * 0.5f) {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_PERCENT(1),
                        .height = CLAY_SIZING_PERCENT(1),
                    },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
            }) {
                CLAY_TEXT(lit("They did not buy the item"), CLAY_TEXT_CONFIG({ .fontId = FONT_ROBOTO, .fontSize = 80, .textColor = {200, 30, 0, 255} }));
                CLAY_TEXT(lit("FAIL"), CLAY_TEXT_CONFIG({ .fontId = FONT_ROBOTO, .fontSize = 60, .textColor = {200, 30, 0, 255} }));
            }
            if (timer_update(&show_fail_timer)) {
                game_go_to_state(GAME_STATE_SELECT_ITEMS);
            }
        }
    } break;
    default: oc_assert(false);
    }
}

Clay_Arena global_clay_arena;
Font_Manager* global_font_manager;

int main(void)
{
    const int screenWidth = 1920;
    const int screenHeight = 1080;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Traveling Salesman Problems");
    InitAudioDevice();
    SetTargetFPS(0);

    failSound = LoadSound("resources/sounds/failure.wav");
    successSound = LoadSound("resources/sounds/success.wav");
    start_sounds[0] = LoadSound("resources/sounds/casino_a.mp3");
    start_sounds[1] = LoadSound("resources/sounds/casino_b.mp3");
    start_sounds[2] = LoadSound("resources/sounds/casino_c.mp3");
    start_sounds[3] = LoadSound("resources/sounds/casino_d.mp3");
    
    Camera2D camera = { 0 };
    camera.offset = (Vector2) { 0.0f, 0.0f };
    camera.zoom = 1.0f;

    
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clay_arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Initialize(clay_arena, (Clay_Dimensions) { screenWidth, screenHeight }, (Clay_ErrorHandler) { HandleClayErrors, NULL });
    
    game_shaders.background_shader = LoadShader(0, "resources/dialogbackground.fs");

    Font_Manager font_manager = {
        .arena = &arena,
        .font_paths = (const char *[]) {
            // [0] = "",
            [0] = "resources/Roboto-Light.ttf",
            [FONT_ROBOTO] = "resources/Roboto-Light.ttf",
            [FONT_ITIM]   = "resources/Itim.ttf",
        },
    };
    font_manager_get_font(&font_manager, 2, 25);
    global_font_manager = &font_manager;
    Clay_SetMeasureTextFunction(Raylib_MeasureText, global_font_manager);

    game_parameters = (Game_Parameters) {
        .screen_width = screenWidth,
        .screen_height = screenHeight,
    };

    oc_arena_alloc(&frame_arena, 1);

    {
        Oc_Arena_Save save = oc_arena_save(&frame_arena);

        dialog_init();
        characters_init();
        pick_items_init();
        pick_encounter_init();
        items_init();
        choose_pickable();

        oc_arena_restore(&frame_arena, save);
    }


    bg_tex = LoadTexture("resources/background.png");

    game_go_to_state(GAME_STATE_SELECT_ITEMS);

    game.items_sold_today[0] = (Item_Sold) {
        .item = ITEM_VACUUM,
        .character = CHARACTERS_NERD,
    };

    smile_game.init();
    shotgun_game.init();
    rhythm_game.init();
    memory_game.init();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        Oc_Arena_Save save = oc_arena_save(&frame_arena);
        game_parameters.screen_width = GetScreenWidth();
        game_parameters.screen_height = GetScreenHeight();


        if (game.current_prerender) game.current_prerender();

        Clay_SetLayoutDimensions((Clay_Dimensions) { game_parameters.screen_width, game_parameters.screen_height });
        Clay_SetPointerState((Clay_Vector2) { GetMouseX(), GetMouseY() }, IsMouseButtonDown(MOUSE_LEFT_BUTTON));
        Clay_UpdateScrollContainers(true, (Clay_Vector2) { GetMouseWheelMove(), 0.0 }, dt);

        Clay_BeginLayout();

        BeginDrawing();
            ClearBackground((Color){40, 40, 40, 255});
            BeginMode2D(camera);
                game_update();
                // pick_items_update();
                // smile_game.update();
                // shotgun_game.update();
                // rhythm_game.update();
                // memory_game.update();

                Clay_RenderCommandArray renderCommands = Clay_EndLayout();
                Clay_Raylib_Render(renderCommands, global_font_manager);
            EndMode2D();

        EndDrawing();

        oc_arena_restore(&frame_arena, save);
    }

    UnloadTexture(bg_tex);

    items_cleanup();
    pick_items_cleanup();
    characters_cleanup();
    dialog_cleanup();

    CloseWindow();

    return 0;
}
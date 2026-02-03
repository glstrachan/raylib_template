#include "raylib.h"
#include "rlgl.h"

#define OC_CORE_IMPLEMENTATION
#include "entity.h"
#include "game.h"
#include "encounter.h"
#include "dialog.h"
#include "characters.h"
#include "pick_items.h"

#define CLAY_IMPLEMENTATION
#include "../external/clay/clay.h"
#include "../external/clay/clay_renderer_raylib.c"
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

static Texture2D bg_tex;
Game_Parameters game_parameters;

Game_State game = { 0 };
Game_Shaders game_shaders = { 0 };

extern Minigame memory_game;
extern Minigame smile_game;

static Shader background_shader;

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
    case GAME_STATE_IN_ENCOUNTER: {
        encounter_start(game.encounter);
    } break;
    case GAME_STATE_DAY_SUMMARY:background_shader = LoadShader(0, "resources/dialogbackground.fs"); break;
    default: oc_assert(false);
    }

    game.state = next_state;
}

void game_update() {
    switch (game.state) {
    //     case GAME_STATE_TRANSITION: {
    //         if (timer_update(&game.transition_timer)) {
    //             game_go_to_state(game.next_state);
    //             break;
    //         }

    //         DrawTexture(game.screenshot, 0, 0, WHITE);
    //         float interp = timer_interpolate(&game.transition_timer);
    //         interp = interp * 2.0f - 1.0f;
    //         if (interp < 0.0f) interp = -interp;
    //         interp = 1.0f - interp;

    //     DrawRectangle(0, 0, game_parameters.screen_width, game_parameters.screen_height, (Color){ 0, 0, 0, 255 * interp });
    // } break;
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
        characters_draw(CHARACTERS_OLDLADY, CHARACTERS_RIGHT);

        encounter_update();
        if (encounter_is_done()) {
            game.current_minigame = &memory_game;

                // game.next_state = GAME_STATE_IN_MINIGAME;
                // game_go_to_state(GAME_STATE_TRANSITION);

            // TODO: go to show day summary
            // game_go_to_state(GAME_STATE_IN_MINIGAME);
            exit(0);
        }
    } break;
    case GAME_STATE_DAY_SUMMARY: {
        Oc_String_Builder sb;
        oc_sb_init(&sb, &frame_arena);
        wprint(&sb.writer, "Day {} Summary", game.current_day);
        string title = oc_sb_to_string(&sb);

        CLAY(CLAY_ID("DaySummary"), {
            .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER } },
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(0.5),
                    .height = CLAY_SIZING_PERCENT(0.6)

                    // .width = CLAY_SIZING_FIXED(100),
                    // .height = CLAY_SIZING_FIXED(100)
                },
                .padding = {40, 16, 30, 16},
                .childGap = 16
            },
            .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
            .custom = { .customData = make_cool_background() },
            .cornerRadius = CLAY_CORNER_RADIUS(16)
        }) {
            CLAY_TEXT(title, CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = 2, .textColor = {255, 255, 255, 255} }));
            CLAY_TEXT(oc_format(&frame_arena, "Sold {} items today", 4), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = 2, .textColor = {255, 255, 255, 255} }));
            CLAY(CLAY_ID("ItemsList"), {
                .layout = { .childGap = 16, .layoutDirection = CLAY_TOP_TO_BOTTOM, .padding = {50} },
            }) {
                for (int i = 0; i < 4; ++i) {
                    CLAY_AUTO_ID({
                        .layout = { .sizing = { .width = CLAY_SIZING_FIXED(200) }, .childAlignment = { .x = CLAY_ALIGN_X_CENTER }, .padding = {16, 16, 16, 16} },
                        .backgroundColor = {100, 100, 100, 255},
                    }) {
                        CLAY_TEXT(oc_format(&frame_arena, "Item {}", i), CLAY_TEXT_CONFIG({ .fontSize = 25, .fontId = 3, .textColor = {255, 255, 255, 255} }));
                    }
                }
            }

            CLAY_AUTO_ID({
                .layout = { .sizing = { .height = CLAY_SIZING_GROW() } }
            });
            CLAY(CLAY_ID("DialogContinue"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_PERCENT(1.0),
                        .height = CLAY_SIZING_FIXED(100)
                    },
                    .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_BOTTOM }
                },
                .backgroundColor = {200, 0, 0, 0},
            }) {
                CLAY(CLAY_ID("PickItemsStartDay"), {
                    .layout = {
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                        .padding = { .left = 20, .right = 20, .top = 10, .bottom = 10 },
                    },
                    .custom = {
                        .customData = Clay_Hovered() ?
                            make_cool_background(.color1 = { 214, 51, 0, 255 }, .color2 = { 222, 51, 0, 255 }) :
                            make_cool_background(.color1 = { 244, 51, 0, 255 }, .color2 = { 252, 51, 0, 255 })
                    },
                    .cornerRadius = CLAY_CORNER_RADIUS(40),
                    .border = { .width = { 3, 3, 3, 3, 0 }, .color = {148, 31, 0, 255} },
                }) {
                    CLAY_TEXT((CLAY_STRING("Next Day")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = 2, .textColor = {255, 255, 255, 255} }));
                    if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        game_go_to_state(GAME_STATE_SELECT_ITEMS);
                    }
                }
            }
        }
    } break;
    default: oc_assert(false);
    }
}

Clay_Arena global_clay_arena;
Font* global_clay_fonts;

int main(void)
{
    const int screenWidth = 1920;
    const int screenHeight = 1080;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(screenWidth, screenHeight, "Travelling Salesman Problems");
    InitAudioDevice();
    SetTargetFPS(240);
    
    Camera2D camera = { 0 };
    camera.offset = (Vector2) { 0.0f, 0.0f };
    camera.zoom = 1.0f;
    
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Initialize(arena, (Clay_Dimensions) { screenWidth, screenHeight }, (Clay_ErrorHandler) { HandleClayErrors, NULL });
    
    game_shaders.background_shader = LoadShader(0, "resources/dialogbackground.fs");
    
    Font font = LoadFontEx("resources/Roboto-Light.ttf", 60, NULL, 0);
    Font dialog_font = LoadFontEx("resources/Itim.ttf", 40, NULL, 0);
    Font dialog_font_big = LoadFontEx("resources/Itim.ttf", 60, NULL, 0);
    Font dialog_font_small = LoadFontEx("resources/Itim.ttf", 25, NULL, 0);
    Font fonts[] = {
        font,
        dialog_font,
        dialog_font_big,
        dialog_font_small
    };
    global_clay_fonts = fonts;
    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);

    game_parameters = (Game_Parameters) {
        .neutral_font = font,
        .dialog_font = dialog_font,
        .dialog_font_big = dialog_font_big,
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
        // extern Encounter sample_encounter_;
        // encounter_start(&sample_encounter_);

        oc_arena_restore(&frame_arena, save);
    }


    bg_tex = LoadTexture("resources/background.png");

    // game.state = GAME_STATE_SELECT_ENCOUNTER;
    game_go_to_state(GAME_STATE_DAY_SUMMARY);
    game.current_minigame = &smile_game;
    // game_go_to_state(GAME_STATE_IN_MINIGAME);
    // game.state = GAME_STATE_IN_MINIGAME;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        Oc_Arena_Save save = oc_arena_save(&frame_arena);
        game_parameters.screen_width = GetScreenWidth();
        game_parameters.screen_height = GetScreenHeight();

        Clay_SetLayoutDimensions((Clay_Dimensions) { game_parameters.screen_width, game_parameters.screen_height });
        Clay_SetPointerState((Clay_Vector2) { GetMouseX(), GetMouseY() }, IsMouseButtonDown(MOUSE_LEFT_BUTTON));
        Clay_UpdateScrollContainers(true, (Clay_Vector2) { GetMouseWheelMove(), 0.0 }, dt);

        Clay_BeginLayout();

        BeginDrawing();
            ClearBackground((Color){40, 40, 40, 255});
            BeginMode2D(camera);
                pick_items_update();
                // game_update();
                Clay_RenderCommandArray renderCommands = Clay_EndLayout();
                Clay_Raylib_Render(renderCommands, fonts);
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
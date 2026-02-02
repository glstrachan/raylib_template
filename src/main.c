#include "raylib.h"
#include "rlgl.h"

#define OC_CORE_IMPLEMENTATION
#include "entity.h"
#include "game.h"
#include "dialog.h"
#include "characters.h"

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

static struct {
    enum {
        GAME_STATE_TRANSITION,
        GAME_STATE_IN_DIALOG,
        GAME_STATE_IN_MINIGAME
    } state;
    uint32_t next_state;
    Minigame* current_minigame;

    Game_Timer transition_timer;
    Texture2D screenshot;
} game;

extern Minigame memory_game;
extern Minigame smile_game;

void game_go_to_state(uint32_t next_state) {
    switch (next_state) {
    case GAME_STATE_TRANSITION: {
        Image screenshot = LoadImageFromScreen();
        game.screenshot = LoadTextureFromImage(screenshot);
        timer_init(&game.transition_timer, 2000);
    } break;
    case GAME_STATE_IN_DIALOG: {
        dialog_init();
    } break;
    case GAME_STATE_IN_MINIGAME: {
        game.current_minigame->init();
    } break;
    default: oc_assert(false);
    }
    game.state = next_state;
}

void game_update() {
    switch (game.state) {
    case GAME_STATE_TRANSITION: {
        if (timer_update(&game.transition_timer)) {
            game_go_to_state(game.next_state);
            break;
        }

        DrawTexture(game.screenshot, 0, 0, WHITE);
        float interp = timer_interpolate(&game.transition_timer);
        interp = interp * 2.0f - 1.0f;
        if (interp < 0.0f) interp = -interp;
        interp = 1.0f - interp;

        DrawRectangle(0, 0, game_parameters.screen_width, game_parameters.screen_height, (Color){ 0, 0, 0, 255 * interp });
    } break;
    case GAME_STATE_IN_DIALOG: {
        // TODO: Move this into dialog or smth so we can scriptably change the background
        DrawTexture(bg_tex, 0, 0, WHITE);
        characters_draw(CHARACTERS_SALESMAN, CHARACTERS_LEFT);
        characters_draw(CHARACTERS_OLDLADY, CHARACTERS_RIGHT);

        dialog_update();
        if (dialog_is_done()) {
            game.current_minigame = &memory_game;

            // game.next_state = GAME_STATE_IN_MINIGAME;
            // game_go_to_state(GAME_STATE_TRANSITION);

            game_go_to_state(GAME_STATE_IN_MINIGAME);
        }
    } break;
    case GAME_STATE_IN_MINIGAME: {
        game.current_minigame->update();
    } break;
    default: oc_assert(false);
    }
}

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
    Clay_Initialize(arena, (Clay_Dimensions) { screenWidth, screenHeight }, (Clay_ErrorHandler) { HandleClayErrors });
    
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
    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);

    game_parameters = (Game_Parameters) {
        .neutral_font = font,
        .dialog_font = dialog_font,
        .dialog_font_big = dialog_font_big,
        .screen_width = screenWidth,
        .screen_height = screenHeight,
    };

    dialog_init();
    characters_init();

    dialog_play(sample_dialog);

    bg_tex = LoadTexture("resources/background.png");

    oc_arena_alloc(&frame_arena, 1);

    // game.state = GAME_STATE_IN_DIALOG;
    // game.current_minigame = &smile_game;
    // game_go_to_state(GAME_STATE_IN_MINIGAME);
    // game.state = GAME_STATE_IN_MINIGAME;

    void start_dialog();
    void do_dialog_frame();

    start_dialog();

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
                do_dialog_frame();
                // game_update();
                Clay_RenderCommandArray renderCommands = Clay_EndLayout();
                Clay_Raylib_Render(renderCommands, fonts);
            EndMode2D();

        EndDrawing();

        oc_arena_restore(&frame_arena, save);
    }

    UnloadTexture(bg_tex);

    characters_cleanup();
    dialog_cleanup();

    CloseWindow();

    return 0;
}
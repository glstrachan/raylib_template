#include "raylib.h"
#include "rlgl.h"

#define OC_CORE_IMPLEMENTATION
#include "entity.h"
#include "game.h"

#define CLAY_IMPLEMENTATION
#include "../external/clay/clay.h"
#include "../external/clay/clay_renderer_raylib.c"
// #include "memory_game.h"

void memory_game_init();
void memory_game_update();
void dialog_update(void);

Oc_Arena arena = { 0 };
Oc_Arena frame_arena = { 0 };

void HandleClayErrors(Clay_ErrorData errorData) {
    // See the Clay_ErrorData struct for more information
    printf("%s", errorData.errorText.chars);
    switch(errorData.errorType) {
        default: break;
    }
}

Game_Parameters game_parameters;

int main(void)
{
    const int screenWidth = 1920;
    const int screenHeight = 1080;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(screenWidth, screenHeight, "House Demo");
    SetTargetFPS(240);
    
    Camera2D camera = { 0 };
    camera.offset = (Vector2) { 0.0f, 0.0f };
    camera.zoom = 1.0f;
    
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Initialize(arena, (Clay_Dimensions) { screenWidth, screenHeight }, (Clay_ErrorHandler) { HandleClayErrors });
    
    Font font = LoadFontEx("resources/Roboto-Light.ttf", 60, NULL, 0);
    Font dialog_font = LoadFontEx("resources/chihaya.ttf", 60, NULL, 0);

    game_parameters = (Game_Parameters) {
        .neutral_font = font,
        .screen_width = screenWidth,
        .screen_height = screenHeight,
    };
    memory_game_init();

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

                memory_game_update();
                DrawRectangle(game_parameters.screen_width / 2 - game_parameters.screen_width * 0.3, game_parameters.screen_height - 200, game_parameters.screen_width * 0.6, 100, RED);
                dialog_update();

            EndMode2D();

        EndDrawing();

        Clay_RenderCommandArray renderCommands = Clay_EndLayout();
        Font fonts[] = {
            font,
            dialog_font,
        };
        Clay_Raylib_Render(renderCommands, fonts);

        oc_arena_restore(&frame_arena, save);
    }

    CloseWindow();

    return 0;
}
#include "game.h"

static Texture2D tv_texture;
static RenderTexture2D tv_game;
static Shader tv_shader;

static Vector2 position;

void rhythm_game_prerender(void) {
    Vector2 velocity = { 0, 0 };

    const float SPEED = 300.0f;
    if (IsKeyDown(KEY_LEFT)) {
        velocity.x = -SPEED * GetFrameTime();
    } else if (IsKeyDown(KEY_RIGHT)) {
        velocity.x = SPEED * GetFrameTime();
    }

    if (IsKeyDown(KEY_UP)) {
        velocity.y = SPEED * GetFrameTime();
    } else if (IsKeyDown(KEY_DOWN)) {
        velocity.y = -SPEED * GetFrameTime();
    }

    position = Vector2Add(position, velocity);

    Camera2D camera = { 0 };
    camera.offset = (Vector2) { 0.0f, 0.0f };
    camera.zoom = 1.0f;
    BeginTextureMode(tv_game);
    ClearBackground(RED);
        BeginMode2D(camera);
            Clay_BeginLayout();
                DrawRectangleV(position, (Vector2) { 100, 100 }, WHITE);
            // DrawRectangle(700, 500, 100, 100, WHITE);
            Clay_RenderCommandArray renderCommands = Clay_EndLayout();
            Clay_Raylib_Render(renderCommands, global_font_manager);
        EndMode2D();
    EndTextureMode();
}

void rhythm_game_init(void) {
    tv_texture = LoadTexture("resources/tv.png");
    tv_game = LoadRenderTexture(800, 600);
    tv_shader = LoadShader(NULL, "resources/tv_shader.glsl");
    game.current_prerender = rhythm_game_prerender;
    position = (Vector2) {0, 0};
}

bool rhythm_game_update(void) {
    Rectangle observerSource = { 0.0f, 0.0f, (float)tv_game.texture.width, (float)tv_game.texture.height };
    Rectangle observerDest = {
        101, 99,
        926, 770,
    };

    observerDest.x += game_parameters.screen_width / 2 - tv_texture.width / 2;
    observerDest.y += game_parameters.screen_height / 2 - tv_texture.height / 2;

    BeginShaderMode(tv_shader);

        float time = GetTime();
        SetShaderValue(tv_shader, GetShaderLocation(tv_shader, "iTime"), &time, SHADER_UNIFORM_FLOAT);

        Vector2 resolution = {GetScreenWidth(), GetScreenHeight()};
        SetShaderValue(tv_shader, GetShaderLocation(tv_shader, "screenResolution"), &resolution, SHADER_UNIFORM_VEC2);
        Vector2 elementResolution = {observerDest.width, observerDest.height};
        SetShaderValue(tv_shader, GetShaderLocation(tv_shader, "elementResolution"), &elementResolution, SHADER_UNIFORM_VEC2);
        Vector2 elementPosition = {observerDest.x, observerDest.y};
        SetShaderValue(tv_shader, GetShaderLocation(tv_shader, "elementPosition"), &elementPosition, SHADER_UNIFORM_VEC2);

        SetShapesTexture((Texture2D) { 0, observerSource.width, observerSource.height, 0, 1 }, observerSource);
        // DrawTexture(tv_game.texture, 101, 99, WHITE);

        DrawTexturePro(tv_game.texture, observerSource, observerDest, (Vector2) {0, 0}, 0.0f, WHITE);
    EndShaderMode();

    // DrawRectangle(1820, 980, 100, 100, RED);
    DrawTexture(tv_texture, game_parameters.screen_width / 2 - tv_texture.width / 2, game_parameters.screen_height / 2 - tv_texture.height / 2, WHITE);
    // DrawTextureV(tv_texture, (Vector2) { observerDest.x, observerDest.y }, WHITE);

    game_objective_widget(lit("Complete the game to earn your respect!"));

    return false;
}

Minigame rhythm_game = {
    .init = rhythm_game_init,
    .update = rhythm_game_update,
};

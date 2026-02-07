#include "game.h"

static Texture2D tv_texture;
static Texture2D space_texture;
static Texture2D laser_canon_body;
static Texture2D laser_canon_barrel;
static Texture2D laser_canon_wheel;
static RenderTexture2D tv_game;
static Shader tv_shader;

static int point_locked = -1;
static int points_count = 0;
static Vector2* points;
static Vector2 points_[1024];
static bool do_create_points = false;
static int current_shot_point_index;

static Game_Timer beam_timer;

static float current_canon_angle;
static enum {
    STATE_IDLE,
    STATE_BEAM,
} current_state = STATE_IDLE;

static const struct { int point_count; Vector2* points; } DEFINED_POINTS[] = {
{
    4,
    (Vector2[]) {
        { 225.500, 128.500 },
        { 160.500, 265.500 },
        { 567.500, 140.500 },
        { 634.500, 294.500 },
    }
},
{
    4,
    (Vector2[]) {
        { 151.500, 71.500 },
        { 584.500, 66.500 },
        { 375.500, 70.500 },
        { 284.500, 135.500 },
        { 472.500, 131.500 },
    }
},
{
    4,
    (Vector2[]) {
        { 147.500, 89.500 },
        { 619.500, 94.500 },
    }
},
{
    4,
    (Vector2[]) {
        { 169.500, 89.500 },
        { 380.500, 242.500 },
        { 586.500, 86.500 },
        { 374.500, 75.500 },
    }
},
{
    4,
    (Vector2[]) {
        { 119.500, 93.500 },
        { 119.500, 343.500 },
        { 648.500, 359.500 },
        { 650.500, 86.500 },
    }
},
{
    4,
    (Vector2[]) {
        { 133.500, 64.500 },
        { 273.500, 116.500 },
        { 440.500, 169.500 },
        { 632.500, 259.500 },
    }
},
{
    4,
    (Vector2[]) {
        { 158.500, 76.500 },
        { 290.500, 128.500 },
        { 421.500, 182.500 },
        { 569.500, 247.500 },
        { 567.500, 53.500 },
        { 422.500, 44.500 },
        { 281.500, 236.500 },
        { 150.500, 234.500 },
    }
},
{
    4,
    (Vector2[]) {
        { 182.500, 360.500 },
        { 587.500, 377.500 },
        { 391.500, 211.500 },
    }
},
{
    4,
    (Vector2[]) {
        { 149.500, 293.500 },
        { 290.500, 292.500 },
        { 438.500, 290.500 },
        { 626.500, 292.500 },
    }
},
{
    4,
    (Vector2[]) {
        { 314.500, 267.500 },
        { 429.500, 269.500 },
        { 360.500, 328.500 },
        { 376.500, 208.500 },
    }
},
{
    4,
    (Vector2[]) {
        { 300.500, 246.500 },
        { 451.500, 243.500 },
        { 297.500, 352.500 },
        { 446.500, 342.500 },
    }
},
};

static void set_random_points(void) {
    int index = GetRandomValue(0, oc_len(DEFINED_POINTS) - 1);
    points = DEFINED_POINTS[index].points;
    points_count = DEFINED_POINTS[index].point_count;
}

void rhythm_game_prerender(void) {
    const float POINT_RADIUS = 20.0f;

    if (do_create_points) {
        if (IsKeyPressed(KEY_S)) {
            print("{{\n");
            print("    4,\n");
            print("    (Vector2[]) {{\n");
            for (int i = 0; i < points_count; i++) {
            print("        {{ {3}, {3} },\n", points[i].x, points[i].y);
            }
            print("    }\n");
            print("},\n");
            points_count = 0;
        }

        Vector2 texture_screen_pos = { 100.0f + game_parameters.screen_width/2.0f - tv_texture.width/2.0f, 96.0f + game_parameters.screen_height/2.0f - tv_texture.height/2.0f };
        Vector2 mouse = { GetMouseX() - texture_screen_pos.x, GetMouseY() - texture_screen_pos.y };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            for (int i = 0; i < points_count; i++) {
                if (Vector2Distance(points[i], mouse) < POINT_RADIUS) {
                    point_locked = i;
                    goto DONE_MOUSE_PRESSED;
                }
            }
            point_locked = points_count;
            points[points_count++] = mouse;
        }
    DONE_MOUSE_PRESSED: (void)0;

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && point_locked) {
            points[point_locked] = mouse;
        }
    }

    if (IsKeyPressed(KEY_SPACE)) {
        timer_init(&beam_timer, 100);
        current_state = STATE_BEAM;
    }

    switch (current_state) {
    case STATE_BEAM: {
        if (timer_update(&beam_timer)) {
            current_shot_point_index++;
            if (current_shot_point_index >= points_count) {
                current_shot_point_index = 0;
                set_random_points();
            }
            current_state = STATE_IDLE;
        }
    } break;
    default: break;
    }


    Camera2D camera = { 0 };
    camera.offset = (Vector2) { 0.0f, 0.0f };
    camera.zoom = 1.0f;
    BeginTextureMode(tv_game);
    ClearBackground(RED);
        BeginMode2D(camera);
            Clay_BeginLayout();
                float t = 0.0f;
                DrawTexturePro(space_texture, (Rectangle) { (1920 - (1080 * 4.0f / 3.0f)) / 2.0f * t, 0.0f, 1080 * 4.0f / 3.0f, 1080.0f }, (Rectangle) { 0.0f, 0.0f, 800.0f, 600.0f }, (Vector2) {0, 0}, 0.0f, WHITE);

                Vector2 canon_pivot = {
                    320+laser_canon_body.width / 2.0f,
                    600.0f - 95.0f,
                };
                Vector2 diff = Vector2Subtract(points[current_shot_point_index], canon_pivot);
                float desired_angle = atan2f(diff.y, diff.x) * 180 / PI + 90.0f;
                current_canon_angle = Lerp(current_canon_angle, desired_angle, GetFrameTime() * 100.0f);

                switch (current_state) {
                case STATE_BEAM: {
                    float interp = timer_interpolate(&beam_timer);
                    DrawRectanglePro((Rectangle) {
                        canon_pivot.x + diff.x * interp, canon_pivot.y + diff.y * interp,
                        8.0f, 40.0f,
                    }, (Vector2) { 2.0f, 20.0f}, current_canon_angle, ORANGE);
                } break;
                default: break;
                }

                DrawTexturePro(laser_canon_barrel,
                    (Rectangle) {0,0, laser_canon_barrel.width,laser_canon_barrel.height},
                    (Rectangle) {
                        canon_pivot.x, canon_pivot.y,

                        laser_canon_barrel.width,laser_canon_barrel.height
                    },
                    (Vector2) {laser_canon_barrel.width/2.0f,90.0f}, current_canon_angle, WHITE
                );
                DrawTexture(laser_canon_body, 320, 600 - laser_canon_body.height, WHITE);
                DrawTexturePro(laser_canon_wheel,
                    (Rectangle) {0,0, laser_canon_wheel.width,laser_canon_wheel.height},
                    (Rectangle) {
                        canon_pivot.x, canon_pivot.y,
                        laser_canon_wheel.width,laser_canon_wheel.height
                    },
                    (Vector2) {laser_canon_wheel.width/2.0f,laser_canon_wheel.height/2.0f}, current_canon_angle, WHITE
                );

                if (do_create_points) {
                    for (int i = 0; i < points_count; i++) {
                        DrawCircleV(points[i], POINT_RADIUS, RED);
                        DrawRing(points[i], POINT_RADIUS - 3, POINT_RADIUS, 0, 360, 100, ((Color) {0, 0, 0, 255}));
                    }
                } else {
                    for (int i = current_shot_point_index; i < points_count; i++) {
                        DrawCircleV(points[i], POINT_RADIUS, RED);
                        DrawRing(points[i], POINT_RADIUS - 3, POINT_RADIUS, 0, 360, 100, ((Color) {0, 0, 0, 255}));
                    }
                }

            Clay_RenderCommandArray renderCommands = Clay_EndLayout();
            Clay_Raylib_Render(renderCommands, global_font_manager);
        EndMode2D();
    EndTextureMode();
}

void rhythm_game_init(void) {
    tv_texture = LoadTexture("resources/tv.png");
    space_texture = LoadTexture("resources/background_space.png");
    laser_canon_body = LoadTexture("resources/laser_canon_body.png");
    laser_canon_barrel = LoadTexture("resources/laser_canon_barrel.png");
    laser_canon_wheel = LoadTexture("resources/laser_canon_wheel.png");

    tv_game = LoadRenderTexture(800, 600);
    tv_shader = LoadShader(NULL, "resources/tv_shader.glsl");
    game.current_prerender = rhythm_game_prerender;
    current_shot_point_index = 0;
    set_random_points();
    current_canon_angle = 0.0f;
}

bool rhythm_game_update(void) {
    Rectangle observerSource = { 0.0f, 0.0f, (float)tv_game.texture.width, -(float)tv_game.texture.height };
    Rectangle observerDest = {
        100, 96,
        800, 602,
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

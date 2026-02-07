#include "game.h"

static Texture2D tv_texture;
static Texture2D space_texture;
static Texture2D laser_canon_body;
static Texture2D laser_canon_barrel;
static Texture2D laser_canon_wheel;
static RenderTexture2D tv_game;
static Shader tv_shader;

static Sound katana1, knife, stabbing;

static int point_locked = -1;
static int points_count = 0;
static Vector2* points;
static Vector2 points_[1024];
static bool do_create_points = false;
static int current_shot_point_index;

static Game_Timer beam_timer;

static bool is_shooting = false;
static float current_canon_angle;
static Game_Timer wait_timer;
static int wait_next_state;
static enum {
    STATE_WAIT,
    STATE_POPIN,
    STATE_PLAYBACK,
    STATE_BEAM,
} current_state = STATE_POPIN;

typedef struct {
    int count;
    uint8_t notes[8];
} Rhythm_Sequence;

#define EIGTH_NOTE(beat, sub_beat) [(beat) * 2 + (sub_beat)] = 1
#define QUARTER_NOTE(beat) [(beat) * 2] = 1
#define HALF_NOTE(beat) [(beat) * 4] = 1

static Rhythm_Sequence* current_rhythm_sequence;
static int rhythm_sequence_note_index, rhythm_sequence_beat_index;
static Game_Timer rhythm_sequence_timer;
static Rhythm_Sequence rhythm_sequences[] = {
    { 4, { QUARTER_NOTE(0), QUARTER_NOTE(1), QUARTER_NOTE(2), QUARTER_NOTE(3) } },
    { 4, { QUARTER_NOTE(0), QUARTER_NOTE(1), EIGTH_NOTE(2, 0), EIGTH_NOTE(2, 1) } },
    { 5, { EIGTH_NOTE(0, 0), EIGTH_NOTE(0, 1), QUARTER_NOTE(2), EIGTH_NOTE(3, 0), EIGTH_NOTE(3, 1) } },
    { 8, { EIGTH_NOTE(0, 0), EIGTH_NOTE(0, 1), EIGTH_NOTE(1, 0), EIGTH_NOTE(1, 1), EIGTH_NOTE(2, 0), EIGTH_NOTE(2, 1), EIGTH_NOTE(3, 0), EIGTH_NOTE(3, 1) } },
};
static Rhythm_Sequence rhythm_sequences_shuffled[oc_len(rhythm_sequences)];

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
    5,
    (Vector2[]) {
        { 151.500, 71.500 },
        { 584.500, 66.500 },
        { 375.500, 70.500 },
        { 284.500, 135.500 },
        { 472.500, 131.500 },
    }
},
{
    2,
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
    8,
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
    3,
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
    while (true) {
        // for(int32_t i = oc_len(rhythm_sequences_shuffled) - 1; i >= 0; i--) {
        //     uint32_t j = GetRandomValue(0, i);
        //     rhythm_sequences_shuffled[i] = rhythm_sequences[j];
        // }

        int index = GetRandomValue(0, oc_len(rhythm_sequences) - 1);
        current_rhythm_sequence = &rhythm_sequences[index];

        // int index = GetRandomValue(0, oc_len(DEFINED_POINTS) - 1);

        points = NULL;
        for(int32_t i = oc_len(DEFINED_POINTS) - 1; i >= 0; i--) {
            uint32_t j = GetRandomValue(0, i);

            if (DEFINED_POINTS[j].point_count == current_rhythm_sequence->count) {
                points = DEFINED_POINTS[j].points;
                points_count = DEFINED_POINTS[j].point_count;
                break;
            }
        }
        if (points) break;
        

        // points = NULL;
        // for (int i = 0; i < oc_len(DEFINED_POINTS); i++) {
        //     if (DEFINED_POINTS[index].point_count == current_rhythm_sequence->count) {
        //         points = DEFINED_POINTS[index].points;
        //         points_count = DEFINED_POINTS[index].point_count;
        //     }
        // }
        // if (points) break;

        // current_rhythm_sequence = NULL;
        // for (int i = 0; i < oc_len(rhythm_sequences_shuffled); i++) {
        //     if (rhythm_sequences_shuffled[i].count == points_count) {
        //         current_rhythm_sequence = &rhythm_sequences_shuffled[i];
        //     }
        // }
        // if (current_rhythm_sequence) break;
    }

    current_shot_point_index = 0;
    Vector2 canon_pivot = {
        320+laser_canon_body.width / 2.0f,
        600.0f - 95.0f,
    };
    Vector2 diff = Vector2Subtract(points[current_shot_point_index], canon_pivot);
    current_canon_angle = atan2f(diff.y, diff.x) * 180 / PI + 90.0f;

    rhythm_sequence_note_index = 0;
    rhythm_sequence_beat_index = 0;
    current_state = STATE_POPIN;
    timer_init(&rhythm_sequence_timer, 250);

    is_shooting = false;
}

static inline void do_wait_then_state(int next_state) {
    current_state = STATE_WAIT;
    wait_next_state = next_state;
    timer_init(&wait_timer, 500);
}

void rhythm_game_prerender(void) {
    const float POINT_RADIUS = 20.0f;

    if (do_create_points) {
        if (IsKeyPressed(KEY_S)) {
            print("{{\n");
            print("    {},\n", points_count);
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

    switch (current_state) {
    case STATE_WAIT: {
        if (timer_update(&wait_timer)) {
            current_state = wait_next_state;
        }
    } break;
    case STATE_POPIN: {
        if (timer_update(&rhythm_sequence_timer)) {
            if (current_rhythm_sequence->notes[rhythm_sequence_beat_index]) {
                PlaySound(stabbing);
                rhythm_sequence_note_index++;
            }
            rhythm_sequence_beat_index++;

            if (rhythm_sequence_beat_index >= 8) {
                rhythm_sequence_beat_index = 0;
                rhythm_sequence_note_index = 0;
                current_state = STATE_PLAYBACK;
                // do_wait_then_state(STATE_PLAYBACK);
            }
            timer_reset(&rhythm_sequence_timer);
        }
    } break;
    case STATE_PLAYBACK: {
        if (timer_update(&rhythm_sequence_timer)) {
            if (rhythm_sequence_beat_index >= 8) {
                set_random_points();
                // do_wait_then_state(STATE_POPIN);
            } else {
                if (current_rhythm_sequence->notes[rhythm_sequence_beat_index]) {
                    PlaySound(katana1);
                    rhythm_sequence_note_index++;
                }
                rhythm_sequence_beat_index++;
                timer_reset(&rhythm_sequence_timer);
            }
        }
        if (IsKeyPressed(KEY_SPACE)) {
            timer_init(&beam_timer, 100);
            is_shooting = true;
        }
    } break;
    default: break;
    }

    if (is_shooting) {
        if (timer_update(&beam_timer)) {
            current_shot_point_index++;
            if (current_shot_point_index >= points_count) {
                // current_shot_point_index = 0;
                // set_random_points();
            } else {
            }
            is_shooting = false;
        }
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

                if (is_shooting) {
                    float interp = timer_interpolate(&beam_timer);
                    DrawRectanglePro((Rectangle) {
                        canon_pivot.x + diff.x * interp, canon_pivot.y + diff.y * interp,
                        8.0f, 40.0f,
                    }, (Vector2) { 2.0f, 20.0f}, current_canon_angle, ORANGE);
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

                switch (current_state) {
                case STATE_POPIN: {
                    for (int i = 0; i < rhythm_sequence_note_index; i++) {
                        DrawCircleV(points[i], POINT_RADIUS, RED);
                        DrawRing(points[i], POINT_RADIUS - 3, POINT_RADIUS, 0, 360, 100, ((Color) {0, 0, 0, 255}));
                    }
                } break;
                case STATE_WAIT: if(wait_next_state != STATE_PLAYBACK) break;
                case STATE_PLAYBACK: {
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
                } break;
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
    
    katana1 = LoadSound("resources/sounds/katana1.mp3");
    knife = LoadSound("resources/sounds/knife.mp3");
    stabbing = LoadSound("resources/sounds/stabbing.mp3");

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

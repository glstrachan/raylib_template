#include "dialog.h"
#include "encounter.h"
/*
top_level_game_state:
    TUTORIAL
    SELECT_ITEMS
    SELECT_ENCOUNTER
    IN_ENCOUNTER
    DAY_SUMMARY
    PLAYTHROUGH_SUMMARY

encounter: (person, item)

*/




// static void smile_encounter(void) {
//     dialog_text("foo");
//     string greet = lookupgreeting();
//     dialog_text("You", greet);

//     // do_minigame(smile_game);

//     switch (dialog_selection("Choose a greeting",
//         "Hello", "Hi", "Howdy", "whats fuckin up?")) {
//     case 0: dialogt(); break;
//     }
// }



#include <raylib.h>
#include "game.h"

static Texture2D image_mad, image_sad, image_neutral, image_happy, image_max_happy;
static Shader shader;
static float sweetspot;
static float smileness;

static float mood_matching;
static float mood_expected;
static Game_Timer mood_sampler;

static float velocity;
static float acceleration;
static float acceleration1;

Encounter_Sequence dialog_sequence = { 0 };

static void place_bar(void) {
    CustomLayoutElement* customBackgroundData = oc_arena_alloc(&frame_arena, sizeof(CustomLayoutElement));
    customBackgroundData->type = CUSTOM_LAYOUT_ELEMENT_TYPE_BACKGROUND;
    customBackgroundData->customData.background = (CustomLayoutElement_Background) { .shader = shader };

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(),
                .height = CLAY_SIZING_FIXED(40),
            },
        },
        .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
        .cornerRadius = CLAY_CORNER_RADIUS(16),
        .custom = { .customData = customBackgroundData },
    }) {
        Texture2D* texture;
        if (smileness < 0.2f) {
            texture = &image_mad;
        } else if (smileness < 0.4f) {
            texture = &image_sad;
        } else if (smileness < 0.6f) {
            texture = &image_neutral;
        } else if (smileness < 0.8f) {
            texture = &image_happy;
        } else {
            texture = &image_max_happy;
        }

        CLAY_AUTO_ID({
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_GROW(),
                }
            },
            .floating = { .offset = {-54/2.0f, -60}, .attachTo = CLAY_ATTACH_TO_PARENT, .attachPoints = { CLAY_ATTACH_POINT_LEFT_CENTER, CLAY_ATTACH_POINT_LEFT_CENTER } },
        }) {
            CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_PERCENT(smileness), .height = CLAY_SIZING_FIXED(60) } } });
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(54),
                        .height = CLAY_SIZING_FIXED(53),
                    },
                },
                .image = { .imageData = texture },
            });
        }

        CLAY_AUTO_ID({
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_GROW(),
                }
            },
            .floating = { .offset = {-10, 0}, .attachTo = CLAY_ATTACH_TO_PARENT, .attachPoints = { CLAY_ATTACH_POINT_LEFT_CENTER, CLAY_ATTACH_POINT_LEFT_CENTER } },
        }) {
            CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_PERCENT(smileness), .height = CLAY_SIZING_FIXED(60) } } });
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(20),
                        .height = CLAY_SIZING_FIXED(60),
                    },
                },
                .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
                .cornerRadius = CLAY_CORNER_RADIUS(10),
                .backgroundColor = {20, 20, 20, 255},
            });
        }
    }
}

#define SMILE_DIALOG_PARAMS() .place_above_dialog = place_bar, .timeout = 5000.0f
#define old_lady_dialog(text, ...) dialog_text(CHARACTERS_OLDLADY, (text), SMILE_DIALOG_PARAMS(), __VA_ARGS__);

void smile_dialog_sequence(void) {
    encounter_begin(&dialog_sequence);
    old_lady_dialog("soo nice smile u have there", .mood = 1.0f);
    old_lady_dialog("my cat died", .mood = 0.0f);
    encounter_end();
}

void smile_game_init(void) {

    image_mad = LoadTexture("resources/mad.png");
    image_sad = LoadTexture("resources/sad.png");
    image_neutral = LoadTexture("resources/neutral.png");
    image_happy = LoadTexture("resources/happy.png");
    image_max_happy = LoadTexture("resources/max_happy.png");
    shader = LoadShader(NULL, "resources/smile_shader.fs");

    smileness = GetRandomValue(0, 1000) / 1000.0f;
    sweetspot = GetRandomValue(100, 900) / 1000.0f;
    velocity = 0.0f;
    acceleration = -0.8;
    acceleration1 = 0.0f;
    mood_matching = 0.0f;

    encounter_sequence_start(&dialog_sequence, smile_dialog_sequence);
    timer_init(&mood_sampler, 1000);
}

bool smile_game_update(void) {
    encounter_sequence_update(&dialog_sequence);
    if (dialog_sequence.is_done) {
        float well_done = Clamp(mood_matching / mood_expected * 0.8314, 0.0f, 1.0f);
        print("{} {}\n", mood_matching, mood_expected);
        return true;
    }

    if (timer_update(&mood_sampler)) {
        mood_matching += 1.0f - fabsf(dialog_sequence.current_mood - smileness);
        mood_expected += 1.0f;
        timer_reset(&mood_sampler);
    }

    const int width = 100;
    const int padding_offset = 100;

    float dt = GetFrameTime();

    float net_acceleration = acceleration1 + acceleration;
    velocity += net_acceleration * dt;
    smileness += velocity * dt;
    if (smileness > 1.0f) {
        smileness = 1.0f;
        velocity = 0.0f;
    } else if (smileness < 0.0f) {
        smileness = 0.0f;
        velocity = 0.0f;
    }

    if (IsKeyPressed(KEY_SPACE)) {
        acceleration1 = 200.0f;
    } else {
        acceleration1 = dt * acceleration1;
        // acceleration1 = 0.0f;
    }
    // smileness = min(smileness + velocity * dt, 1.0f);


    {
        float midline = game_parameters.screen_height / 2.0f;
        float startpoint_x = 400.00f;
        float height = 20.0f;
        float gap = 200.0f;
        float smileness_scale = -height * 0.5 * ((1.0f - smileness) * 2.0f - 1.0f) * 5.0f;

        static Vector2 points[8];
        points[0] = (Vector2) {startpoint_x, midline + smileness_scale * 3.0f};
        points[1] = (Vector2) {startpoint_x, midline + smileness_scale * 3.0f};
        points[2] = (Vector2) {startpoint_x + gap, midline + height + smileness_scale};
        points[3] = (Vector2) {startpoint_x + gap * 2.0f, midline + height};
        points[4] = (Vector2) {startpoint_x + gap * 3.0f, midline + height};
        points[5] = (Vector2) {startpoint_x + gap * 4.0f, midline + height + smileness_scale};
        points[6] = (Vector2) {startpoint_x + gap * 5.0f, midline  + smileness_scale * 3.0f};
        points[7] = (Vector2) {startpoint_x + gap * 5.0f, midline  + smileness_scale * 3.0f};

        static Vector2 points1[8];
        points1[0] = (Vector2) {startpoint_x + gap * 5.0f, midline  + smileness_scale * 3.0f};
        points1[1] = (Vector2) {startpoint_x + gap * 5.0f, midline  + smileness_scale * 3.0f};
        points1[2] = (Vector2) {startpoint_x + gap * 4.0f, midline - height + smileness_scale};
        points1[3] = (Vector2) {startpoint_x + gap * 3.0f, midline - height};
        points1[4] = (Vector2) {startpoint_x + gap * 2.0f, midline - height};
        points1[5] = (Vector2) {startpoint_x + gap, midline - height + smileness_scale};
        points1[6] = (Vector2) {startpoint_x, midline + smileness_scale * 3.0f};
        points1[7] = (Vector2) {startpoint_x, midline + smileness_scale * 3.0f};

        Color color = {200, 10, 50, 255};
        // DrawSplineLinear(points, oc_len(points), 1.0f, (Color){255, 255, 255, 255});
        DrawSplineCatmullRom(points, oc_len(points), 4.0f, color);
        DrawSplineCatmullRom(points1, oc_len(points1), 4.0f, color);
    }

    game_objective_widget(lit("Smile the right amount based on what the Old Lady is saying!"));

    return false;
}

Minigame smile_game = {
    .init = smile_game_init,
    .update = smile_game_update,
};
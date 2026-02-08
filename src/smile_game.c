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

static Texture2D image_mad, image_sad, image_neutral, image_happy, image_max_happy, bighead;
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

static bool is_in_old_lady_dialog = false;

static void place_bar(void) {
    is_in_old_lady_dialog = true;
    CustomLayoutElement* customBackgroundData = oc_arena_alloc(&frame_arena, sizeof(CustomLayoutElement));
    customBackgroundData->type = CUSTOM_LAYOUT_ELEMENT_TYPE_BACKGROUND;
    customBackgroundData->customData.background = (CustomLayoutElement_Background) { .shader = shader };

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
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
                    .width = CLAY_SIZING_GROW(0),
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
                    .width = CLAY_SIZING_GROW(0),
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

#define DEFAULT_DIFFICULTY (5000.0f)
#define SALESMAN_TIMEOUT (2000.0f)
static float difficulty_timeout = DEFAULT_DIFFICULTY;
static int target_dialog_count;
static int dialog_count;

#define SMILE_DIALOG_PARAMS() .place_above_dialog = place_bar, .timeout = difficulty_timeout, .use_bottom_text = true

#define old_lady_dialog(text, ...) do {                                          \
    if (dialog_count >= target_dialog_count) dialog_yield();                      \
    dialog_text(CHARACTERS_OLDLADY, (text), SMILE_DIALOG_PARAMS(), __VA_ARGS__); \
    dialog_count++;                                                              \
} while (0)

void smile_dialog_sequence(void) {
    encounter_begin(&dialog_sequence);
    
    // Very Good (0.9-1.0)
    old_lady_dialog("what a wonderful day this is! the sun is shining!", .mood = 0.95f);
    dialog_text(CHARACTERS_SALESMAN, "It really is beautiful outside!", .timeout = SALESMAN_TIMEOUT);
    
    // Good (0.7-0.8)
    old_lady_dialog("my garden is looking quite nice this year", .mood = 0.75f);
    dialog_text(CHARACTERS_SALESMAN, "I noticed those lovely flowers by your door!", .timeout = SALESMAN_TIMEOUT);
    
    // Neutral (0.4-0.6)
    old_lady_dialog("well, i suppose the groceries cost a bit more these days", .mood = 0.5f);
    dialog_text(CHARACTERS_SALESMAN, "Yes, prices have certainly gone up recently.", .timeout = SALESMAN_TIMEOUT);
    
    // Bad (0.2-0.3)
    old_lady_dialog("my knees have been bothering me something awful lately", .mood = 0.25f);
    dialog_text(CHARACTERS_SALESMAN, "Oh dear, that sounds quite painful.", .timeout = SALESMAN_TIMEOUT);
    
    // Very Bad (0.0-0.1)
    old_lady_dialog("my cat harold passed away last week. he was my best friend.", .mood = 0.05f);
    dialog_text(CHARACTERS_SALESMAN, "I'm so sorry to hear that. That must be really difficult.", .timeout = SALESMAN_TIMEOUT);
    
    // Good again (0.7-0.8)
    old_lady_dialog("but you know, at least i have these nice memories with him", .mood = 0.72f);
    dialog_text(CHARACTERS_SALESMAN, "Those memories are truly precious.", .timeout = SALESMAN_TIMEOUT);
    
    // Very Good (0.85-0.95)
    old_lady_dialog("and now i'm looking forward to enjoying my time with visitors like you!", .mood = 0.88f);
    dialog_text(CHARACTERS_SALESMAN, "Well, it's been a pleasure talking with you!", .timeout = SALESMAN_TIMEOUT);
    
    // Neutral-Bad (0.35-0.45)
    old_lady_dialog("though i do worry sometimes about managing everything on my own", .mood = 0.4f);
    dialog_text(CHARACTERS_SALESMAN, "That's completely understandable.", .timeout = SALESMAN_TIMEOUT);
    
    // Good (0.65-0.75)
    old_lady_dialog("but i'm a strong woman, and i've made it this far just fine", .mood = 0.7f);
    dialog_text(CHARACTERS_SALESMAN, "You certainly seem like someone with a lot of character!", .timeout = SALESMAN_TIMEOUT);
    
    // Very Good (0.9-1.0)
    old_lady_dialog("and that smile of yours reminds me why life is worth living!", .mood = 0.92f);
    dialog_text(CHARACTERS_SALESMAN, "That's the nicest thing anyone's said to me all week!", .timeout = SALESMAN_TIMEOUT);
    
    // Neutral (0.45-0.55)
    old_lady_dialog("you know, my grandson visited last month. he's doing well in school.", .mood = 0.52f);
    dialog_text(CHARACTERS_SALESMAN, "That's wonderful! You must be very proud of him.", .timeout = SALESMAN_TIMEOUT);
    
    // Bad (0.15-0.25)
    old_lady_dialog("though he doesn't call me as often as i'd like", .mood = 0.2f);
    dialog_text(CHARACTERS_SALESMAN, "I'm sure he thinks of you often, even when he's busy.", .timeout = SALESMAN_TIMEOUT);
    
    // Good (0.75-0.85)
    old_lady_dialog("my bridge club meets every thursday and we always have a good laugh", .mood = 0.8f);
    dialog_text(CHARACTERS_SALESMAN, "That sounds like so much fun! I love hearing about that.", .timeout = SALESMAN_TIMEOUT);
    
    // Bad (0.2-0.35)
    old_lady_dialog("though sometimes my old bones make it hard to get around", .mood = 0.28f);
    dialog_text(CHARACTERS_SALESMAN, "I can certainly understand that struggle.", .timeout = SALESMAN_TIMEOUT);
    
    // Good (0.7-0.8)
    old_lady_dialog("but i refuse to let a little thing like that slow me down!", .mood = 0.78f);
    dialog_text(CHARACTERS_SALESMAN, "That's the spirit! Your determination is inspiring.", .timeout = SALESMAN_TIMEOUT);
    
    // Neutral-Good (0.55-0.65)
    old_lady_dialog("my late husband used to say 'life is what you make of it'", .mood = 0.6f);
    dialog_text(CHARACTERS_SALESMAN, "He sounds like he was a very wise man.", .timeout = SALESMAN_TIMEOUT);
    
    // Very Good (0.88-0.98)
    old_lady_dialog("and i've decided to make mine count for as much as i can!", .mood = 0.93f);
    dialog_text(CHARACTERS_SALESMAN, "Now that's a wonderful way to live your life!", .timeout = SALESMAN_TIMEOUT);
    
    // Bad (0.18-0.28)
    old_lady_dialog("sometimes the house feels too quiet without anyone around", .mood = 0.23f);
    dialog_text(CHARACTERS_SALESMAN, "That must be a lonely feeling. Thank you for letting me visit.", .timeout = SALESMAN_TIMEOUT);
    
    // Very Good (0.85-0.95)
    old_lady_dialog("but days like this remind me that there's still so much joy to be found!", .mood = 0.9f);
    dialog_text(CHARACTERS_SALESMAN, "You have such a beautiful outlook on life! It's truly wonderful.", .timeout = SALESMAN_TIMEOUT);
    
    // Good (0.72-0.82)
    old_lady_dialog("well, you've certainly brightened my day, young man. thank you for that", .mood = 0.77f);
    dialog_text(CHARACTERS_SALESMAN, "The pleasure has been all mine, ma'am. Thank you for your kindness.", .timeout = SALESMAN_TIMEOUT);
    
    encounter_end();
}

void smile_game_init(void) {

    image_mad = LoadTexture("resources/mad.png");
    image_sad = LoadTexture("resources/sad.png");
    image_neutral = LoadTexture("resources/neutral.png");
    image_happy = LoadTexture("resources/happy.png");
    image_max_happy = LoadTexture("resources/max_happy.png");
    bighead = LoadTexture("resources/bighead.png");
    shader = LoadShader(NULL, "resources/smile_shader.fs");

    smileness = GetRandomValue(0, 1000) / 1000.0f;
    sweetspot = GetRandomValue(100, 900) / 1000.0f;
    velocity = 0.0f;
    acceleration = -0.8;
    acceleration1 = 0.0f;
    mood_matching = 0.0f;
    
    difficulty_timeout = DEFAULT_DIFFICULTY - 1000.0f * game.current_day;
    target_dialog_count = 5 + 5 * game.current_day;
    dialog_count = 0;

    encounter_sequence_start(&dialog_sequence, smile_dialog_sequence);
    timer_init(&mood_sampler, 1000);
}

bool smile_game_update(void) {
    is_in_old_lady_dialog = false;
    encounter_sequence_update(&dialog_sequence);
    if (dialog_sequence.is_done || dialog_count >= target_dialog_count) {
        float well_done = Clamp(mood_matching / mood_expected * 0.8314, 0.0f, 1.0f);
        return true;
    }

    #ifdef _DEBUG
        if (IsKeyPressed(KEY_F)) {
            return true;
        }
    #endif

    if (is_in_old_lady_dialog) {
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
    } else {
        velocity = 0.0f;
    }
    // smileness = min(smileness + velocity * dt, 1.0f);


    DrawTexture(bighead, 408, 0, WHITE);
    // smileness = 0.4f;
    {
        float midline = 400.0f;
        float height = 4.0f;
        float gap = 18.0f;
        float startpoint_x = 400 + bighead.width/2.0f - gap*5.0f/2.0f;
        float smileness_scale = -height * 0.5 * (smileness * 2.0f - 1.0f) * 3.0f;

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

        Color color = {0, 0, 0, 255};
        // DrawSplineLinear(points, oc_len(points), 1.0f, (Color){255, 255, 255, 255});
        DrawSplineCatmullRom(points, oc_len(points), 5.0f, color);
        DrawSplineCatmullRom(points1, oc_len(points1), 5.0f, color);
    }

    game_objective_widget(lit("Smile the right amount based on what the Old Lady is saying!"));

    return false;
}

Minigame smile_game = {
    .init = smile_game_init,
    .update = smile_game_update,
};
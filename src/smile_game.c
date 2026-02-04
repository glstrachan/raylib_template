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

static Shader shader;
static float sweetspot;
static float smileness;

static float velocity;
static float acceleration;
static float acceleration1;
Encounter_Sequence dialog_sequence = { 0 };

void smile_dialog_sequence(void) {
    encounter_begin(&dialog_sequence);
    dialog_text("Old lady", "soo nice smile u have there");
    dialog_text("Old lady", "my cat died");
    encounter_end();
}

void smile_game_init(void) {
    shader = LoadShader(NULL, "resources/smile_shader.fs");
    smileness = GetRandomValue(0, 1000) / 1000.0f;
    sweetspot = GetRandomValue(100, 900) / 1000.0f;
    velocity = 0.0f;
    acceleration = 0.08;
    acceleration1 = 0.0f;

    encounter_sequence_start(&dialog_sequence, smile_dialog_sequence);
}

bool smile_game_update(void) {

    encounter_sequence_update(&dialog_sequence);

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
        acceleration1 = -20.0f;
    } else {
        acceleration1 = dt * acceleration1;
        // acceleration1 = 0.0f;
    }
    // smileness = min(smileness + velocity * dt, 1.0f);

    Rectangle rect = {
        .x = game_parameters.screen_width - padding_offset - width,
        .y = padding_offset,
        .width = width,
        .height = game_parameters.screen_height - padding_offset * 2,
    };

    // DrawRectangle(game_parameters.screen_width - padding_offset - width, padding_offset, width, game_parameters.screen_height - padding_offset * 2, (Color){ 0, 0, 0, 255 });
    // DrawRectangleRec(rect, (Color) { 0, 0, 0, 255 });

    Color red = {255, 0, 0, 255};
    Color green = {0, 255, 0, 255};
    BeginShaderMode(shader);
        static Vector2 resolution;
        resolution = (Vector2) {rect.width, rect.height};
        SetShaderValue(shader, GetShaderLocation(shader, "resolution"), &resolution, SHADER_UNIFORM_VEC2);
        SetShaderValue(shader, GetShaderLocation(shader, "smile_0to1"), &sweetspot, SHADER_UNIFORM_FLOAT);

        SetShapesTexture((Texture2D){1, rect.width, rect.height, 1, 7}, (Rectangle){0, 0, rect.width, rect.height});
        DrawRectangleRec(rect, WHITE);
    EndShaderMode();

    const int height = 10;
    const int margin = 4;
    Rectangle lines_rect = {
        rect.x - margin, rect.y + smileness * rect.height - height / 2, width + margin * 2, height,
    };
    DrawRectangleLinesEx(lines_rect, 2.0f, (Color) { 255, 255, 255, 255 });



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

    return false;
}

Minigame smile_game = {
    .init = smile_game_init,
    .update = smile_game_update,
};
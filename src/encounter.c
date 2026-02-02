#include "../external/clay/clay.h"
#include "../external/clay/clay_renderer_raylib.h"

#include "encounter.h"
#include "jump.h"


Encounter_Sequence encounter_top_sequence = { 0 };
static Encounter* current_encounter;

void encounter_start(Encounter* encounter) {
    current_encounter = encounter;
    encounter_sequence_start(&encounter_top_sequence, encounter);
}

void encounter_update(void) {
    if (!current_encounter) return;
    encounter_sequence_update(&encounter_top_sequence);
}

void encounter_sequence_start(Encounter_Sequence* sequence, Encounter* encounter) {
    if (!sequence->stack) {
        sequence->stack = malloc(10 * 1024);
    }
    sequence->encounter = encounter;
    sequence->first_time = true;

    uintptr_t top_of_stack = oc_align_forward(((uintptr_t)sequence->stack) + 10 * 1024 - 16, 16);

    static Encounter_Sequence* this_sequence;
    this_sequence = sequence; // needed since we modify rsp
    asm volatile("mov %%rsp, %0" : "=r" (this_sequence->old_stack));
    asm volatile("mov %0, %%rsp" :: "r" (top_of_stack));
    if (my_setjmp(this_sequence->jump_back_buf) == 0) {
        this_sequence->encounter->fn();
    }
    asm volatile("mov %0, %%rsp" :: "r" (this_sequence->old_stack));
}

void encounter_sequence_update(Encounter_Sequence* sequence) {
    asm volatile("mov %%rsp, %0" : "=r" (sequence->old_stack));
    if (my_setjmp(sequence->jump_back_buf) == 0) {
        my_longjmp(sequence->jump_buf, 1);
    }
    asm volatile("mov %0, %%rsp" :: "r" (sequence->old_stack));
}

bool encounter_is_done(void) {
    return encounter_top_sequence.is_done;
}

void sample_encounter(void) {
    encounter_begin();

    dialog_text("Old Lady", "Y'know back in my day you was either white or you was dead. You darn whippersnappers!!");
    dialog_text("potato", "Good, you?");

    switch (dialog_selection("Choose a Fruit", "Potato", "Cherry", "Tomato", "Apple")) {
        case 0: {
            dialog_text("Old Lady", "Wowwwww! you chose potato!");
        } break;
        case 1: {
            dialog_text("Old Lady", "Cherry's okay");
        } break;
        case 2: {
            dialog_text("Old Lady", "Oh... you chose tomato");
        } break;
        case 3: {
            dialog_text("Old Lady", "Apple? really?");
        } break;
    }

    extern Minigame memory_game, smile_game;
    encounter_minigame(&memory_game);

    dialog_text("Old Lady", "Wow impressive!");
    dialog_text("Old Lady", "Now try this");

    encounter_minigame(&smile_game);

    encounter_end();
}

Encounter sample_encounter_ = {
    .fn = sample_encounter,
    .name = lit("Old Lady"),
};

static Shader background_shader;
void pick_encounter_init(void) {
    background_shader = LoadShader(0, "resources/dialogbackground.fs");
}

void pick_encounter(void) {
    game.encounter = &sample_encounter_;
}

void pick_encounter_update(void) {
    Oc_String_Builder sb;
    oc_sb_init(&sb, &frame_arena);
    wprint(&sb.writer, "Selling to {}", game.encounter->name);
    string txt = oc_sb_to_string(&sb);

    CustomLayoutElement* customBackgroundData = oc_arena_alloc(&frame_arena, sizeof(CustomLayoutElement));
    customBackgroundData->type = CUSTOM_LAYOUT_ELEMENT_TYPE_BACKGROUND;
    customBackgroundData->customData.background = (CustomLayoutElement_Background) { background_shader };


    CLAY(CLAY_ID("DialogBox"), {
        .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER } },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_PERCENT(0.8),
                .height = CLAY_SIZING_PERCENT(0.6)
            },
            .padding = {16, 16, 16, 16},
            .childGap = 16
        },
        .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
        .custom = { .customData = customBackgroundData },
        .cornerRadius = CLAY_CORNER_RADIUS(16)
    }) {
        CLAY_TEXT(((Clay_String) { .length = txt.len, .chars = txt.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = 2, .textColor = {255, 255, 255, 255} }));
        CLAY_AUTO_ID({
            .layout = { .sizing = { .height = CLAY_SIZING_GROW() } }
        });
        CLAY(CLAY_ID("DialogContinue"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                },
                .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = {200, 0, 0, 255},
        }) {
            CLAY_TEXT((CLAY_STRING("Enter to Select")), CLAY_TEXT_CONFIG({ .fontSize = 25, .fontId = 3, .textColor = {135, 135, 135, 255} }));
        }
    }

    // DrawTextEx(game_parameters.dialog_font_big, txt.ptr, (Vector2) { game_parameters.screen_width / 2.0f, 100.0f }, game_parameters.dialog_font_big.baseSize, 10, ())
}


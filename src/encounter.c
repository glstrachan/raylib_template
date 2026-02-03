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
        CLAY_TEXT(STR_TO_CLAY_STRING(txt), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
        CLAY_AUTO_ID({
            .layout = { .sizing = { .height = CLAY_SIZING_GROW() } }
        });
        CLAY(CLAY_ID("DaySummaryBottom"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                    .height = CLAY_SIZING_FIXED(100)
                },
                .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_BOTTOM }
            },
            .backgroundColor = {200, 0, 0, 0},
        }) {
            CLAY(CLAY_ID("DaySummaryNextDay"), {
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
                CLAY_TEXT((CLAY_STRING("Start Selling")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
                if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    extern Encounter sample_encounter_;
                    game.encounter = &sample_encounter_;
                    game_go_to_state(GAME_STATE_IN_ENCOUNTER);
                }
            }
        }
    }
}


#include "raylib.h"
#include "rlgl.h"
#include "game.h"

#include "dialog.h"

#include "../external/clay/clay.h"

Enum(Dialog_Decoration_Type, uint32_t,
    DIALOG_TYPE_DEFAULT = 0,
    DIALOG_TYPE_ANGRY,
    DIALOG_TYPE_SURPRISED    
);

static uint32_t total_chars;
static uint32_t printed_chars;
static Game_Timer char_timer;
static Dialog_Item current_dialog = NULL;

typedef struct {
    string name;
} Dialog_Selection;

static int dialog_branches[1024];
static int dialog_max_item;
static int dialog_current_item;
static int dialog_current_branch;
static bool first_time;

int _dialog_selection(int count, Dialog_Selection items[]) {
    return 0;
}

#define dialog_selection(...) ({                                   \
    int selection;                                                 \
    if (dialog_current_item < dialog_max_item) {                   \
        dialog_current_item++;                                     \
        selection = dialog_branches[dialog_current_branch++];      \
    } else if (dialog_current_item == dialog_max_item) {           \
        Dialog_Selection items[] = { __VA_ARGS__ };                \
        selection = _dialog_selection(oc_len(items), items);       \
        if (selection < 0) {                                       \
            first_time = false;                                    \
            return;                                                \
        } else {                                                   \
            dialog_branches[dialog_current_branch++] = selection;  \
            dialog_current_item++;                                 \
            dialog_max_item++;                                     \
            first_time = true;                                     \
        }                                                          \
    } else {                                                       \
        oc_assert(false);                                          \
    }                                                              \
    selection;                                                     \
})

bool _dialog_text(string speaker_name, string text, Dialog_Decoration_Type decoration) {

    // draw the text
    if (first_time) {
        total_chars = text.len;
        printed_chars = 0;
        timer_init(&char_timer, 100);
    }

    bool result = false;

    if(printed_chars < total_chars) {
        if (timer_update(&char_timer)) {
            timer_reset(&char_timer);
            printed_chars++;
        }
        
        if(IsKeyPressed(KEY_SPACE)) {
            printed_chars = total_chars;
        }
    } else if(total_chars == printed_chars) {
        // Draw a continue prompt

        if (IsKeyPressed(KEY_SPACE)) {
            result = true;
        }
    }

    // Oc_String_Builder builder;
    // oc_sb_init(&builder, &frame_arena);

    // wprint(&builder.writer, "{}", string_slice(text, 0, printed_chars));
    // string display_str = oc_sb_to_string(&builder);
    string display_str = string_slice(text, 0, printed_chars);

    // Vector2 text_position = {game_parameters.screen_width / 2 - game_parameters.screen_width * 0.3, game_parameters.screen_height - 200};
    
    // .left = text_position.x, .right = text_position.x + game_parameters.screen_width * 0.6, .top = text_position.y, .bottom = game_parameters.screen_height 
    // CLAY_AUTO_ID({ 
    //     .floating = { .offset = {0, -100}, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_BOTTOM, CLAY_ATTACH_POINT_CENTER_BOTTOM } },
    //     .layout = { .sizing = {CLAY_SIZING_FIXED(game_parameters.screen_width * 0.6), CLAY_SIZING_FIXED(100)} },
    //     .backgroundColor = { 120, 120, 120, 255 },
    // }) {
    //     CLAY_TEXT(((Clay_String) { .length = display_str.len, .chars = display_str.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = 1, .textColor = {255, 255, 255, 255} }));
    // }

    CLAY(CLAY_ID("DialogBox"), {
        .floating = { .offset = {0, -100}, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_BOTTOM, CLAY_ATTACH_POINT_CENTER_BOTTOM } },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_PERCENT(0.5),
                .height = CLAY_SIZING_PERCENT(0.18)
            },
            .padding = {16, 16, 16, 16},
            .childGap = 16
        },
        .backgroundColor = {200, 200, 200, 255},
        .cornerRadius = CLAY_CORNER_RADIUS(16)
    }) {
        CLAY(CLAY_ID("DialogName"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                    .height = CLAY_SIZING_PERCENT(0.25)
                }
            },
            .backgroundColor = {200, 0, 0, 0},
        }) {
            CLAY_TEXT(((Clay_String) { .length = speaker_name.len, .chars = speaker_name.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = 1, .textColor = {255, 255, 255, 255} }));
        }
        CLAY(CLAY_ID("DialogTextContainer"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                    .height = CLAY_SIZING_GROW(),
                },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER }
            },
            .backgroundColor = {0, 255, 0, 0},
        }) {
            CLAY(CLAY_ID("DialogText"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_PERCENT(0.95),
                        .height = CLAY_SIZING_GROW(),
                    }
                },
                .backgroundColor = {0, 255, 255, 0},
            }) {
                CLAY_TEXT(((Clay_String) { .length = display_str.len, .chars = display_str.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = 1, .textColor = {255, 255, 255, 255} }));
            }
        }
        CLAY(CLAY_ID("DialogContinue"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                    .height = CLAY_SIZING_PERCENT(0.18)
                }
            },
            .backgroundColor = {200, 0, 0, 0},
        }) {}
    }

    return result;
}

#define dialog_text(speaker_name, text, decoration) do {          \
    if (dialog_current_item < dialog_max_item) {                  \
        dialog_current_item++;                                    \
    } else if (dialog_current_item == dialog_max_item) {           \
        if (_dialog_text(_Generic((speaker_name), string : (speaker_name), default: lit(speaker_name)), _Generic((text), string : (text), default: lit(text)), (decoration))) { \
            dialog_current_item++;                                \
            dialog_max_item++;                                    \
            first_time = true; \
            return; \
        } else {                                                  \
            first_time = false; \
            return;                                               \
        }                                                         \
    } else oc_assert(false);                                      \
} while (0)

void sample_dialog(void) {
    dialog_text("john", "hello, how are you? hello, how are you? hello, how are y", 0);
    dialog_text("potato", "Good, you?", 0);

    switch (dialog_selection(
        { "Potato" },
        { "Apple" },
        { "Cellary" },
    )) {
        // case 0: handle_potato();
        // case 1: handle_apple();
        // case 2: handle_cellary();
    }
}

void dialog_play(Dialog_Item dialog) {
    first_time = true;
    current_dialog = dialog;
    dialog_max_item = 0;
}

void dialog_update(void) {
    if(!current_dialog) return;

    dialog_current_item = 0;
    dialog_current_branch = 0;

    current_dialog();
}

#include "raylib.h"
#include "rlgl.h"
#include "game.h"

#include "encounter.h"
#include "dialog.h"

#include "../external/clay/clay.h"
#include "../external/clay/clay_renderer_raylib.h"

#include "dialog.h"

typedef struct {
    string name;
} Dialog_Selection;

static union {
    Dialog_Text_Data text;
    Dialog_Selection_Data selection;
} data;

int _dialog_selection(Encounter_Sequence* sequence, string prompt, int count, const char* items[]) {
    int result = -1;

    if (sequence->first_time) {
        data.selection.index = 0;
        sequence->first_time = false;
    }

    if (IsKeyPressed(KEY_RIGHT)) {
        data.selection.index = min(data.selection.index + 1, count - 1);
    } else if (IsKeyPressed(KEY_LEFT)) {
        data.selection.index = max(data.selection.index - 1, 0);
    }

    if (IsKeyPressed(KEY_ENTER)) {
        result = data.selection.index;
    }

    CustomLayoutElement* customBackgroundData = make_cool_background();

    DialogTextUserData* textUserData = oc_arena_alloc(&frame_arena, sizeof(DialogTextUserData));
    textUserData->visible_chars = data.text.printed_chars;

    CLAY(CLAY_ID("DialogBox"), {
        .floating = { .offset = {0, -100}, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_BOTTOM, CLAY_ATTACH_POINT_CENTER_BOTTOM } },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_PERCENT(0.5),
                .height = CLAY_SIZING_PERCENT(0.16)
            },
            .padding = {16, 16, 20, 10},
            .childGap = 16
        },
        .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
        .custom = { .customData = customBackgroundData },
        .cornerRadius = CLAY_CORNER_RADIUS(16)
    }) {
        CLAY(CLAY_ID("DialogName"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                    .height = CLAY_SIZING_PERCENT(0.25)
                },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = {200, 0, 0, 0},
        }) {
            CLAY_TEXT(((Clay_String) { .length = prompt.len, .chars = prompt.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
        }
        CLAY(CLAY_ID("DialogTextContainer"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                    .height = CLAY_SIZING_GROW(),
                },
                .childGap = 20,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = {0, 255, 0, 0},
        }) {
            for (int i = 0; i < count; ++i) {
                Clay_Color color;
                if (i == data.selection.index) {
                    color = (Clay_Color) { 180, 180, 180, 80 };
                } else {
                    color = (Clay_Color) { 0, 0, 0, 0 };
                }
                CLAY(CLAY_IDI("SelectionItem", i), {
                    .layout = { .padding = {16, 16, 2, 2} },
                    .backgroundColor = Clay_Hovered() ? (Clay_Color) { 180, 180, 180, 40 } : color,
                    .cornerRadius = CLAY_CORNER_RADIUS(4)
                }) {
                    CLAY_TEXT(((Clay_String) { .length = strlen(items[i]), .chars = items[i] }), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
                }

                bool buttonIsHovered = Clay_PointerOver(CLAY_IDI("SelectionItem", i));
                if (buttonIsHovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    result = i;
                }
            }
        }
        CLAY(CLAY_ID("DialogContinue"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                    .height = CLAY_SIZING_PERCENT(0.18)
                },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = {200, 0, 0, 0},
        }) {
            CLAY_TEXT((CLAY_STRING("Enter to Select")), CLAY_TEXT_CONFIG({ .fontSize = 25, .fontId = FONT_ITIM, .textColor = {135, 135, 135, 255} }));
        }
    }

    return result;
}

bool _dialog_text(Encounter_Sequence* sequence, string speaker_name, string text) {

    // draw the text
    if (sequence->first_time) {
        data.text.total_chars = text.len;
        data.text.printed_chars = 0;
        timer_init(&data.text.char_timer, 30);
        sequence->first_time = false;
    }

    bool result = false;

    if(data.text.printed_chars < data.text.total_chars) {
        if (timer_update(&data.text.char_timer)) {
            timer_reset(&data.text.char_timer);
            data.text.printed_chars++;
        }
        
        if(IsKeyPressed(KEY_SPACE)) {
            data.text.printed_chars = data.text.total_chars;
        }
    } else if(data.text.total_chars == data.text.printed_chars) {
        if (IsKeyPressed(KEY_SPACE)) {
            result = true;
        }
    }

    CustomLayoutElement* customBackgroundData = make_cool_background();

    DialogTextUserData* textUserData = oc_arena_alloc(&frame_arena, sizeof(DialogTextUserData));
    textUserData->visible_chars = data.text.printed_chars;

    CLAY(CLAY_ID("DialogBox"), {
        .floating = { .offset = {0, -100}, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_BOTTOM, CLAY_ATTACH_POINT_CENTER_BOTTOM } },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_PERCENT(0.5),
                .height = CLAY_SIZING_PERCENT(0.18)
            },
            .padding = {16, 16, 20, 10},
            .childGap = 16
        },
        .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
        .custom = { .customData = customBackgroundData },
        .cornerRadius = CLAY_CORNER_RADIUS(16)
    }) {
        CLAY(CLAY_ID("DialogName"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                    .height = CLAY_SIZING_PERCENT(0.25)
                },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = {200, 0, 0, 0},
        }) {
            CLAY_TEXT(((Clay_String) { .length = speaker_name.len, .chars = speaker_name.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
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
                CLAY_TEXT(((Clay_String) { .length = text.len, .chars = text.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255}, .userData = textUserData }));
            }
        }
        CLAY(CLAY_ID("DialogContinue"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                    .height = CLAY_SIZING_PERCENT(0.18)
                },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = {200, 0, 0, 0},
        }) {
            if(data.text.total_chars == data.text.printed_chars) {
                CLAY_TEXT((CLAY_STRING("Space to Continue")), CLAY_TEXT_CONFIG({ .fontSize = 25, .fontId = FONT_ITIM, .textColor = {135, 135, 135, 255} }));
            }
        }
    }

    return result;
}

void dialog_init() {
}

void dialog_cleanup() {
}
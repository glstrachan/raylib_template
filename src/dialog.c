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

int _dialog_selection(volatile Encounter_Sequence* volatile sequence, string prompt, int count, const char* items[], Dialog_Parameter parameters) {
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
                .height = CLAY_SIZING_FIT(.min = 200, .max = 800)
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
                    .height = CLAY_SIZING_FIT(0)
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
                    .height = CLAY_SIZING_FIT(0),
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
                    .layout = { .padding = {16, 16, 2, 2}, .sizing = { .height = CLAY_SIZING_GROW(0) }, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
                    .backgroundColor = Clay_Hovered() ? i == data.selection.index ? (Clay_Color) { 230, 230, 230, 80 } : (Clay_Color) { 180, 180, 180, 40 } : color,
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                    .border = { .width = { 2, 2, 2, 2, 0 }, .color = {135, 135, 135, 100} },
                }) {
                    CLAY_TEXT(((Clay_String) { .length = strlen(items[i]), .chars = items[i] }), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255}, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
                }

                bool buttonIsHovered = Clay_PointerOver(CLAY_IDI("SelectionItem", i));
                if (buttonIsHovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    result = i;
                }
            }
        }
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                    .height = CLAY_SIZING_FIXED(40)
                },
            },
            .backgroundColor = {200, 0, 0, 0},
        });

        CLAY(CLAY_ID("DialogContinue"), {
            .floating = { .offset = {0, 0}, .attachTo = CLAY_ATTACH_TO_PARENT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_BOTTOM, CLAY_ATTACH_POINT_CENTER_BOTTOM } },
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                .height = CLAY_SIZING_FIXED(40)
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

static Shader timer_shader;

bool _dialog_text(volatile Encounter_Sequence* volatile sequence, string speaker_name, string text, Dialog_Parameter parameters) {

    // draw the text
    if (sequence->first_time) {
        data.text.total_chars = text.len;
        data.text.printed_chars = 0;
        timer_init(&data.text.char_timer, 30);
        if (parameters.timeout > 0.0f) timer_init(&data.text.timeout_timer, parameters.timeout);
        sequence->first_time = false;
    }

    sequence->current_mood = parameters.mood;

    bool result = false;

    if(data.text.printed_chars < data.text.total_chars) {
        if (timer_update(&data.text.char_timer)) {
            timer_reset(&data.text.char_timer);
            data.text.printed_chars++;
        }
    }

    if (parameters.timeout > 0.0f) {
        if (timer_update(&data.text.timeout_timer)) {
            result = true;
        }
    } else if(!parameters.no_skip) {
        if(data.text.printed_chars < data.text.total_chars) {
            if(IsKeyPressed(KEY_SPACE)) {
                data.text.printed_chars = data.text.total_chars;
            }
        } else if(data.text.total_chars == data.text.printed_chars) {
            if (IsKeyPressed(KEY_SPACE)) {
                result = true;
            }
        }
    }

    CustomLayoutElement* customBackgroundData = oc_arena_alloc(&frame_arena, sizeof(CustomLayoutElement));
    customBackgroundData->type = CUSTOM_LAYOUT_ELEMENT_TYPE_BACKGROUND;
    customBackgroundData->customData.background = (CustomLayoutElement_Background) { .shader = timer_shader };


    DialogTextUserData* textUserData = oc_arena_alloc(&frame_arena, sizeof(DialogTextUserData));
    textUserData->visible_chars = data.text.printed_chars;

    CLAY_AUTO_ID({
        .floating = { .offset = {0, -100}, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_BOTTOM, CLAY_ATTACH_POINT_CENTER_BOTTOM } },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_PERCENT(0.5),
            },
            .childGap = 16
        },
    }) {
        if (parameters.place_above_dialog) {
            parameters.place_above_dialog();
        }


        CLAY_AUTO_ID({
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    // .height = CLAY_SIZING_FIXED(250)
                    .height = CLAY_SIZING_FIT(.min = 200, .max = 800)
                },
                .padding = {16, 20, 20, 10},
            },
            .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
            .custom = { .customData = make_cool_background() },
            .cornerRadius = CLAY_CORNER_RADIUS(16)
        }) {
            CLAY(CLAY_ID("DialogBox"), {
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .sizing = {
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_FIT(0),
                    },
                },
            }) {
                CLAY(CLAY_ID("DialogName"), {
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_PERCENT(1.0),
                            .height = CLAY_SIZING_FIT(0),
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
                            .height = CLAY_SIZING_FIT(0),
                        },
                        // .childAlignment = { .x = CLAY_ALIGN_X_CENTER }
                        .padding = { 16, 16, 8, 8 },
                    },
                    .backgroundColor = {0, 255, 0, 0},
                }) {
                    CLAY_TEXT(((Clay_String) { .length = text.len, .chars = text.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255}, .userData = textUserData }));
                }

                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_PERCENT(1.0),
                            .height = CLAY_SIZING_FIXED(20)
                        },
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
                    },
                    .backgroundColor = {200, 0, 0, 0},
                });
            }

            CLAY_AUTO_ID({
                .floating = { .offset = {0, 0}, .attachTo = CLAY_ATTACH_TO_PARENT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_BOTTOM, CLAY_ATTACH_POINT_CENTER_BOTTOM } },
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_PERCENT(1.0),
                        .height = CLAY_SIZING_FIXED(40)
                    },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
                },
                .backgroundColor = {200, 0, 0, 0},
            }) {
                if(data.text.total_chars == data.text.printed_chars && parameters.timeout == 0.0f) {
                    CLAY_TEXT((CLAY_STRING("Space to Continue")), CLAY_TEXT_CONFIG({ .fontSize = 25, .fontId = FONT_ITIM, .textColor = {135, 135, 135, 255} }));
                }
            }

            if(parameters.use_bottom_text) {
                CLAY_AUTO_ID({
                    .floating = { .offset = {0, 0}, .attachTo = CLAY_ATTACH_TO_PARENT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_BOTTOM, CLAY_ATTACH_POINT_CENTER_BOTTOM } },
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_PERCENT(1.0),
                            .height = CLAY_SIZING_FIXED(40)
                        },
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
                    },
                    .backgroundColor = {200, 0, 0, 0},
                }) {
                    CLAY_TEXT((CLAY_STRING("Press Space to Smile")), CLAY_TEXT_CONFIG({ .fontSize = 25, .fontId = FONT_ITIM, .textColor = {135, 135, 135, 255} }));
                }
            }


            if (parameters.timeout > 0.0f) {
                CLAY_AUTO_ID({
                    .layout = { .sizing = { .width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) } }
                }) {
                    float f = timer_interpolate(&data.text.timeout_timer);
                    SetShaderValue(timer_shader, GetShaderLocation(timer_shader, "completion"), &f, SHADER_UNIFORM_FLOAT);
                    CLAY_AUTO_ID({
                        .layout = { .sizing = { .width = CLAY_SIZING_FIXED(50), .height = CLAY_SIZING_FIXED(50) } },
                        .border = { .width = { 2, 2, 2, 2, 0 }, .color = {135, 135, 135, 255} },
                        .cornerRadius = CLAY_CORNER_RADIUS(1000),
                        .custom = { .customData = customBackgroundData },
                        // .backgroundColor = { 0, 0, 255, 255 },
                    });

                    CLAY_AUTO_ID({
                        .floating = { .offset = {-10, 0}, .attachTo = CLAY_ATTACH_TO_PARENT, .attachPoints = { CLAY_ATTACH_POINT_LEFT_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER } },
                    }) {
                        CLAY_TEXT(oc_format(&frame_arena, "{1}", timer_time_left(&data.text.timeout_timer) / 1000.0f), CLAY_TEXT_CONFIG({ .fontId = FONT_ITIM, .fontSize = 20, .textColor = {255, 255, 255, 255} }));
                    }
                }
            }
        }
    }

    return result;
}

void dialog_init() {
    timer_shader = LoadShader(NULL, "resources/timer_shader.fs");
}

void dialog_cleanup() {
}
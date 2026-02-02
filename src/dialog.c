#include "raylib.h"
#include "rlgl.h"
#include "game.h"

#include "dialog.h"

#include "../external/clay/clay.h"
#include "../external/clay/clay_renderer_raylib.h"


static uint32_t total_chars;
static uint32_t printed_chars;
static Game_Timer char_timer;
static Dialog_Item current_dialog = NULL;

typedef struct {
    string name;
} Dialog_Selection;

static bool first_time;
static int dialog_selection_index;

static jmp_buf dialog_jump_buf, dialog_jump_back_buf;

static Shader background_shader;

int _dialog_selection(string prompt, int count, const char* items[]) {
    int result = -1;

    if (first_time) {
        dialog_selection_index = 0;
        first_time = false;
    }

    if (IsKeyPressed(KEY_RIGHT)) {
        dialog_selection_index = min(dialog_selection_index + 1, count - 1);
    } else if (IsKeyPressed(KEY_LEFT)) {
        dialog_selection_index = max(dialog_selection_index - 1, 0);
    }

    if (IsKeyPressed(KEY_ENTER)) {
        result = dialog_selection_index;
    }

    CustomLayoutElement* customBackgroundData = oc_arena_alloc(&frame_arena, sizeof(CustomLayoutElement));
    customBackgroundData->type = CUSTOM_LAYOUT_ELEMENT_TYPE_BACKGROUND;
    customBackgroundData->customData.background = (CustomLayoutElement_Background) { background_shader };

    DialogTextUserData* textUserData = oc_arena_alloc(&frame_arena, sizeof(DialogTextUserData));
    textUserData->visible_chars = printed_chars;

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
            CLAY_TEXT(((Clay_String) { .length = prompt.len, .chars = prompt.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = 2, .textColor = {255, 255, 255, 255} }));
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
                if (i == dialog_selection_index) {
                    color = (Clay_Color) { 180, 180, 180, 80 };
                } else {
                    color = (Clay_Color) { 0, 0, 0, 0 };
                }
                CLAY(CLAY_IDI("SelectionItem", i), {
                    .layout = { .padding = {16, 16, 2, 2} },
                    .backgroundColor = Clay_Hovered() ? (Clay_Color) { 180, 180, 180, 40 } : color,
                    .cornerRadius = CLAY_CORNER_RADIUS(4)
                }) {
                    CLAY_TEXT(((Clay_String) { .length = strlen(items[i]), .chars = items[i] }), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = 1, .textColor = {255, 255, 255, 255} }));
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
            if(total_chars == printed_chars) {
                CLAY_TEXT((CLAY_STRING("Enter to Select")), CLAY_TEXT_CONFIG({ .fontSize = 25, .fontId = 3, .textColor = {135, 135, 135, 255} }));
            }
        }
    }

    return result;
}

bool _dialog_text(string speaker_name, string text, Dialog_Parameters parameters) {

    // draw the text
    if (first_time) {
        total_chars = text.len;
        printed_chars = 0;
        timer_init(&char_timer, 30);
        first_time = false;
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
        if (IsKeyPressed(KEY_SPACE)) {
            result = true;
        }
    }

    string display_str = string_slice(text, 0, printed_chars);

    CustomLayoutElement* customBackgroundData = oc_arena_alloc(&frame_arena, sizeof(CustomLayoutElement));
    customBackgroundData->type = CUSTOM_LAYOUT_ELEMENT_TYPE_BACKGROUND;
    customBackgroundData->customData.background = (CustomLayoutElement_Background) { background_shader };

    DialogTextUserData* textUserData = oc_arena_alloc(&frame_arena, sizeof(DialogTextUserData));
    textUserData->visible_chars = printed_chars;

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
            CLAY_TEXT(((Clay_String) { .length = speaker_name.len, .chars = speaker_name.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = 2, .textColor = {255, 255, 255, 255} }));
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
                CLAY_TEXT(((Clay_String) { .length = text.len, .chars = text.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = 1, .textColor = {255, 255, 255, 255}, .userData = textUserData }));
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
            if(total_chars == printed_chars) {
                CLAY_TEXT((CLAY_STRING("Space to Continue")), CLAY_TEXT_CONFIG({ .fontSize = 25, .fontId = 3, .textColor = {135, 135, 135, 255} }));
            }
        }
    }

    return result;
}

static void* stack = NULL, *old_stack = NULL;

void sample_dialog(void) {
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

    dialog_end();
}

void dialog_play(Dialog_Item dialog) {
    if (!stack) {
        stack = malloc(10 * 1024);
    }
    first_time = true;
    current_dialog = dialog;


    uintptr_t top_of_stack = oc_align_forward(((uintptr_t)stack) + 10 * 1024 - 16, 16);

    asm volatile("mov %%rsp, %0" : "=r" (old_stack));
    asm volatile("mov %0, %%rsp" :: "r" (top_of_stack));
    if (my_setjmp(dialog_jump_back_buf) == 0) {
        sample_dialog();
    }
    asm volatile("mov %0, %%rsp" :: "r" (old_stack));
}

bool dialog_is_done(void) {
    return current_dialog == NULL;
}

void dialog_update(void) {
    if(!current_dialog) return;

    bool before_ft = first_time;


    asm volatile("mov %%rsp, %0" : "=r" (old_stack));
    if (my_setjmp(dialog_jump_back_buf) == 0) {
        my_longjmp(dialog_jump_buf, 1);
    }
    asm volatile("mov %0, %%rsp" :: "r" (old_stack));

    if (first_time && before_ft) {
        current_dialog = NULL;
    }
}

void dialog_init() {
    background_shader = LoadShader(0, "resources/dialogbackground.fs");
}

void dialog_cleanup() {
    UnloadShader(background_shader);
}
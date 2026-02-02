#pragma once

#include "game.h"
#include "jump.h"

Enum(Dialog_Decoration_Type, uint32_t,
    DIALOG_TYPE_DEFAULT = 0,
    DIALOG_TYPE_ANGRY,
    DIALOG_TYPE_SURPRISED    
);

typedef struct {
    Dialog_Decoration_Type decoration;
    float desired_mood;
} Dialog_Parameters;

typedef void (*Dialog_Item)();

void dialog_play(Dialog_Item dialog);
void dialog_update(void);

void sample_dialog(void);

bool dialog_is_done(void);
void dialog_init();
void dialog_cleanup();

bool _dialog_text(string speaker_name, string text, Dialog_Parameters parameters);
int _dialog_selection(string prompt, int count, const char* items[]);

#define CSTR_TO_STRING(str) (_Generic((str), string : (str), default: lit(str)))

#define dialog_selection(prompt, ...) ({                                       \
    int selection;                                                                   \
    if (my_setjmp(dialog_jump_buf) == 0) {                                           \
        first_time = true;                                                           \
        my_longjmp(dialog_jump_back_buf, 1);                                     \
    }                                                                                \
    {                                                                                \
        const char* items[] = { __VA_ARGS__ };                                       \
        selection = _dialog_selection(CSTR_TO_STRING(prompt), oc_len(items), items); \
        if (selection < 0) {                                                         \
            my_longjmp(dialog_jump_back_buf, 1);                                     \
        }                                                                            \
    }                                                                                \
    selection;                                                                       \
})

#define dialog_text(speaker_name, text, ...) do {                                                                                       \
    if (my_setjmp(dialog_jump_buf) == 0) {                                                                                              \
        first_time = true;                                                                                                              \
        my_longjmp(dialog_jump_back_buf, 1);                                                                                            \
    }                                                                                                                                   \
    if (!_dialog_text(CSTR_TO_STRING(speaker_name), CSTR_TO_STRING(text), (Dialog_Parameters) { .desired_mood = 0.0f, __VA_ARGS__ })) { \
        my_longjmp(dialog_jump_back_buf, 1);                                                                                            \
    }                                                                                                                                   \
} while (0)

#define dialog_end() do { current_dialog = NULL; my_longjmp(dialog_jump_back_buf, 1); } while (0)
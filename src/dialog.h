#pragma once

#include "game.h"

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

#define dialog_selection(prompt, ...) ({                                             \
    int selection;                                                                   \
    if (dialog_current_item < dialog_max_item) {                                     \
        dialog_current_item++;                                                       \
        selection = dialog_branches[dialog_current_branch++];                        \
    } else if (dialog_current_item == dialog_max_item) {                             \
        const char* items[] = { __VA_ARGS__ };                                       \
        selection = _dialog_selection(CSTR_TO_STRING(prompt), oc_len(items), items); \
        if (selection < 0) {                                                         \
            first_time = false;                                                      \
            return;                                                                  \
        } else {                                                                     \
            dialog_branches[dialog_current_branch++] = selection;                    \
            dialog_current_item++;                                                   \
            dialog_max_item++;                                                       \
            first_time = true;                                                       \
            return;                                                                  \
        }                                                                            \
    } else {                                                                         \
        oc_assert(false);                                                            \
    }                                                                                \
    selection;                                                                       \
})

#define dialog_text(speaker_name, text, ...) do {                                              \
    if (dialog_current_item < dialog_max_item) {                                                      \
        dialog_current_item++;                                                                        \
    } else if (dialog_current_item == dialog_max_item) {                                              \
        if (_dialog_text(CSTR_TO_STRING(speaker_name), CSTR_TO_STRING(text), ((Dialog_Parameters) { .desired_mood = 0.0, __VA_ARGS__ }))) { \
            dialog_current_item++;                                                                    \
            dialog_max_item++;                                                                        \
            first_time = true;                                                                        \
            return;                                                                                   \
        } else {                                                                                      \
            first_time = false;                                                                       \
            return;                                                                                   \
        }                                                                                             \
    } else oc_assert(false);                                                                          \
} while (0)
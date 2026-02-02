#pragma once

#include "dialog.h"

typedef void (*Encounter)(void);

extern bool encounter_first_time;

void encounter_start(Encounter encounter);
void encounter_update(void);
bool encounter_is_done(void);
void sample_encounter(void);

extern jmp_buf encounter_jump_buf, encounter_jump_back_buf;

#define CSTR_TO_STRING(str) (_Generic((str), string : (str), default: lit(str)))

#define dialog_selection(prompt, ...) ({                                             \
    int selection;                                                                   \
    if (my_setjmp(encounter_jump_buf) == 0) {                                        \
        encounter_first_time = true;                                                 \
        my_longjmp(encounter_jump_back_buf, 1);                                      \
    }                                                                                \
    {                                                                                \
        const char* items[] = { __VA_ARGS__ };                                       \
        selection = _dialog_selection(CSTR_TO_STRING(prompt), oc_len(items), items); \
        if (selection < 0) {                                                         \
            my_longjmp(encounter_jump_back_buf, 1);                                  \
        }                                                                            \
    }                                                                                \
    selection;                                                                       \
})

#define dialog_text(speaker_name, text, ...) do {                                                                                       \
    if (my_setjmp(encounter_jump_buf) == 0) {                                                                                           \
        encounter_first_time = true;                                                                                                    \
        my_longjmp(encounter_jump_back_buf, 1);                                                                                         \
    }                                                                                                                                   \
    if (!_dialog_text(CSTR_TO_STRING(speaker_name), CSTR_TO_STRING(text), (Dialog_Parameters) { .desired_mood = 0.0f, __VA_ARGS__ })) { \
        my_longjmp(encounter_jump_back_buf, 1);                                                                                         \
    }                                                                                                                                   \
} while (0)

#define encounter_minigame(minigame) do {       \
    if (my_setjmp(encounter_jump_buf) == 0) {   \
        encounter_first_time = true;            \
        (minigame)->init();                     \
        my_longjmp(encounter_jump_back_buf, 1); \
    }                                           \
    if (!(minigame)->update()) {                \
        my_longjmp(encounter_jump_back_buf, 1); \
    }                                           \
} while (0)

#define encounter_end() do { current_encounter = NULL; my_longjmp(encounter_jump_back_buf, 1); } while (0)

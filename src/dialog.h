#pragma once

#include "game.h"
#include "jump.h"

/* Core Dialog System */

#define CSTR_TO_STRING(str) (_Generic((str), string : (str), default: lit(str)))

typedef void (*Dialog_Item)();

void dialog_play(Dialog_Item dialog);
void dialog_update(void);

bool dialog_is_done(void);

/* Types of Dialog */

void dialog_init();
void dialog_cleanup();

typedef struct {
    uint32_t total_chars;
    uint32_t printed_chars;
    Game_Timer char_timer;
} Dialog_Text_Data;

typedef struct {
    int index;
} Dialog_Selection_Data;

static union {
    Dialog_Text_Data text;
    Dialog_Selection_Data selection;
} data;

bool _dialog_text(string speaker_name, string text);
int _dialog_selection(string prompt, int count, const char* items[]);

#define dialog_text(speaker_name, text) do {                                             \
    if (my_setjmp(dialog_jump_buf) == 0) {                                               \
        first_time = true;                                                               \
        my_longjmp(dialog_jump_back_buf, 1);                                             \
    }                                                                                    \
    if (!_dialog_text(CSTR_TO_STRING(speaker_name), CSTR_TO_STRING(text))) {             \
        my_longjmp(dialog_jump_back_buf, 1);                                             \
    }                                                                                    \
} while (0)

#define dialog_selection(prompt, ...) ({                                             \
    int selection;                                                                   \
    if (my_setjmp(dialog_jump_buf) == 0) {                                           \
        first_time = true;                                                           \
        my_longjmp(dialog_jump_back_buf, 1);                                         \
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

#define dialog_end() do { current_dialog = NULL; my_longjmp(dialog_jump_back_buf, 1); } while (0)


/* Dialog Paths */

void sample_dialog(void);
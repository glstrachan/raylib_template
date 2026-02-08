#pragma once

#include "dialog.h"

extern Encounter_Sequence encounter_top_sequence;

void encounter_start(Encounter_Fn encounter);
void encounter_update(void);
void encounter_sequence_start(Encounter_Sequence* sequence, Encounter_Fn encounter);
void encounter_sequence_update(Encounter_Sequence* sequence);

bool encounter_is_done(void);
void sample_encounter(void);

void pick_encounter_init(void);
void pick_encounter(void);
void pick_encounter_update(void);

extern jmp_buf encounter_jump_buf, encounter_jump_back_buf;
extern Encounter_Sequence encounter_top_sequence;

extern Sound start_sounds[];

#define CSTR_TO_STRING(str) (_Generic((str), string : (str), default: lit(str)))

#define dialog_options(...) ((const char *[]){ __VA_ARGS__ })
#define dialog_selection(prompt, elements, ...) ({                                             \
    int selection;                                                                   \
    if (my_setjmp(__current_sequence->jump_buf) == 0) {                                        \
        __current_sequence->first_time = true;                                                 \
        my_longjmp(__current_sequence->jump_back_buf, 1);                                      \
    }                                                                                \
    {                                                                                \
        const char* items[] = (elements);                                       \
        selection = _dialog_selection(__current_sequence, CSTR_TO_STRING(prompt), oc_len(items), items, ((Dialog_Parameter) { .place_above_dialog = NULL, __VA_ARGS__ })); \
        if (selection < 0) {                                                         \
            my_longjmp(__current_sequence->jump_back_buf, 1);                                  \
        }                                                                            \
    }                                                                                \
    selection;                                                                       \
})

#define dialog_yield() my_longjmp(__current_sequence->jump_back_buf, 1)

#define dialog_text(speaker_name, text, ...) do {                                                                                       \
    if (my_setjmp(__current_sequence->jump_buf) == 0) {                                                                                           \
        __current_sequence->first_time = true;                                                                                                    \
        my_longjmp(__current_sequence->jump_back_buf, 1);                                                                                         \
    }                                                                                                                                   \
    if (!_dialog_text(__current_sequence, character_data[(speaker_name)].name, CSTR_TO_STRING(text), ((Dialog_Parameter) { .place_above_dialog = NULL, __VA_ARGS__ }))) { \
        my_longjmp(__current_sequence->jump_back_buf, 1);                                                                                         \
    }                                                                                                                                   \
} while (0)

#ifdef _DEBUG
#define encounter_minigame(minigame) do {       \
    if (my_setjmp(__current_sequence->jump_buf) == 0) {   \
        __current_sequence->first_time = true;            \
        (minigame)->init();                     \
        /* Play a sounds*/                      \
        PlaySound(start_sounds[game.current_character - CHARACTERS_OLDLADY]); \
        my_longjmp(__current_sequence->jump_back_buf, 1); \
    }                                           \
    if (!(minigame)->update() && !IsKeyPressed(KEY_F)) {                \
        my_longjmp(__current_sequence->jump_back_buf, 1); \
    }                                           \
    if ((minigame)->cleanup) (minigame)->cleanup(); \
    game.current_prerender = NULL; \
} while (0)
#else

#define encounter_minigame(minigame) do {       \
    if (my_setjmp(__current_sequence->jump_buf) == 0) {   \
        __current_sequence->first_time = true;            \
        (minigame)->init();                     \
        /* Play a sounds*/                      \
        PlaySound(start_sounds[game.current_character - CHARACTERS_OLDLADY]); \
        my_longjmp(__current_sequence->jump_back_buf, 1); \
    }                                           \
    if (!(minigame)->update()) {                \
        my_longjmp(__current_sequence->jump_back_buf, 1); \
    }                                           \
    if ((minigame)->cleanup) (minigame)->cleanup(); \
    game.current_prerender = NULL; \
} while (0)
#endif

#define encounter_begin(...) Encounter_Sequence* __current_sequence = (&encounter_top_sequence, ## __VA_ARGS__ /* omg lol */)
#define encounter_end() do { __current_sequence->is_done = true; my_longjmp(__current_sequence->jump_back_buf, 1); } while (0)

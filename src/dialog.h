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
    Game_Timer char_timer, timeout_timer;
} Dialog_Text_Data;

typedef struct {
    int index;
} Dialog_Selection_Data;

typedef void (*Encounter_Fn)(void);

typedef struct Encounter {
    Encounter_Fn fn;
    string name;
} Encounter;

typedef struct {
    Encounter_Fn encounter;
    jmp_buf jump_buf, jump_back_buf;
    void *stack, *old_stack;
    bool first_time;
    bool is_done;
} Encounter_Sequence;

typedef struct {
    void (*place_above_dialog)(void);
    float timeout;
} Dialog_Parameter;

bool _dialog_text(Encounter_Sequence* sequence, string speaker_name, string text, Dialog_Parameter parameters);
int _dialog_selection(Encounter_Sequence* sequence, string prompt, int count, const char* items[]);

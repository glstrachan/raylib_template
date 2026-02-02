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

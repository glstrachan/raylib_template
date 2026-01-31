#pragma once


typedef void (*Dialog_Item)();

void dialog_play(Dialog_Item dialog);
void dialog_update(void);

void sample_dialog(void);

void dialog_init();
void dialog_cleanup();
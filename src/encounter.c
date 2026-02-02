
#include "encounter.h"
#include "jump.h"

Encounter_Sequence encounter_top_sequence = { 0 };
static Encounter current_encounter;

void encounter_start(Encounter encounter) {
    current_encounter = encounter;
    encounter_sequence_start(&encounter_top_sequence, encounter);
}

void encounter_update(void) {
    if (!current_encounter) return;
    encounter_sequence_update(&encounter_top_sequence);
}

void encounter_sequence_start(Encounter_Sequence* sequence, Encounter encounter) {
    if (!sequence->stack) {
        sequence->stack = malloc(10 * 1024);
    }
    sequence->encounter = encounter;
    sequence->first_time = true;

    uintptr_t top_of_stack = oc_align_forward(((uintptr_t)sequence->stack) + 10 * 1024 - 16, 16);

    static Encounter_Sequence* this_sequence;
    this_sequence = sequence; // needed since we modify rsp
    asm volatile("mov %%rsp, %0" : "=r" (this_sequence->old_stack));
    asm volatile("mov %0, %%rsp" :: "r" (top_of_stack));
    if (my_setjmp(this_sequence->jump_back_buf) == 0) {
        this_sequence->encounter();
    }
    asm volatile("mov %0, %%rsp" :: "r" (this_sequence->old_stack));
}

void encounter_sequence_update(Encounter_Sequence* sequence) {
    asm volatile("mov %%rsp, %0" : "=r" (sequence->old_stack));
    if (my_setjmp(sequence->jump_back_buf) == 0) {
        my_longjmp(sequence->jump_buf, 1);
    }
    asm volatile("mov %0, %%rsp" :: "r" (sequence->old_stack));
}

bool encounter_is_done(void) {
    return encounter_top_sequence.is_done;
}

void sample_encounter(void) {
    encounter_begin();

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

    extern Minigame memory_game, smile_game;
    encounter_minigame(&memory_game);

    dialog_text("Old Lady", "Wow impressive!");
    dialog_text("Old Lady", "Now try this");

    encounter_minigame(&smile_game);

    encounter_end();
}


#include "encounter.h"
#include "jump.h"

bool encounter_first_time;
jmp_buf encounter_jump_buf, encounter_jump_back_buf;
static Encounter current_encounter;
static void* stack = NULL, *old_stack = NULL;

void encounter_start(Encounter encounter) {
    if (!stack) {
        stack = malloc(10 * 1024);
    }
    encounter_first_time = true;
    current_encounter = encounter;


    uintptr_t top_of_stack = oc_align_forward(((uintptr_t)stack) + 10 * 1024 - 16, 16);

    asm volatile("mov %%rsp, %0" : "=r" (old_stack));
    asm volatile("mov %0, %%rsp" :: "r" (top_of_stack));
    if (my_setjmp(encounter_jump_back_buf) == 0) {
        current_encounter();
    }
    asm volatile("mov %0, %%rsp" :: "r" (old_stack));
}

void encounter_update(void) {
    if(!current_encounter) return;

    asm volatile("mov %%rsp, %0" : "=r" (old_stack));
    if (my_setjmp(encounter_jump_back_buf) == 0) {
        my_longjmp(encounter_jump_buf, 1);
    }
    asm volatile("mov %0, %%rsp" :: "r" (old_stack));
}

bool encounter_is_done(void) {
    return current_encounter == NULL;
}

void sample_encounter(void) {
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

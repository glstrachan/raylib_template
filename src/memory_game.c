#include "raylib.h"
#include "rlgl.h"
#include "game.h"
#include "../external/clay/clay.h"
#include "../external/clay/clay_renderer_raylib.h"

static float timer;
static uint32_t game_round;

Enum(Memory_State, uint32_t,
    MEMORY_DISPLAYING,
    MEMORY_RECREATE
);

void memory_game_init() {
    // timer = GetTime()
}

bool memory_game_update() {

}

Minigame memory_game = {
    .init = memory_game_init,
    .update = memory_game_update,
};
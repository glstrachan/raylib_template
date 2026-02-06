#include "dialog.h"
#include "encounter.h"

#include <raylib.h>
#include "game.h"

#include "pick_items.h"

typedef struct {
    Item_Type item_type;
    Vector2 position;
    Vector2 velocity;
    bool destroyed;
    float cooldown;
} Shotgun_Game_Item;

static Array(Shotgun_Game_Item) shotgun_game_items;

static float shot_cooldown;
static Texture2D bg_tex;

void shotgun_game_init() {
    bg_tex = LoadTexture("resources/background.png"); // Needs an unload

    // Basic test
    oc_array_append(&arena, &shotgun_game_items, ((Shotgun_Game_Item) {
        ITEM_AK47, 
        (Vector2) {100, 100}, 
        (Vector2) {1, -1},
        false,
        100.0f
    }));
    oc_array_append(&arena, &shotgun_game_items, ((Shotgun_Game_Item) {
        ITEM_COMPUTER, 
        (Vector2) {800, 100}, 
        (Vector2) {-1, -1},
        false,
        100.0f
    }));
}

bool shotgun_game_update() {
    HideCursor();

    // We may spawn a new item here depending on stuff
    // We check if the user is trying to shoot and if they hit an item

    // Draw Background
    DrawTexture(bg_tex, 0, 0, WHITE);

    // Draw all the items
    for(uint32_t i = 0; i < shotgun_game_items.count; i++) {
        Shotgun_Game_Item* game_item = &shotgun_game_items.items[i];

        game_item->velocity = Vector2Subtract(game_item->velocity, Vector2Scale((Vector2) {0.0, -1.5}, GetFrameTime()));
        game_item->position = Vector2Add(game_item->position, Vector2Scale(game_item->velocity, GetFrameTime() * 400)); 
        
        DrawTexture(item_data[game_item->item_type].texture, game_item->position.x, game_item->position.y, WHITE);
    }

    // Draw aim cursor
    Vector2 mouse = (Vector2) { (float)GetMouseX(), (float)GetMouseY() };
    DrawCircle(mouse.x, mouse.y, 30, (Color) {255, 0, 0, 86});
    DrawCircle(mouse.x, mouse.y, 5, (Color) {255, 0, 0, 255});
    DrawRing(mouse, 25, 30, 0, 360, 100, ((Color) {255, 0, 0, 255}));

    return false;
}

Minigame shotgun_game = {
    .init = shotgun_game_init,
    .update = shotgun_game_update,
};

// Game structure

// So there is several rounds

// and in each round you play a variant of the minigame

// In the most basic variant

// There is a set of items that will spawn in

// We spawn them in

// Basically score the user based on if they click on an item successfully



// How does the state machine or whatever work

// 
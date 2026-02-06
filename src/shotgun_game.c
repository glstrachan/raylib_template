#include "dialog.h"
#include "encounter.h"

#include <raylib.h>
#include "game.h"

#include "pick_items.h"

Enum(Shotgun_Item_State, uint32_t,
    SHOTGUN_ITEM_STATE_UNSPAWNED,
    SHOTGUN_ITEM_STATE_ALERTING,
    SHOTGUN_ITEM_STATE_SPAWNED,
    SHOTGUN_ITEM_STATE_DESTROYED
);

typedef struct {
    Item_Type item_type;
    Vector2 position;
    Vector2 velocity;
    Shotgun_Item_State state;
    float cooldown;
    float spawn_time;
} Shotgun_Game_Item;

static Array(Shotgun_Game_Item) shotgun_game_items;

static Texture2D bg_tex;
static Texture2D bam_tex;
static Texture2D alert_tex;

static float start_time;

static inline float get_random_float() {
    return ((float)GetRandomValue(0, INT_MAX) / (float)INT_MAX);
}

static void shotgun_game_add_items(int n) {
    for(int i = 0; i < 30; i++) {
            oc_array_append(&arena, &shotgun_game_items, ((Shotgun_Game_Item) {
            GetRandomValue(0, ITEM_COUNT - 1),
            (Vector2) {
                GetRandomValue(0, 1920),
                GetRandomValue(0, 1080)
            },
            Vector2Normalize((Vector2) { get_random_float() - 0.5, get_random_float() - 0.5 }),
            SHOTGUN_ITEM_STATE_UNSPAWNED,
            0.2f,
            10.0 * get_random_float()
        }));
    }
    
}

void shotgun_game_init() {
    bg_tex = LoadTexture("resources/background.png"); // Needs an unload
    bam_tex = LoadTexture("resources/bam.png");       // Same
    alert_tex = LoadTexture("resources/alert.png");   // Same

    start_time = GetTime();
    
    shotgun_game_add_items(20);
    
    // Basic test
    // oc_array_append(&arena, &shotgun_game_items, ((Shotgun_Game_Item) {
    //     ITEM_AK47, 
    //     (Vector2) {100, 100},
    //     (Vector2) {1, -1},
    //     SHOTGUN_ITEM_STATE_UNSPAWNED,
    //     0.2f,
    //     1.5
    // }));
}

bool shotgun_game_update() {
    float item_hit_radius = 120.0f;

    Vector2 mouse = (Vector2) { (float)GetMouseX(), (float)GetMouseY() };

    // We check if the user is trying to shoot and if they hit an item
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for(uint32_t i = 0; i < shotgun_game_items.count; i++) {
            Shotgun_Game_Item* game_item = &shotgun_game_items.items[i];

            // Check if we are close enough to hitting this object
            if(game_item->state == SHOTGUN_ITEM_STATE_SPAWNED) {
                float dist = Vector2Distance(mouse, game_item->position);
                
                if(dist < item_hit_radius) {
                    game_item->state = SHOTGUN_ITEM_STATE_DESTROYED;
                }
            }
        }
    }

    // Cleanup old items we dont need to keep around anymore
    uint32_t length = 0;

    for(uint32_t i = 0; i < shotgun_game_items.count; i++) {
        Shotgun_Game_Item* game_item = &shotgun_game_items.items[i];

        if(game_item->state == SHOTGUN_ITEM_STATE_DESTROYED) {
            game_item->cooldown -= GetFrameTime();
        }

        if(game_item->cooldown > 0.0f) {
            shotgun_game_items.items[length] = shotgun_game_items.items[i];
            length++;
        }
    }

    shotgun_game_items.count = length;

    HideCursor();

    // Draw Background
    DrawTexture(bg_tex, 0, 0, WHITE);

    // Draw all the items
    for(uint32_t i = 0; i < shotgun_game_items.count; i++) {
        Shotgun_Game_Item* game_item = &shotgun_game_items.items[i];
        Texture2D item_texture = item_data[game_item->item_type].texture;

        switch(game_item->state) {
            case SHOTGUN_ITEM_STATE_UNSPAWNED: {
                if(GetTime() - start_time > game_item->spawn_time) {
                    game_item->state = SHOTGUN_ITEM_STATE_ALERTING;
                }
            } break;
            case SHOTGUN_ITEM_STATE_ALERTING: {
                if(GetTime() - start_time > game_item->spawn_time + 0.5) {
                    game_item->state = SHOTGUN_ITEM_STATE_SPAWNED;
                }

                DrawTexture(alert_tex, game_item->position.x - alert_tex.width * 0.5, game_item->position.y - alert_tex.height * 0.5, WHITE);
            } break;
            case SHOTGUN_ITEM_STATE_SPAWNED: {
                game_item->velocity = Vector2Subtract(game_item->velocity, Vector2Scale((Vector2) {0.0, -1.5}, GetFrameTime()));
                game_item->position = Vector2Add(game_item->position, Vector2Scale(game_item->velocity, GetFrameTime() * 400)); 
                DrawTexture(item_texture, game_item->position.x - item_texture.width * 0.5, game_item->position.y - item_texture.height * 0.5, WHITE);
            } break;
            case SHOTGUN_ITEM_STATE_DESTROYED: {
                DrawTexture(bam_tex, game_item->position.x - bam_tex.width * 0.5, game_item->position.y - bam_tex.height * 0.5, WHITE);
            } break;
        }

        // Draws the hitbox
        // DrawCircle(game_item->position.x, game_item->position.y, item_hit_radius, (Color) {255, 0, 0, 100});
    }

    // Draw aim cursor
    DrawCircle(mouse.x, mouse.y, 30, (Color) {255, 0, 0, 86});
    DrawCircle(mouse.x, mouse.y, 5, (Color) {255, 0, 0, 255});
    DrawRing(mouse, 25, 30, 0, 360, 100, ((Color) {255, 0, 0, 255}));

    game_objective_widget(lit("Shoot the right items!"));

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
#include "dialog.h"
#include "encounter.h"

#include <limits.h>
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
static bool ended;
static float end_time;

static uint32_t num_items;
static uint32_t score_multiplier = 500;
static uint32_t score;

static float base_velocity;

static Sound flingSound;
static Sound alertSound;
static Sound explodeSound;
static Sound shootSound;

static inline float get_random_float() {
    return ((float)GetRandomValue(0, INT_MAX) / (float)INT_MAX);
}

static void shotgun_game_add_items() {
    // Periodically space out when things spawn
    // also initial locations to be just outside the perimeter of the screen
    // and it chooses a initial velocity pointing towards a point along the center of the screen

    float spawn_time = 0.0f;

    for(int i = 0; i < num_items; i++) {
            spawn_time += get_random_float() * 1.0 + 0.1;

            Vector2 spawn_location = (get_random_float() > 0.5) ? (Vector2) {0 - 50, GetRandomValue(20, 900)} : (Vector2) {1920 + 50, GetRandomValue(20, 900)};
            Vector2 center_point = (Vector2) {1920 / 2, GetRandomValue(-200, 700)};

            oc_array_append(&arena, &shotgun_game_items, ((Shotgun_Game_Item) {
            GetRandomValue(0, ITEM_COUNT - 1),
            spawn_location,
            Vector2Scale(Vector2Normalize(Vector2Subtract(center_point, spawn_location)), get_random_float() * 3.5f + 1.5f),
            SHOTGUN_ITEM_STATE_UNSPAWNED,
            0.2f,
            spawn_time
        }));
    }
    
}

void shotgun_game_init() {
    bg_tex = LoadTexture("resources/background.png"); // Needs an unload
    bam_tex = LoadTexture("resources/bam.png");       // Same
    alert_tex = LoadTexture("resources/alert.png");   // Same

    start_time = GetTime();
    score = 0;

    ended = false;

    flingSound = LoadSound("resources/sounds/duck-hunt_fling.wav");
    alertSound = LoadSound("resources/sounds/duck-hunt_warning.wav");
    explodeSound = LoadSound("resources/sounds/duck-hunt_explosion.wav");
    shootSound = LoadSound("resources/sounds/duck-hunt_shoot.wav");
    
    // TODO: Change this based on current day

    // depending on the day make it more items

    num_items = (game.current_day + 1) * 25;
    base_velocity = (float)game.current_day * 1.0;

    shotgun_game_add_items();
}

bool shotgun_game_update() {
    float item_hit_radius = 120.0f;

    Vector2 mouse = (Vector2) { (float)GetMouseX(), (float)GetMouseY() };

    // We check if the user is trying to shoot and if they hit an item
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySound(shootSound);
        
        for(uint32_t i = 0; i < shotgun_game_items.count; i++) {
            Shotgun_Game_Item* game_item = &shotgun_game_items.items[i];

            // Check if we are close enough to hitting this object
            if(game_item->state == SHOTGUN_ITEM_STATE_SPAWNED) {
                float dist = Vector2Distance(mouse, game_item->position);
                
                if(dist < item_hit_radius) {
                    if(game_item->item_type == game.current_item) {
                        score = max(0, (int)score - 3);
                    }
                    else {
                        score += 1;
                    }
                    
                    game_item->state = SHOTGUN_ITEM_STATE_DESTROYED;
                    PlaySound(explodeSound);
                }
            }
        }
    }

    // Cleanup old items we dont need to keep around anymore
    uint32_t length = 0;

    for(uint32_t i = 0; i < shotgun_game_items.count; i++) {
        Shotgun_Game_Item* game_item = &shotgun_game_items.items[i];

        if(game_item->position.y > 1500) {
            game_item->state = SHOTGUN_ITEM_STATE_DESTROYED;
        }

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

    // Draw UI bs
    // TODO

    game_objective_widget(oc_format(&frame_arena, "Shoot any item that isnt {}", item_data[game.current_item].name));

    // Draw the score
    CLAY_AUTO_ID({
        .floating = { .offset = { 16, 100 }, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_RIGHT_TOP, CLAY_ATTACH_POINT_RIGHT_TOP } },
        .layout = {
            .padding = {10, 25, 0, 0},
            .sizing = {
                .width = CLAY_SIZING_FIT(0),
                .height = CLAY_SIZING_FIT(0)
            },
        },
        .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
        .custom = { .customData = make_cool_background() },
        .cornerRadius = CLAY_CORNER_RADIUS(6),
    }) {
        CLAY_TEXT((oc_format(&frame_arena, "Score: {}", score * score_multiplier)), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
    }

    // Draw all the items
    for(uint32_t i = 0; i < shotgun_game_items.count; i++) {
        Shotgun_Game_Item* game_item = &shotgun_game_items.items[i];
        Texture2D item_texture = item_data[game_item->item_type].texture;

        switch(game_item->state) {
            case SHOTGUN_ITEM_STATE_UNSPAWNED: {
                if(GetTime() - start_time > game_item->spawn_time) {
                    game_item->state = SHOTGUN_ITEM_STATE_ALERTING;
                    PlaySound(alertSound);
                }
            } break;
            case SHOTGUN_ITEM_STATE_ALERTING: {
                if(GetTime() - start_time > game_item->spawn_time + 1.0) {
                    game_item->state = SHOTGUN_ITEM_STATE_SPAWNED;
                    PlaySound(flingSound);
                }
                
                if(game_item->position.x < 0) {
                    DrawTexture(alert_tex, game_item->position.x - alert_tex.width * 0.5 + 100, game_item->position.y - alert_tex.height * 0.5, WHITE);
                }
                else {
                    DrawTexture(alert_tex, game_item->position.x - alert_tex.width * 0.5 - 100, game_item->position.y - alert_tex.height * 0.5, WHITE);
                }
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

    // Check if we should end the game
    if(shotgun_game_items.count == 0) {
        if(!ended) {
            ended = true;
            end_time = GetTime();
        }
        else if(GetTime() - end_time > 1.0f) {
            game_submit_minigame_score((float)score / (float)num_items);
            ShowCursor();

            return true;
        }
    }

    return false;
}

Minigame shotgun_game = {
    .init = shotgun_game_init,
    .update = shotgun_game_update,
};

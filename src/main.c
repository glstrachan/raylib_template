#include "raylib.h"
#include "rlgl.h"

#define OC_CORE_IMPLEMENTATION
#include "entity.h"
#include "game.h"
// #include "memory_game.h"

void memory_game_init(Game_Parameters parameters);
void memory_game_update(Game_Parameters parameters);

#if 0
#define WORLD_TO_SCREEN (150.0f)
#define PLAYER_SPEED (10.0f)
#define PLAYER_SIZE 1

EntityId player_id;
EntityId interacted_with = 0;

Entity entities[ENTITIES_MAX_COUNT] = { 0 };
EntityId next_entity_id = 1; // 0 is for null entity reference

static inline float lerpf(float a, float b, float t) {
    return b * t + (1.0f - t) * a;
}

const float ground_y = 0.0f;
const Color player_color = { 255, 0, 0, 255 };
#define each_entity(_id) (EntityId _id = 1; _id < next_entity_id; ++_id) if (entities[_id].type != ENTITY_INVALID)

static inline void GameDrawRectangle(Vector2 pos, Vector2 size, Color color) {
    pos.y = 1080/WORLD_TO_SCREEN - pos.y - size.y;
    DrawRectangleV(pos, size, color);
}

static inline void GameDrawCircle(Vector2 pos, float radius, Color color) {
    pos.y = 1080/WORLD_TO_SCREEN - pos.y;
    DrawCircleV(pos, radius, color);
}

typedef struct {
    uint8_t type;
} GridTile;

typedef struct {
    GridTile* tiles;
    uint32_t width, height;
} WorldGrid;

void draw_grid() {
    Entity* p = &entities[player_id];
    uint32_t start_x = max((int64_t)p->position.x - 1920LL/WORLD_TO_SCREEN, 0LL);
    uint32_t start_y = max((int64_t)p->position.y - 1080LL/WORLD_TO_SCREEN, 0LL);
    for (uint32_t y = start_y; y < start_y + 1080LL/WORLD_TO_SCREEN; ++y) {
        for (uint32_t x = start_x; x < start_x + 1920LL/WORLD_TO_SCREEN; ++x) {
            // GameDrawRectangle()
            // DrawRectangleLines(x + 1920LL/WORLD_TO_SCREEN/2.0, 1080/WORLD_TO_SCREEN - y, 1, 1, (Color){0, 0, 0, 255});
            Rectangle rectangle = {
                .x = x + 1920LL/WORLD_TO_SCREEN/2.0,
                .y = 1080/WORLD_TO_SCREEN - y + 1920LL/WORLD_TO_SCREEN/2.0,
                .width = 1,
                .height = 1,
            };
            DrawRectangleLinesEx(rectangle, 0.01, (Color){0, 0, 0, 255});
        }
    }
}

EntityId entity_new(EntityType type) {
    memset(&entities[next_entity_id], 0, sizeof(entities[next_entity_id]));
    entities[next_entity_id].type = type;
    if (type == ENTITY_PLAYER) player_id = next_entity_id;
    next_entity_id++;
    return next_entity_id - 1;
}

void entity_remove(EntityId entity) {
    entities[entity].type = ENTITY_INVALID;
}

void entity_update(EntityId id) {
    Entity* e = &entities[id];
    float dt = GetFrameTime();

    switch (e->type) {
    case ENTITY_PLAYER: {



        if (e->position.y > ground_y) {
            // e->player.speed.y -= 1 * 9.8 * dt;
        } else {
            // e->player.speed.y = 0.0;
        }

        bool is_grounded = false;
        bool is_grounded_with_room = false;

        e->player.interacting_with = 0;
        for each_entity(id) {
            Entity* other = &entities[id];
            switch (other->type) {
            case ENTITY_INTERACT: {
                float dx = e->position.x - other->position.x;
                float dy = e->position.y - other->position.y;
                if (dx * dx + dy * dy < 2) {
                    e->player.interacting_with = id;
                    break;
                }
            } break;
            case ENTITY_PLATFORM: {
                if (e->position.x + PLAYER_SIZE >= other->position.x && e->position.x < other->position.x + other->platform.size.x) {

                    if (e->position.y - other->platform.size.y < other->position.y) {
                        is_grounded = true;
                    }
                    if (e->position.y - other->platform.size.y - 0.08 < other->position.y) {
                        is_grounded_with_room = true;
                    }
                }

            } break;
            default: continue;
            }
        }

        if (is_grounded) {
            e->player.speed.y = 0;
        } else {
            e->player.speed.y -= 2 * 9.8 * dt;
        }

        if (IsKeyDown(KEY_SPACE) && is_grounded_with_room) {
            e->player.speed.y += 1.1; 
        }

        float desired_speed = 0.0f;
        if (IsKeyDown(KEY_RIGHT)) {
            desired_speed = PLAYER_SPEED;
            // e->player.speed.x += dt * PLAYER_SPEED;
        } else if (IsKeyDown(KEY_LEFT)) {
            desired_speed = -PLAYER_SPEED;
            // e->player.speed.x -= dt * PLAYER_SPEED;
        }
        e->player.speed.x = lerpf(e->player.speed.x, desired_speed, dt * 20.0);

        if (IsKeyPressed(KEY_E) && e->player.interacting_with) {
            entity_remove(e->player.interacting_with);
            e->player.interacting_with = 0;
        }

        e->position.x += e->player.speed.x * dt;
        e->position.y += e->player.speed.y * dt;
    } break;
    case ENTITY_DOOR: {

    } break;
    case ENTITY_INTERACT: {
        // float dx = e->position.x - player_position.x;
        // float dy = e->position.y - player_position.y;
        // DrawRectangle(e->position.x, e->position.y, 100, 100, (Color){0, 255, 0, 0});
        // if (dx * dx + dy * dy < 100) {
        //     DrawRectangle(100, 100, 100, 100, (Color){0, 0, 255, 255});
        // }
    } break;
    case ENTITY_PLATFORM: break;
    default:
        oc_assert(false && "invalid entity");
        break;
    }
}

void entity_draw(EntityId id) {
    Entity* e = &entities[id];

    switch (e->type) {
    case ENTITY_PLAYER: {
        GameDrawRectangle(e->position, (Vector2) {PLAYER_SIZE, PLAYER_SIZE}, player_color);

        if (e->player.interacting_with) {
            // DrawText(, 1920/WORLD_TO_SCREEN/2, 0.2, 0.3, (Color){0, 0, 0, 255});
            // Fontj
            DrawTextEx(GetFontDefault(), "Press [E] to pick up", (Vector2){ 1920/WORLD_TO_SCREEN/2.0, 1080/WORLD_TO_SCREEN - 0.4 }, 0.3, 0.02, (Color){0, 0, 0, 255});
            // GameDrawRectangle((Vector2){0.1, 1.0}, (Vector2){1, 1}, (Color){0, 0, 255, 255});
        }
    } break;
    case ENTITY_DOOR: {

    } break;
    case ENTITY_INTERACT: {
        GameDrawCircle((Vector2){e->position.x, e->position.y}, 0.5f, (Color){0, 255, 0, 255});
    } break;
    case ENTITY_PLATFORM: {
        GameDrawRectangle(e->position, e->platform.size, (Color){100, 100, 100, 255});
    } break;
    default:
        oc_assert(false && "invalid entity");
        break;
    }
}

void entity_update_all() {
    for each_entity(id) {
        entity_update(id);
    }
}

void entity_draw_all() {
    for each_entity(id) {
        entity_draw(id);
    }
}

// void entity_update_all() {
//     for (EntityId id = 1; id < next_entity_id; ++id) {
//         if (entities[id].type != ENTITY_INVALID) {
//             entity_update(id);
//         }
//     }
// }
#endif

Oc_Arena arena = { 0 };
Oc_Arena frame_arena = { 0 };

int main(void)
{
    Oc_Arena a = { 0 };
    Array(uint32) arr = { 0 };
    oc_array_append(&a, &arr, 65);

    Hash_Map(uint32, uint32) hm;
    hash_map_put(&a, &hm, 45, 100);
    uint32* value = hash_map_get(&a, &hm, 45);
    print("foo {}\n", *value);

    const int screenWidth = 1920;
    const int screenHeight = 1080;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(screenWidth, screenHeight, "House Demo");

    // Camera camera = { 0 };
    // camera.position = (Vector3){ 10.0f, 10.0f, 10.0f }; // Camera position
    // camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    // camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    // camera.fovy = 45.0f;                                // Camera field-of-view Y
    // camera.projection = CAMERA_ORTHOGRAPHIC;             // Camera mode type

    Camera2D camera = { 0 };
    camera.offset = (Vector2) { 0.0f, 0.0f };
    camera.zoom = 1.0f;
    // camera.zoom = WORLD_TO_SCREEN;

    Model model = LoadModel("resources/house.obj");
    Vector3 position = { 0.0f, 0.0f, 0.0f };            // Set model position

    SetTargetFPS(240);

    Vector2 player_speed = { 0.0f, 0.0f };

    // EntityId entity;

    // entity = entity_new(ENTITY_PLAYER);
    // entities[entity].position.x = 0;
    // entities[entity].position.y = 3;

    // entity = entity_new(ENTITY_INTERACT);
    // entities[entity].position.x = 10.6;
    // entities[entity].position.y = 5;

    // entity = entity_new(ENTITY_PLATFORM);
    // entities[entity].position.x = 0;
    // entities[entity].position.y = 1;
    // entities[entity].platform.size.x = 3;
    // entities[entity].platform.size.y = 0.1;

    // entity = entity_new(ENTITY_PLATFORM);
    // entities[entity].position.x = 4;
    // entities[entity].position.y = 1.7;
    // entities[entity].platform.size.x = 1.2;
    // entities[entity].platform.size.y = 0.1;

    // entity = entity_new(ENTITY_PLATFORM);
    // entities[entity].position.x = 7;
    // entities[entity].position.y = 2.1;
    // entities[entity].platform.size.x = 1.2;
    // entities[entity].platform.size.y = 0.1;


    // entity = entity_new(ENTITY_PLATFORM);
    // entities[entity].position.x = 10;
    // entities[entity].position.y = 3.1;
    // entities[entity].platform.size.x = 1.2;
    // entities[entity].platform.size.y = 0.1;

    Font font = LoadFontEx("resources/Roboto-Light.ttf", 60, NULL, 0);

    Game_Parameters parameters = {
        .neutral_font = font,
        .screen_width = screenWidth,
        .screen_height = screenHeight,
    };
    memory_game_init(parameters);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // entity_update_all();

        Game_Parameters parameters = {
            .neutral_font = font,
            .screen_width = screenWidth,
            .screen_height = screenHeight,
        };
        Oc_Arena_Save save = oc_arena_save(&frame_arena);

        BeginDrawing();
            ClearBackground((Color){40, 40, 40, 255});
            BeginMode2D(camera);

                memory_game_update(parameters);
                // draw_grid();
                // entity_draw_all();
            EndMode2D();

            // DrawText("Test Text", 190, 200, 20, LIGHTGRAY);
        EndDrawing();

        oc_arena_restore(&frame_arena, save);
    }

    UnloadModel(model);

    CloseWindow();

    return 0;
}
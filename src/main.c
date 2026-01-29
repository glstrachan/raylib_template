#include "raylib.h"

#define OC_CORE_IMPLEMENTATION
#include "entity.h"

Entity entities[ENTITIES_MAX_COUNT] = { 0 };
EntityId next_entity_id = 1; // 0 is for null entity reference

void entity_update(EntityId id) {
    Entity* e = &entities[id];

    switch (e->type) {
    case ENTITY_PLAYER: {
        if (e->player.target) {
            Entity* target = &entities[e->player.target];
            e->position = target->position;
        } else {
            e->position.x += 1;
        }
    } break;
    case ENTITY_DOOR: {
        
    } break;
    default:
        oc_assert(false && "invalid entity");
        break;
    }
}

void entity_update_all() {
    for (EntityId id = 1; id < next_entity_id; ++id) {
        if (entities[id].type != ENTITY_INVALID) {
            entity_update(id);
        }
    }
}

int main(void)
{
    // Oc_Arena a = { 0 };
    // Hash_Map(uint32, uint32) hm;
    // hash_map_put(&a, &hm, 45, 100);

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "House Demo");

    Camera camera = { 0 };
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_ORTHOGRAPHIC;             // Camera mode type

    Model model = LoadModel("resources/house.obj");
    Vector3 position = { 0.0f, 0.0f, 0.0f };            // Set model position

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        UpdateCamera(&camera, CAMERA_FIRST_PERSON);

        BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginMode3D(camera);
                DrawModel(model, position, 1.0f, RED);
                DrawGrid(20, 10.0f);
            EndMode3D();

            DrawText("Test Text", 190, 200, 20, LIGHTGRAY);
        EndDrawing();
    }

    UnloadModel(model);

    CloseWindow();

    return 0;
}
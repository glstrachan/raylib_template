#include "raylib.h"

int main(void)
{
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
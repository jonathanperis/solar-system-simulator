#include <raylib.h>

#include "sim/constants.h"
#include "sim/solar_system.h"
#include "sim/units.h"
#include "render/renderer.h"

int main(void)
{
    const int screen_width = 1280;
    const int screen_height = 720;

    InitWindow(screen_width, screen_height, "Solar System Simulator");
    SetTargetFPS(60);

    Camera3D camera = {0};
    camera.position = (Vector3){0.0f, 8.0f, 18.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    SolarSystem system = solar_system_create_sun_mercury();
    const double time_scale = SOLAR_DAY_SECONDS;

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);
        solar_system_step(&system, (double)GetFrameTime() * time_scale);

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
        renderer_draw_solar_system(&system);
        EndMode3D();

        DrawText("Solar System Simulator - Milestone 2: Mercury", 20, 20, 20, RAYWHITE);
        DrawText(TextFormat("Bodies: %zu (Sun + Mercury)", system.body_count), 20, 50, 18, RAYWHITE);
        DrawText(TextFormat("Elapsed days: %.2f", seconds_to_days(system.elapsed_seconds)), 20, 75, 18, RAYWHITE);
        DrawText(TextFormat("Time scale: %.0f simulation seconds / real second", time_scale), 20, 100, 18, RAYWHITE);
        DrawText("Physics: SI Newtonian baseline; rendering scale: 1 AU = 10 units", 20, 125, 18, RAYWHITE);
        DrawText("Mercury starts at perihelion with vis-viva tangential speed", 20, 150, 18, RAYWHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

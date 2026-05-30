#include <raylib.h>

#include "sim/constants.h"
#include "sim/solar_system.h"
#include "sim/units.h"
#include "render/renderer.h"

static size_t next_body_index(size_t current, const SolarSystem *system)
{
    if (system->body_count == 0) {
        return 0;
    }

    return (current + 1) % system->body_count;
}

static Vector3 body_camera_target(const SolarSystem *system, size_t body_index)
{
    if (system->body_count == 0 || body_index >= system->body_count) {
        return (Vector3){0.0f, 0.0f, 0.0f};
    }

    Vec3d render_position = meters_vec_to_render_vec3d(system->bodies[body_index].position_m);
    return (Vector3){
        (float)render_position.x,
        (float)render_position.y,
        (float)render_position.z,
    };
}

static RenderScaleMode next_render_scale_mode(RenderScaleMode mode)
{
    return mode == RENDER_SCALE_ILLUSTRATIVE ? RENDER_SCALE_REAL : RENDER_SCALE_ILLUSTRATIVE;
}

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

    SolarSystem system = solar_system_create_sun_mercury_venus();
    const double time_scale = SOLAR_DAY_SECONDS;
    size_t focused_body_index = 0;
    RenderScaleMode render_mode = RENDER_SCALE_ILLUSTRATIVE;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_C)) {
            focused_body_index = next_body_index(focused_body_index, &system);
        }

        if (IsKeyPressed(KEY_V)) {
            render_mode = next_render_scale_mode(render_mode);
        }

        camera.target = body_camera_target(&system, focused_body_index);
        UpdateCamera(&camera, CAMERA_ORBITAL);
        solar_system_step(&system, (double)GetFrameTime() * time_scale);
        camera.target = body_camera_target(&system, focused_body_index);

        const char *focused_body_name = "None";
        if (system.body_count > 0 && focused_body_index < system.body_count) {
            focused_body_name = system.bodies[focused_body_index].name;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
        renderer_draw_solar_system(&system, render_mode);
        EndMode3D();

        DrawText("Solar System Simulator - Milestone 3: Venus", 20, 20, 20, RAYWHITE);
        DrawText(TextFormat("Bodies: %zu (Sun + Mercury + Venus)", system.body_count), 20, 50, 18, RAYWHITE);
        DrawText(TextFormat("Elapsed days: %.2f", seconds_to_days(system.elapsed_seconds)), 20, 75, 18, RAYWHITE);
        DrawText(TextFormat("Time scale: %.0f simulation seconds / real second", time_scale), 20, 100, 18, RAYWHITE);
        DrawText(TextFormat("View: %s (V to toggle)", renderer_scale_mode_label(render_mode)), 20, 125, 18, RAYWHITE);
        DrawText(TextFormat("Camera focus: %s (Tab/C to cycle)", focused_body_name), 20, 150, 18, RAYWHITE);
        DrawText("Physics: SI Newtonian baseline; rendering scale: 1 AU = 10 units", 20, 175, 18, RAYWHITE);
        DrawText("Illustrative view clamps radii; real-scale view uses physical radius scale", 20, 200, 18, RAYWHITE);
        DrawText("Mercury and Venus start at perihelion with vis-viva tangential speeds", 20, 225, 18, RAYWHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

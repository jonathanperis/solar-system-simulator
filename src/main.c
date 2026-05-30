#include <raylib.h>

#include "app/body_trails.h"
#include "app/orbit_camera.h"
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

static Vector3 orbit_camera_vec3_to_raylib(OrbitCameraVec3 vector)
{
    return (Vector3){vector.x, vector.y, vector.z};
}

static OrbitCameraVec3 raylib_vec3_to_orbit_camera(Vector3 vector)
{
    return (OrbitCameraVec3){vector.x, vector.y, vector.z};
}

static Vector3 body_camera_target(const SolarSystem *system, size_t body_index, RenderScaleMode mode)
{
    Vec3d render_position = renderer_body_position(system, body_index, mode);
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

static void apply_orbit_camera(Camera3D *camera, const OrbitCameraState *state, Vector3 target)
{
    OrbitCameraVec3 orbit_target = raylib_vec3_to_orbit_camera(target);

    camera->target = target;
    camera->position = orbit_camera_vec3_to_raylib(orbit_camera_position(orbit_target, state));
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

    OrbitCameraState orbit_camera = orbit_camera_default_state();
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars();
    BodyTrails trails = body_trails_create();
    body_trails_record_system(&trails, &system);
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

        float frame_time = GetFrameTime();
        Vector3 camera_target = body_camera_target(&system, focused_body_index, render_mode);
        orbit_camera_apply_zoom(&orbit_camera, GetMouseWheelMove());
        orbit_camera_advance(&orbit_camera, frame_time);
        solar_system_step(&system, (double)frame_time * time_scale);
        body_trails_record_system(&trails, &system);
        camera_target = body_camera_target(&system, focused_body_index, render_mode);
        apply_orbit_camera(&camera, &orbit_camera, camera_target);

        const char *focused_body_name = "None";
        if (system.body_count > 0 && focused_body_index < system.body_count) {
            focused_body_name = system.bodies[focused_body_index].name;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
        renderer_draw_solar_system(&system, &trails, render_mode);
        EndMode3D();

        DrawText("Solar System Simulator - Milestone 6: Mars", 20, 20, 20, RAYWHITE);
        DrawText(TextFormat("Bodies: %zu (Sun + Mercury + Venus + Earth + Moon + Mars)", system.body_count), 20, 50, 18, RAYWHITE);
        DrawText(TextFormat("Elapsed days: %.2f", seconds_to_days(system.elapsed_seconds)), 20, 75, 18, RAYWHITE);
        DrawText(TextFormat("Time scale: %.0f simulation seconds / real second", time_scale), 20, 100, 18, RAYWHITE);
        DrawText(TextFormat("View: %s (V to toggle)", renderer_scale_mode_label(render_mode)), 20, 125, 18, RAYWHITE);
        DrawText(TextFormat("Camera focus: %s (Tab/C to cycle)", focused_body_name), 20, 150, 18, RAYWHITE);
        DrawText(TextFormat("Camera zoom: %.1f units (mouse wheel, clamped)", orbit_camera.distance), 20, 175, 18, RAYWHITE);
        DrawText("Camera auto-orbits at fixed pitch; zoom cannot flip the angle", 20, 200, 18, RAYWHITE);
        DrawText("Physics: SI Newtonian baseline; rendering scale: 1 AU = 10 units", 20, 225, 18, RAYWHITE);
        DrawText("Illustrative view keeps planets large and expands Moon offset; real-scale view uses physical scale", 20, 250, 18, RAYWHITE);
        DrawText("Moon starts at Earth-relative perigee with vis-viva speed", 20, 275, 18, RAYWHITE);
        DrawText("Mars starts at heliocentric perihelion with vis-viva speed", 20, 300, 18, RAYWHITE);
        DrawText("Traces: planets and moons leave bounded motion trails", 20, 325, 18, RAYWHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

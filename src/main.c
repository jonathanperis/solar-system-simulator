#include <raylib.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include "app/body_labels.h"
#include "app/body_trails.h"
#include "app/orbit_camera.h"
#include "sim/constants.h"
#include "sim/solar_system.h"
#include "sim/units.h"
#include "render/renderer.h"

#define SOLAR_APP_MAX_PHYSICS_STEP_SECONDS (5.0 * 60.0)

typedef struct SolarApp {
    Camera3D camera;
    OrbitCameraState orbit_camera;
    SolarSystem system;
    BodyTrails trails;
    double time_scale;
    size_t focused_body_index;
    RenderScaleMode render_mode;
    char body_list_label[160];
} SolarApp;

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

static void step_system_with_substeps(SolarSystem *system, double dt_seconds)
{
    /* The renderer may deliver long frames; clamp physics updates into smaller
     * SI-second chunks so close moons do not receive one huge unstable step. */
    while (dt_seconds > SOLAR_APP_MAX_PHYSICS_STEP_SECONDS) {
        solar_system_step(system, SOLAR_APP_MAX_PHYSICS_STEP_SECONDS);
        dt_seconds -= SOLAR_APP_MAX_PHYSICS_STEP_SECONDS;
    }

    if (dt_seconds > 0.0) {
        solar_system_step(system, dt_seconds);
    }
}

static void apply_orbit_camera(Camera3D *camera, const OrbitCameraState *state, Vector3 target)
{
    OrbitCameraVec3 orbit_target = raylib_vec3_to_orbit_camera(target);

    camera->target = target;
    camera->position = orbit_camera_vec3_to_raylib(orbit_camera_position(orbit_target, state));
}

static void solar_app_update_draw(void *user_data)
{
    SolarApp *app = user_data;

    if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_C)) {
        app->focused_body_index = next_body_index(app->focused_body_index, &app->system);
    }

    if (IsKeyPressed(KEY_V)) {
        app->render_mode = next_render_scale_mode(app->render_mode);
    }

    float frame_time = GetFrameTime();
    Vector3 camera_target = body_camera_target(&app->system, app->focused_body_index, app->render_mode);
    orbit_camera_apply_zoom(&app->orbit_camera, GetMouseWheelMove());
    orbit_camera_advance(&app->orbit_camera, frame_time);
    step_system_with_substeps(&app->system, (double)frame_time * app->time_scale);
    body_trails_record_system(&app->trails, &app->system);
    camera_target = body_camera_target(&app->system, app->focused_body_index, app->render_mode);
    apply_orbit_camera(&app->camera, &app->orbit_camera, camera_target);

    const char *focused_body_name = "None";
    if (app->system.body_count > 0 && app->focused_body_index < app->system.body_count) {
        focused_body_name = app->system.bodies[app->focused_body_index].name;
    }

    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode3D(app->camera);
    renderer_draw_solar_system(&app->system, &app->trails, app->render_mode);
    EndMode3D();

    DrawText("Solar System Simulator - Milestone 7: Phobos + Deimos", 20, 20, 20, RAYWHITE);
    DrawText(TextFormat("Bodies: %zu (%s)", app->system.body_count, app->body_list_label), 20, 50, 18, RAYWHITE);
    DrawText(TextFormat("Elapsed days: %.2f", seconds_to_days(app->system.elapsed_seconds)), 20, 75, 18, RAYWHITE);
    DrawText(TextFormat("Time scale: %.0f simulation seconds / real second", app->time_scale), 20, 100, 18, RAYWHITE);
    DrawText(TextFormat("View: %s (V to toggle)", renderer_scale_mode_label(app->render_mode)), 20, 125, 18, RAYWHITE);
    DrawText(TextFormat("Camera focus: %s (Tab/C to cycle)", focused_body_name), 20, 150, 18, RAYWHITE);
    DrawText(TextFormat("Camera zoom: %.1f units (mouse wheel, clamped)", app->orbit_camera.distance), 20, 175, 18, RAYWHITE);
    DrawText("Camera auto-orbits at fixed pitch; zoom cannot flip the angle", 20, 200, 18, RAYWHITE);
    DrawText("Physics: SI Newtonian baseline; rendering scale: 1 AU = 10 units", 20, 225, 18, RAYWHITE);
    DrawText("Illustrative view keeps planets large and separates parent-relative moons", 20, 250, 18, RAYWHITE);
    DrawText("Moon starts at Earth-relative perigee with vis-viva speed", 20, 275, 18, RAYWHITE);
    DrawText("Mars starts at heliocentric perihelion with vis-viva speed", 20, 300, 18, RAYWHITE);
    DrawText("Phobos and Deimos start at Mars-relative periareion with vis-viva speed", 20, 325, 18, RAYWHITE);
    DrawText("Grid expands to cover the farthest rendered orbit", 20, 350, 18, RAYWHITE);
    DrawText("Traces: planets and moons keep their full motion history", 20, 375, 18, RAYWHITE);

    EndDrawing();
}

int main(void)
{
    const int screen_width = 1280;
    const int screen_height = 720;

    InitWindow(screen_width, screen_height, "Solar System Simulator");
    SetTargetFPS(60);

    SolarApp app = {0};
    app.camera.position = (Vector3){0.0f, 8.0f, 18.0f};
    app.camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    app.camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    app.camera.fovy = 45.0f;
    app.camera.projection = CAMERA_PERSPECTIVE;
    app.orbit_camera = orbit_camera_default_state();
    app.system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    app.trails = body_trails_create();
    app.time_scale = SOLAR_DAY_SECONDS;
    app.render_mode = RENDER_SCALE_ILLUSTRATIVE;
    solar_app_body_list_label(&app.system, app.body_list_label, sizeof(app.body_list_label));
    body_trails_record_system(&app.trails, &app.system);

#if defined(PLATFORM_WEB)
    /* Emscripten owns the browser event loop; app state is static so the
     * callback never points at a stack frame that returned. */
    static SolarApp web_app;
    web_app = app;
    emscripten_set_main_loop_arg(solar_app_update_draw, &web_app, 0, 1);
#else
    while (!WindowShouldClose()) {
        solar_app_update_draw(&app);
    }

    body_trails_destroy(&app.trails);
    CloseWindow();
#endif
    return 0;
}

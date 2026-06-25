#include <raylib.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include "app/body_trails.h"
#include "app/orbit_camera.h"
#include "app/simulation_step.h"
#include "sim/constants.h"
#include "sim/solar_system.h"
#include "sim/units.h"
#include "render/renderer.h"

typedef struct SolarApp {
    Camera3D camera;
    OrbitCameraState orbit_camera;
    OrbitCameraVec3 smoothed_camera_target;
    RenderTexture2D scene_texture;
    SolarSystem system;
    BodyTrails trails;
    double time_scale;
    size_t focused_body_index;
    RenderScaleMode render_mode;
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

static void apply_orbit_camera(Camera3D *camera, const OrbitCameraState *state, Vector3 target)
{
    OrbitCameraVec3 orbit_target = raylib_vec3_to_orbit_camera(target);

    camera->target = target;
    camera->position = orbit_camera_vec3_to_raylib(orbit_camera_position(orbit_target, state));
}

static void solar_app_ensure_scene_texture(SolarApp *app, int width, int height)
{
    if (width < 1 || height < 1) {
        return;
    }

    if (app->scene_texture.id != 0 && app->scene_texture.texture.width == width && app->scene_texture.texture.height == height) {
        return;
    }

    if (app->scene_texture.id != 0) {
        UnloadRenderTexture(app->scene_texture);
    }
    app->scene_texture = LoadRenderTexture(width, height);
}

static void draw_scene_to_texture(SolarApp *app, int screen_width, int screen_height)
{
    BeginTextureMode(app->scene_texture);
    renderer_draw_backdrop(screen_width, screen_height, app->system.elapsed_seconds);

    BeginMode3D(app->camera);
    renderer_draw_solar_system(&app->system, &app->trails, app->render_mode);
    EndMode3D();
    EndTextureMode();
}

static void draw_scene_texture(const SolarApp *app, int screen_width, int screen_height)
{
    Rectangle source = {
        0.0f,
        0.0f,
        (float)app->scene_texture.texture.width,
        -(float)app->scene_texture.texture.height,
    };
    Rectangle destination = {0.0f, 0.0f, (float)screen_width, (float)screen_height};

    DrawTexturePro(app->scene_texture.texture, source, destination, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
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
    orbit_camera_apply_zoom(&app->orbit_camera, GetMouseWheelMove());
    orbit_camera_advance(&app->orbit_camera, frame_time);
    solar_app_step_system_with_trails(
        &app->system,
        &app->trails,
        (double)frame_time * app->time_scale,
        SOLAR_APP_MAX_PHYSICS_STEP_SECONDS
    );

    Vector3 desired_camera_target = body_camera_target(&app->system, app->focused_body_index, app->render_mode);
    app->smoothed_camera_target = orbit_camera_smooth_target(
        app->smoothed_camera_target,
        raylib_vec3_to_orbit_camera(desired_camera_target),
        frame_time
    );
    Vector3 camera_target = orbit_camera_vec3_to_raylib(app->smoothed_camera_target);
    apply_orbit_camera(&app->camera, &app->orbit_camera, camera_target);

    const char *focused_body_name = "None";
    if (app->system.body_count > 0 && app->focused_body_index < app->system.body_count) {
        focused_body_name = app->system.bodies[app->focused_body_index].name;
    }

    int screen_width = GetRenderWidth();
    int screen_height = GetRenderHeight();
    if (screen_width <= 0 || screen_height <= 0) {
        screen_width = GetScreenWidth();
        screen_height = GetScreenHeight();
    }
    solar_app_ensure_scene_texture(app, screen_width, screen_height);
    draw_scene_to_texture(app, screen_width, screen_height);

    BeginDrawing();
    ClearBackground((Color){8, 9, 26, 255});
    draw_scene_texture(app, screen_width, screen_height);

    DrawText("Solar System Simulator", 20, 20, 20, RAYWHITE);
    DrawText(TextFormat("Elapsed days: %.2f", seconds_to_days(app->system.elapsed_seconds)), 20, 50, 18, RAYWHITE);
    DrawText(TextFormat("Focus: %s", focused_body_name), 20, 75, 18, RAYWHITE);
    DrawText(TextFormat("View: %s | Zoom: %.1f", renderer_scale_mode_label(app->render_mode), app->orbit_camera.distance), 20, 100, 18, RAYWHITE);
    DrawText("Tab/C: focus | V: scale | Wheel: zoom", 20, 125, 18, RAYWHITE);

    EndDrawing();
}

int main(void)
{
    const int screen_width = 1280;
    const int screen_height = 720;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
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
    app.smoothed_camera_target = raylib_vec3_to_orbit_camera(body_camera_target(&app.system, app.focused_body_index, app.render_mode));
    app.scene_texture = LoadRenderTexture(screen_width, screen_height);
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

    UnloadRenderTexture(app.scene_texture);
    body_trails_destroy(&app.trails);
    CloseWindow();
#endif
    return 0;
}

#include <raylib.h>
#include <rlgl.h>
#include <math.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>

EM_JS(int, solar_web_initial_canvas_width, (void), {
    const wrap = document.querySelector('.canvas-wrap');
    const canvas = Module.canvas;
    const rect = (wrap || canvas).getBoundingClientRect();
    const value = Math.round(rect.width || canvas.clientWidth || canvas.width || 1280);
    return Math.max(1, value);
})

EM_JS(int, solar_web_initial_canvas_height, (void), {
    const wrap = document.querySelector('.canvas-wrap');
    const canvas = Module.canvas;
    const rect = (wrap || canvas).getBoundingClientRect();
    const value = Math.round(rect.height || canvas.clientHeight || canvas.height || 720);
    return Math.max(1, value);
})

EM_JS(void, solar_web_report_state, (const char *body_name, const char *parent_name, const char *view_mode,
    const char *camera_target, int selected, int paused, int speed_preset, int auto_rotate,
    double elapsed_seconds, double trail_interval_seconds, int trails_failed, int has_parent,
    double distance_m, double speed_mps, double mass_kg, double radius_m, double zoom), {
    Module.reportState({body: UTF8ToString(body_name), parent: UTF8ToString(parent_name),
        view: UTF8ToString(view_mode), cameraTarget: UTF8ToString(camera_target), selected,
        paused: !!paused, speedPreset: speed_preset, autoRotate: !!auto_rotate,
        elapsedSeconds: elapsed_seconds, intervalSeconds: trail_interval_seconds,
        trailsFailed: !!trails_failed, hasParent: !!has_parent,
        distanceM: distance_m, speedMps: speed_mps, massKg: mass_kg, radiusM: radius_m, zoom});
})

EM_JS(void, solar_web_add_body, (int index, const char *name), {
    Module.addBody(index, UTF8ToString(name));
})

EM_JS(void, solar_web_initialization_failed, (void), {
    Module.onAbort("WebGL could not initialize. Check browser graphics support.");
})

EM_JS(int, solar_web_canvas_has_focus, (void), {
    return document.activeElement === Module.canvas;
})
#endif

#include "app/body_trails.h"
#include "app/orbit_camera.h"
#include "app/simulation_step.h"
#include "app/simulation_session.h"
#include "sim/constants.h"
#include "sim/solar_system.h"
#include "sim/units.h"
#include "render/renderer.h"

typedef struct SolarApp {
    Camera3D camera;
    OrbitCameraState orbit_camera;
    SimulationSession session;
    RenderScaleMode render_mode;
    bool auto_rotate;
    bool system_framed;
} SolarApp;

/* One C command boundary serves native shortcuts and web buttons. Values match
 * runtimeCommands in docs/src/lib/simulator.ts; physics never runs in JS. */
typedef enum SolarCommand {
    SOLAR_COMMAND_PAUSE, SOLAR_COMMAND_STEP, SOLAR_COMMAND_RESET,
    SOLAR_COMMAND_SPEED, SOLAR_COMMAND_SELECT, SOLAR_COMMAND_VIEW,
    SOLAR_COMMAND_ZOOM, SOLAR_COMMAND_ROTATE, SOLAR_COMMAND_FRAME
} SolarCommand;

static SolarApp app;

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

static size_t camera_target_index(const SolarApp *state)
{
    return state->system_framed
        ? renderer_system_frame(&state->session.system, state->session.selected_body_index, state->render_mode).root_index
        : state->session.selected_body_index;
}

static void frame_selected_system(SolarApp *state)
{
    RenderSystemFrame frame = renderer_system_frame(&state->session.system, state->session.selected_body_index, state->render_mode);
    orbit_camera_frame_sphere(&state->orbit_camera, (float)frame.radius, state->camera.fovy,
        (float)GetScreenWidth() / (float)GetScreenHeight());
    state->system_framed = true;
}

static void solar_app_command(SolarApp *state, SolarCommand command, int value)
{
    SimulationSession *session = &state->session;
    switch (command) {
        case SOLAR_COMMAND_PAUSE: session->paused = !session->paused; break;
        case SOLAR_COMMAND_STEP: simulation_session_single_step(session); break;
        case SOLAR_COMMAND_RESET:
            simulation_session_reset(session);
            if (state->system_framed) frame_selected_system(state);
            break;
        case SOLAR_COMMAND_SPEED: simulation_session_set_speed(session, value); break;
        case SOLAR_COMMAND_SELECT:
            simulation_session_select_body(session, value);
            state->system_framed = false;
            state->orbit_camera = orbit_camera_default_state();
            break;
        case SOLAR_COMMAND_VIEW:
            state->render_mode = next_render_scale_mode(state->render_mode);
            if (state->system_framed) frame_selected_system(state);
            break;
        case SOLAR_COMMAND_ZOOM: orbit_camera_apply_zoom(&state->orbit_camera, (float)value); break;
        case SOLAR_COMMAND_ROTATE: state->auto_rotate = !state->auto_rotate; break;
        case SOLAR_COMMAND_FRAME: frame_selected_system(state); break;
    }
}

#if defined(PLATFORM_WEB)
static void report_web_state(const SolarApp *state)
{
    const SimulationSession *session = &state->session;
    BodyInspection body = simulation_session_inspect(session);
    solar_web_report_state(body.name, body.parent_name, renderer_scale_mode_label(state->render_mode),
        session->system.bodies[camera_target_index(state)].name, (int)session->selected_body_index,
        session->paused, session->speed_preset, state->auto_rotate,
        session->system.elapsed_seconds, session->trails.sample_interval_seconds,
        body_trails_recording_failed(&session->trails), body.has_parent,
        body.distance_m, body.speed_mps, body.mass_kg, body.radius_m, state->orbit_camera.distance);
}

EMSCRIPTEN_KEEPALIVE void solar_web_command(int command, int value)
{
    solar_app_command(&app, (SolarCommand)command, value);
    report_web_state(&app);
}
#endif

static void solar_app_update_draw(void *user_data)
{
    SolarApp *app = user_data;

#if defined(PLATFORM_WEB)
    /* CSS owns the frame; keep raylib's projection/backing size in sync after
     * responsive layout changes instead of stretching the old framebuffer. */
    int width = solar_web_initial_canvas_width();
    int height = solar_web_initial_canvas_height();
    if (width != GetScreenWidth() || height != GetScreenHeight()) {
        SetWindowSize(width, height);
        if (app->system_framed) frame_selected_system(app);
    }
#endif

#if defined(PLATFORM_WEB)
    bool controls_active = solar_web_canvas_has_focus();
#else
    bool controls_active = true;
#endif
    if (controls_active) {
        bool next = IsKeyPressed(KEY_C);
#if !defined(PLATFORM_WEB)
        next = next || IsKeyPressed(KEY_TAB);
#endif
        if (next) solar_app_command(app, SOLAR_COMMAND_SELECT,
            (int)((app->session.selected_body_index + 1) % app->session.system.body_count));
        for (int i = 0; i < 9; ++i) {
            if (IsKeyPressed(KEY_ONE + i)) solar_app_command(app, SOLAR_COMMAND_SELECT, i);
        }
        if (IsKeyPressed(KEY_ZERO)) solar_app_command(app, SOLAR_COMMAND_SELECT, 9);
        if (IsKeyPressed(KEY_SPACE)) solar_app_command(app, SOLAR_COMMAND_PAUSE, 0);
        if (IsKeyPressed(KEY_N)) solar_app_command(app, SOLAR_COMMAND_STEP, 0);
        if (IsKeyPressed(KEY_R)) solar_app_command(app, SOLAR_COMMAND_RESET, 0);
        if (IsKeyPressed(KEY_A)) solar_app_command(app, SOLAR_COMMAND_ROTATE, 0);
        if (IsKeyPressed(KEY_F)) solar_app_command(app, SOLAR_COMMAND_FRAME, 0);
        if (IsKeyPressed(KEY_V)) solar_app_command(app, SOLAR_COMMAND_VIEW, 0);
        if (IsKeyPressed(KEY_LEFT_BRACKET)) solar_app_command(app, SOLAR_COMMAND_SPEED, app->session.speed_preset - 1);
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) solar_app_command(app, SOLAR_COMMAND_SPEED, app->session.speed_preset + 1);
    }

    float frame_time = GetFrameTime();
    orbit_camera_apply_zoom(&app->orbit_camera, GetMouseWheelMove());
    if (app->auto_rotate) orbit_camera_advance(&app->orbit_camera, frame_time);
    simulation_session_update(&app->session, frame_time);
    Vector3 camera_target = body_camera_target(&app->session.system, camera_target_index(app), app->render_mode);
    apply_orbit_camera(&app->camera, &app->orbit_camera, camera_target);

#if defined(PLATFORM_WEB)
    report_web_state(app);
#endif

    BeginDrawing();
    ClearBackground(BLACK);

    /* Real-scale moon systems need a near plane smaller than raylib's default.
     * Clip distances follow the camera only; SI positions and radii stay intact. */
    rlSetClipPlanes(fmax(1e-9, app->orbit_camera.distance * 0.001), fmax(1000.0, app->orbit_camera.distance * 4.0));
    BeginMode3D(app->camera);
    renderer_draw_solar_system(&app->session.system, &app->session.trails, app->render_mode);
    EndMode3D();

#if !defined(PLATFORM_WEB)
    /* Astro presents these readouts outside the web canvas for accessibility
     * and mobile layout. Native builds retain their in-window HUD. */
    DrawText("Solar System Simulator", 20, 20, 20, RAYWHITE);
    BodyInspection body = simulation_session_inspect(&app->session);
    DrawText(TextFormat("%s | %.0f simulated seconds | %.0f sim s/real s", app->session.paused ? "Paused" : "Running",
        app->session.system.elapsed_seconds, simulation_session_time_scale(&app->session)), 20, 50, 18, RAYWHITE);
    DrawText(TextFormat("Selected: %s | Parent: %s | View: %s", body.name, body.parent_name,
        renderer_scale_mode_label(app->render_mode)), 20, 75, 18, RAYWHITE);
    DrawText(TextFormat("Mass: %.6g kg | Physical radius: %.3f km", body.mass_kg, body.radius_m / 1000.0), 20, 100, 18, RAYWHITE);
    DrawText(body.has_parent ? TextFormat("Parent-relative: %.3f km | %.6f km/s", body.distance_m / 1000.0, body.speed_mps / 1000.0)
        : "Parent-relative distance/speed: N/A (no parent)", 20, 125, 18, RAYWHITE);
    DrawText("Space: pause | N: +15 s (paused) | R: reset | [ / ]: speed", 20, 155, 18, RAYWHITE);
    DrawText("1-9 / 0 / Tab / C: select | V: scale | F: frame system | Wheel: zoom", 20, 180, 18, RAYWHITE);
    DrawText(TextFormat("A: camera rotation (%s) | Camera target: %s", app->auto_rotate ? "on" : "off",
        app->session.system.bodies[camera_target_index(app)].name), 20, 205, 18, RAYWHITE);
    if (body_trails_recording_failed(&app->session.trails)) {
        DrawText("Trail recording paused: memory unavailable.", 20, 235, 18, RED);
    }
#endif

    EndDrawing();
}

int main(void)
{
    int screen_width = 1280;
    int screen_height = 720;

#if defined(PLATFORM_WEB)
    screen_width = solar_web_initial_canvas_width();
    screen_height = solar_web_initial_canvas_height();
#endif

#if defined(PLATFORM_WEB)
    InitWindow(screen_width, screen_height, "Live simulator · Solar System Simulator");
    if (!IsWindowReady()) {
        solar_web_initialization_failed();
        return 1;
    }
#else
    InitWindow(screen_width, screen_height, "Solar System Simulator");
#endif
    SetTargetFPS(60);

    app.camera.position = (Vector3){0.0f, 8.0f, 18.0f};
    app.camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    app.camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    app.camera.fovy = 45.0f;
    app.camera.projection = CAMERA_PERSPECTIVE;
    app.orbit_camera = orbit_camera_default_state();
    app.session = simulation_session_create();
    app.auto_rotate = true;
    app.render_mode = RENDER_SCALE_ILLUSTRATIVE;

#if defined(PLATFORM_WEB)
    /* Static app storage is shared by the browser loop and exported commands. */
    for (size_t i = 0; i < app.session.system.body_count; ++i) {
        solar_web_add_body((int)i, app.session.system.bodies[i].name);
    }
    report_web_state(&app);
    emscripten_set_main_loop_arg(solar_app_update_draw, &app, 0, 1);
#else
    while (!WindowShouldClose()) {
        solar_app_update_draw(&app);
    }

    simulation_session_destroy(&app.session);
    CloseWindow();
#endif
    return 0;
}

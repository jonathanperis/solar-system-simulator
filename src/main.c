#include <raylib.h>
#include <rlgl.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
    double distance_m, double speed_mps, double mass_kg, double radius_m, double zoom,
    int mass_quality, int radius_quality, double achieved_time_scale, double pending_seconds, int short_timescale), {
    Module.reportState({body: UTF8ToString(body_name), parent: UTF8ToString(parent_name),
        view: UTF8ToString(view_mode), cameraTarget: UTF8ToString(camera_target), selected,
        paused: !!paused, speedPreset: speed_preset, autoRotate: !!auto_rotate,
        elapsedSeconds: elapsed_seconds, intervalSeconds: trail_interval_seconds,
        trailsFailed: !!trails_failed, hasParent: !!has_parent,
        distanceM: distance_m, speedMps: speed_mps, massKg: mass_kg, radiusM: radius_m, zoom,
        massQuality: mass_quality, radiusQuality: radius_quality,
        achievedTimeScale: achieved_time_scale, pendingSeconds: pending_seconds, shortTimescale: !!short_timescale});
})

EM_JS(void, solar_web_add_body, (int index, const char *name, const char *group), {
    Module.addBody(index, UTF8ToString(name), UTF8ToString(group));
})

EM_JS(void, solar_web_clear_bodies, (int experiment, int count), {
    Module.clearBodies(!!experiment, count);
})

EM_JS(void, solar_web_report_lab, (int lesson, int method, double dt, double ticks, double factor,
    double acceleration, double specific_energy, double energy, double energy_change, int isolated,
    double momentum, double angular_momentum, double magnification, int trail_frame, int vectors,
    double px, double py, double pz, double vx, double vy, double vz, int contact_mode), {
    Module.reportLabState({lesson, method, dt, ticks, factor, acceleration, specificEnergy: specific_energy,
        energy, energyChange: energy_change, isolated: !!isolated, momentum, angularMomentum: angular_momentum,
        magnification, trailFrame: trail_frame, vectors: !!vectors, position: [px, py, pz], velocity: [vx, vy, vz], contactMode: contact_mode});
})

EM_JS(void, solar_web_download_csv, (const char *data, int length), {
    Module.downloadCsv(UTF8ToString(data, length));
})

EM_JS(void, solar_web_begin_forces, (double seconds), {
    Module.forceSample = {time: seconds, rows: []};
})
EM_JS(void, solar_web_force_row, (const char *name, double magnitude, double fraction, double x, double y, double z), {
    Module.forceSample.rows.push({name: UTF8ToString(name), magnitude, fraction, vector: [x, y, z]});
})
EM_JS(void, solar_web_finish_forces, (void), { Module.reportForces(Module.forceSample); })

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
#include "app/csv_export.h"
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
    bool body_framed;
    RenderTrailFrame trail_frame;
    bool vectors;
    char feedback[160];
#if defined(PLATFORM_WEB)
    size_t web_body_count;
#endif
#if !defined(PLATFORM_WEB)
    bool searching;
    char search[64];
    int search_match;
#endif
} SolarApp;

/* One C command boundary serves native shortcuts and web buttons. Values match
 * runtimeCommands in docs/src/lib/simulator.ts; physics never runs in JS. */
typedef enum SolarCommand {
    SOLAR_COMMAND_PAUSE, SOLAR_COMMAND_STEP, SOLAR_COMMAND_RESET,
    SOLAR_COMMAND_SPEED, SOLAR_COMMAND_SELECT, SOLAR_COMMAND_VIEW,
    SOLAR_COMMAND_ZOOM, SOLAR_COMMAND_ROTATE, SOLAR_COMMAND_FRAME, SOLAR_COMMAND_FRAME_BODY,
    SOLAR_COMMAND_TRAILS, SOLAR_COMMAND_VECTORS, SOLAR_COMMAND_BACKGROUND, SOLAR_COMMAND_CONTACT
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
    state->body_framed = false;
}

static void frame_selected_body(SolarApp *state)
{
    RenderSystemFrame frame = renderer_body_frame(&state->session.system, state->session.selected_body_index, state->render_mode);
    orbit_camera_frame_sphere(&state->orbit_camera, (float)frame.radius, state->camera.fovy,
        (float)GetScreenWidth() / (float)GetScreenHeight());
    state->system_framed = false;
    state->body_framed = true;
}

static bool start_app_lesson(SolarApp *state, LessonPreset lesson, double factor, PhysicsIntegrator method, double dt)
{
    if (!simulation_session_start_lesson(&state->session, lesson, factor, method, dt)) return false;
    state->orbit_camera = orbit_camera_default_state();
    state->system_framed = state->body_framed = false;
    if (lesson == LESSON_COLLISION) {
        state->render_mode = RENDER_SCALE_REAL;
        frame_selected_system(state);
    }
    if (lesson == LESSON_ENCOUNTER) frame_selected_system(state);
    if (lesson == LESSON_RESONANCE) {
        state->session.selected_body_index = 0;
        frame_selected_system(state);
    }
    snprintf(state->feedback, sizeof(state->feedback), "Lesson: %s | Initial speed factor %.2f", lesson_name(lesson), factor);
    return true;
}

static bool export_snapshot(const SolarApp *state)
{
#if defined(PLATFORM_WEB)
    FILE *stream = tmpfile();
#else
    FILE *stream = simulation_csv_open_output("solar-snapshot.csv");
#endif
    if (!stream) return false;
    bool ok = simulation_csv_begin(stream, &state->session) && simulation_csv_sample(stream, &state->session);
#if defined(PLATFORM_WEB)
    if (ok && fflush(stream) == 0 && fseek(stream, 0, SEEK_END) == 0) {
        long length = ftell(stream);
        char *data = length > 0 ? malloc((size_t)length + 1) : NULL;
        ok = data && fseek(stream, 0, SEEK_SET) == 0 && fread(data, 1, (size_t)length, stream) == (size_t)length;
        if (ok) {
            data[length] = '\0';
            solar_web_download_csv(data, (int)length);
        }
        free(data);
    } else ok = false;
#endif
    if (fclose(stream) != 0) ok = false;
    return ok;
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
            else if (state->body_framed) frame_selected_body(state);
            break;
        case SOLAR_COMMAND_SPEED: simulation_session_set_speed(session, value); break;
        case SOLAR_COMMAND_SELECT:
            simulation_session_select_body(session, value);
            state->system_framed = false;
            state->body_framed = false;
            state->orbit_camera = orbit_camera_default_state();
            break;
        case SOLAR_COMMAND_VIEW:
            state->render_mode = next_render_scale_mode(state->render_mode);
            if (state->system_framed) frame_selected_system(state);
            else if (state->body_framed) frame_selected_body(state);
            break;
        case SOLAR_COMMAND_ZOOM: orbit_camera_apply_zoom(&state->orbit_camera, (float)value); break;
        case SOLAR_COMMAND_ROTATE: state->auto_rotate = !state->auto_rotate; break;
        case SOLAR_COMMAND_FRAME: frame_selected_system(state); break;
        case SOLAR_COMMAND_FRAME_BODY: frame_selected_body(state); break;
        case SOLAR_COMMAND_TRAILS: state->trail_frame = state->trail_frame == RENDER_TRAILS_ABSOLUTE ? RENDER_TRAILS_PARENT : RENDER_TRAILS_ABSOLUTE; break;
        case SOLAR_COMMAND_VECTORS: state->vectors = !state->vectors; break;
        case SOLAR_COMMAND_BACKGROUND: simulation_session_set_background(session, value != 0); break;
        case SOLAR_COMMAND_CONTACT:
            if (session->lesson == LESSON_COLLISION) {
                CollisionMode mode = session->clock.collision_mode == COLLISION_BOUNCE ? COLLISION_MERGE : COLLISION_BOUNCE;
                simulation_session_start_configured_lesson(session, session->lesson, session->velocity_factor,
                    session->clock.integrator, simulation_clock_step_seconds(&session->clock), mode);
                frame_selected_system(state);
            }
            break;
    }
}

#if defined(PLATFORM_WEB)
static void populate_web_bodies(void)
{
    app.web_body_count = app.session.system.body_count;
    solar_web_clear_bodies(app.session.catalog_experiment, (int)app.session.system.body_count);
    for (size_t i = 0; i < app.session.system.body_count; ++i) {
        const Body *body = &app.session.system.bodies[i];
        const char *group = body->group ? body->group : body->kind == BODY_KIND_MOON ? "Earth and Mars moons"
            : body->kind == BODY_KIND_PLANET ? "Planets" : "Primary bodies";
        solar_web_add_body((int)i, body->name, group);
    }
}

static void report_web_state(const SolarApp *state)
{
    const SimulationSession *session = &state->session;
    BodyInspection body = simulation_session_inspect(session);
    solar_web_report_state(body.name, body.parent_name, renderer_scale_mode_label(state->render_mode),
        session->system.bodies[camera_target_index(state)].name, (int)session->selected_body_index,
        session->paused, session->speed_preset, state->auto_rotate,
        session->system.elapsed_seconds, session->trails.sample_interval_seconds,
        body_trails_recording_failed(&session->trails), body.has_parent,
        body.distance_m, body.speed_mps, body.mass_kg, body.radius_m, state->orbit_camera.distance,
        body.mass_quality, body.radius_quality, session->achieved_time_scale, session->clock.pending_seconds,
        session->lesson == LESSON_COLLISION);
    PhysicsDiagnostics diagnostics = physics_diagnostics(&session->system);
    const Body *selected = &session->system.bodies[session->selected_body_index];
    solar_web_report_lab(session->lesson, session->clock.integrator, simulation_clock_step_seconds(&session->clock),
        (double)session->clock.ticks, session->velocity_factor, body.acceleration_mps2, body.specific_energy_jpkg,
        diagnostics.total_energy_j, simulation_session_energy_change(session, &diagnostics), diagnostics.isolated,
        vec3d_length(diagnostics.momentum_kg_mps), vec3d_length(diagnostics.angular_momentum_kg_m2ps),
        renderer_radius_magnification(selected, state->render_mode), state->trail_frame, state->vectors,
        selected->position_m.x, selected->position_m.y, selected->position_m.z,
        selected->velocity_mps.x, selected->velocity_mps.y, selected->velocity_mps.z, session->clock.collision_mode);
    ForceContribution forces[SOLAR_SYSTEM_BODY_CAPACITY];
    size_t count = physics_force_breakdown(&session->system, session->selected_body_index, forces, SOLAR_SYSTEM_BODY_CAPACITY);
    solar_web_begin_forces(session->system.elapsed_seconds);
    for (size_t i = 0; i < count && i < 6; ++i) {
        ForceContribution f = forces[i];
        solar_web_force_row(session->system.bodies[f.source_index].name, f.magnitude_mps2, f.magnitude_fraction,
            f.acceleration_mps2.x, f.acceleration_mps2.y, f.acceleration_mps2.z);
    }
    solar_web_finish_forces();
}

EMSCRIPTEN_KEEPALIVE int solar_web_lesson(int lesson, double factor, int method, double dt)
{
    if (!start_app_lesson(&app, (LessonPreset)lesson, factor, (PhysicsIntegrator)method, dt)) return 0;
    populate_web_bodies();
    report_web_state(&app);
    return 1;
}

EMSCRIPTEN_KEEPALIVE int solar_web_export(void)
{
    return export_snapshot(&app);
}

EMSCRIPTEN_KEEPALIVE void solar_web_command(int command, int value)
{
    solar_app_command(&app, (SolarCommand)command, value);
    if (app.web_body_count != app.session.system.body_count) populate_web_bodies();
    report_web_state(&app);
}

EMSCRIPTEN_KEEPALIVE int solar_web_experiment(const char *text)
{
    if (!simulation_session_start_experiment(&app.session, text)) return 0;
    app.orbit_camera = orbit_camera_default_state();
    app.system_framed = app.body_framed = false;
    populate_web_bodies();
    frame_selected_body(&app);
    report_web_state(&app);
    return 1;
}

EMSCRIPTEN_KEEPALIVE void solar_web_demo(void)
{
    simulation_session_demo(&app.session);
    app.orbit_camera = orbit_camera_default_state();
    app.system_framed = app.body_framed = false;
    populate_web_bodies();
    report_web_state(&app);
}
#endif

#if !defined(PLATFORM_WEB)
static bool update_body_search(SolarApp *state)
{
    bool opened = !state->searching && IsKeyPressed(KEY_SLASH);
    if (opened) {
        state->searching = true;
        state->search[0] = '\0';
    }
    if (!state->searching) return false;
    bool changed = opened;
    int character;
    while ((character = GetCharPressed()) != 0) {
        size_t length = strlen(state->search);
        if (!opened && character >= 32 && character <= 126 && length + 1 < sizeof(state->search)) {
            state->search[length] = (char)character;
            state->search[length + 1] = '\0';
            changed = true;
        }
    }
    size_t length = strlen(state->search);
    if (IsKeyPressed(KEY_BACKSPACE) && length) {
        state->search[length - 1] = '\0';
        changed = true;
    }
    if (changed) state->search_match = simulation_session_find_body(&state->session, state->search, 0);
    if (IsKeyPressed(KEY_DOWN)) state->search_match = simulation_session_find_body(&state->session,
        state->search, (size_t)(state->search_match + 1));
    if (IsKeyPressed(KEY_ENTER) && state->search_match >= 0) {
        solar_app_command(state, SOLAR_COMMAND_SELECT, state->search_match);
        state->searching = false;
    }
    if (IsKeyPressed(KEY_ESCAPE)) state->searching = false;
    return true;
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
        else if (app->body_framed) frame_selected_body(app);
    }
#endif

#if defined(PLATFORM_WEB)
    bool controls_active = solar_web_canvas_has_focus();
#else
    bool controls_active = !update_body_search(app);
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
        if (IsKeyPressed(KEY_B)) solar_app_command(app, SOLAR_COMMAND_FRAME_BODY, 0);
        if (IsKeyPressed(KEY_V)) solar_app_command(app, SOLAR_COMMAND_VIEW, 0);
        if (IsKeyPressed(KEY_LEFT_BRACKET)) solar_app_command(app, SOLAR_COMMAND_SPEED, app->session.speed_preset - 1);
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) solar_app_command(app, SOLAR_COMMAND_SPEED, app->session.speed_preset + 1);
        if (IsKeyPressed(KEY_T)) solar_app_command(app, SOLAR_COMMAND_TRAILS, 0);
        if (IsKeyPressed(KEY_X)) solar_app_command(app, SOLAR_COMMAND_VECTORS, 0);
        if (IsKeyPressed(KEY_M)) solar_app_command(app, SOLAR_COMMAND_CONTACT, 0);
        if (IsKeyPressed(KEY_E)) snprintf(app->feedback, sizeof(app->feedback), "%s",
            export_snapshot(app) ? "Snapshot saved: solar-snapshot.csv (SI units)" : "Could not write snapshot CSV.");
#if !defined(PLATFORM_WEB)
        if (IsKeyPressed(KEY_L)) {
            LessonPreset next = (LessonPreset)((app->session.lesson + 1) % LESSON_COUNT);
            start_app_lesson(app, next, 1, PHYSICS_VERLET, lesson_default_step(next));
        }
        if (!app->session.catalog_experiment && app->session.lesson != LESSON_CORE) {
            double dt = simulation_clock_step_seconds(&app->session.clock);
            if (IsKeyPressed(KEY_I)) start_app_lesson(app, app->session.lesson, app->session.velocity_factor,
                app->session.clock.integrator == PHYSICS_VERLET ? PHYSICS_EULER : PHYSICS_VERLET, dt);
            if (IsKeyPressed(KEY_D)) start_app_lesson(app, app->session.lesson, app->session.velocity_factor,
                app->session.clock.integrator, app->session.lesson == LESSON_COLLISION ? (dt == .1 ? .2 : .1) : dt == 15 ? 75 : dt == 75 ? 150 : dt == 150 ? 300 : 15);
            if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_MINUS)) start_app_lesson(app, app->session.lesson,
                fmax(0.1, fmin(2, app->session.velocity_factor + (IsKeyPressed(KEY_EQUAL) ? 0.1 : -0.1))), app->session.clock.integrator, dt);
        }
#endif
    }

    float frame_time = GetFrameTime();
#if !defined(PLATFORM_WEB)
    simulation_session_set_background(&app->session, IsWindowMinimized());
#endif
    orbit_camera_apply_zoom(&app->orbit_camera, GetMouseWheelMove());
    if (app->auto_rotate) orbit_camera_advance(&app->orbit_camera, frame_time);
    simulation_session_update(&app->session, frame_time);
#if defined(PLATFORM_WEB)
    if (app->web_body_count != app->session.system.body_count) populate_web_bodies();
#endif
    Vec3d origin = renderer_body_position(&app->session.system, camera_target_index(app), app->render_mode);
    apply_orbit_camera(&app->camera, &app->orbit_camera, (Vector3){0,0,0});

#if defined(PLATFORM_WEB)
    report_web_state(app);
#endif

    BeginDrawing();
    ClearBackground(BLACK);

    /* Real-scale moon systems need a near plane smaller than raylib's default.
     * Clip distances follow the camera only; SI positions and radii stay intact. */
    rlSetClipPlanes(fmax(1e-9, app->orbit_camera.distance * 0.001), fmax(1000.0, app->orbit_camera.distance * 4.0));
    BeginMode3D(app->camera);
    renderer_draw_solar_system(&app->session.system, &app->session.trails, app->render_mode, app->trail_frame, origin);
    if (app->vectors) renderer_draw_vectors(&app->session.system, app->session.selected_body_index, app->render_mode, origin);
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
    const char *mass = body.mass_quality == PHYSICAL_UNKNOWN ? "Unknown (test particle)"
        : TextFormat("%.6g kg%s", body.mass_kg, body.mass_quality == PHYSICAL_ESTIMATED ? " (estimated)" : body.mass_quality == PHYSICAL_PUBLISHED ? " (published)" : "");
    const char *radius = body.radius_quality == PHYSICAL_UNKNOWN ? "Unknown (marker only)"
        : TextFormat("%.3f km%s", body.radius_m / 1000.0, body.radius_quality == PHYSICAL_ESTIMATED ? " (estimated)" : body.radius_quality == PHYSICAL_PUBLISHED ? " (published)" : "");
    DrawText(TextFormat("Mass: %s | Physical radius: %s", mass, radius), 20, 100, 18, RAYWHITE);
    DrawText(body.has_parent ? TextFormat("Parent-relative: %.3f km | %.6f km/s", body.distance_m / 1000.0, body.speed_mps / 1000.0)
        : "Parent-relative distance/speed: N/A (no parent)", 20, 125, 18, RAYWHITE);
    DrawText(TextFormat("Space: pause | N: +%.1f s (paused) | R: reset | [ / ]: speed", simulation_clock_step_seconds(&app->session.clock)), 20, 155, 18, RAYWHITE);
    DrawText("1-9 / 0 / Tab / C: select | V: scale | F: family | B: body | Wheel: zoom", 20, 180, 18, RAYWHITE);
    DrawText(TextFormat("A: camera rotation (%s) | Camera target: %s", app->auto_rotate ? "on" : "off",
        app->session.system.bodies[camera_target_index(app)].name), 20, 205, 18, RAYWHITE);
    DrawText(TextFormat("Achieved: %.2f days/second | Pending: %.3f days",
        app->session.paused ? 0 : app->session.achieved_time_scale / SOLAR_DAY_SECONDS,
        app->session.clock.pending_seconds / SOLAR_DAY_SECONDS), 20, 230, 18, RAYWHITE);
    DrawText(app->searching ? TextFormat("Find: %s | %s | Down: next | Enter: select | Esc: close", app->search,
        app->search_match >= 0 ? app->session.system.bodies[app->search_match].name : "No match")
        : "/: find body by name, designation, or moon group", 20, 280, 18, RAYWHITE);
    if (body_trails_recording_failed(&app->session.trails)) {
        DrawText("Trail recording paused: memory unavailable.", 20, 255, 18, RED);
    }
    PhysicsDiagnostics diagnostics = physics_diagnostics(&app->session.system);
    DrawText(TextFormat("%s | %s | dt %.1f s | tick %llu | Accel %.5g m/s^2", lesson_name(app->session.lesson),
        app->session.clock.integrator == PHYSICS_EULER ? "Euler (teaching)" : "Verlet",
        simulation_clock_step_seconds(&app->session.clock), (unsigned long long)app->session.clock.ticks, body.acceleration_mps2), 20, 310, 16, RAYWHITE);
    const char *magnification = body.radius_quality == PHYSICAL_UNKNOWN ? "Unknown radius (marker only)"
        : TextFormat("%.3gx", renderer_radius_magnification(&app->session.system.bodies[app->session.selected_body_index], app->render_mode));
    DrawText(TextFormat("Energy %.6g J | dE/(K0+|U0|) %.3g | Radius magnification %s", diagnostics.total_energy_j,
        simulation_session_energy_change(&app->session, &diagnostics),
        magnification), 20, 335, 16, RAYWHITE);
    DrawText("L: lesson | I: integrator | D: dt | - / =: initial speed | M: contact model (changes reset)", 20, 360, 16, RAYWHITE);
    DrawText(TextFormat("T: trails (%s) | X: vector directions | E: export SI snapshot",
        app->trail_frame == RENDER_TRAILS_PARENT ? "parent-relative" : "absolute"), 20, 385, 16, RAYWHITE);
    DrawText("Vectors: green velocity / orange acceleration; lengths are illustrative", 20, 410, 16, RAYWHITE);
    DrawText(app->feedback, 20, 435, 16, RAYWHITE);
    ForceContribution forces[3];
    size_t force_count = physics_force_breakdown(&app->session.system, app->session.selected_body_index, forces, 3);
    for (size_t i = 0; i < force_count; ++i) {
        ForceContribution f = forces[i];
        DrawText(TextFormat("Gravity from %s: %.4g m/s^2 (%.2f%% of source magnitudes), [%.3g, %.3g, %.3g]",
            app->session.system.bodies[f.source_index].name, f.magnitude_mps2, f.magnitude_fraction * 100,
            f.acceleration_mps2.x, f.acceleration_mps2.y, f.acceleration_mps2.z), 20, 465 + (int)i * 20, 15, RAYWHITE);
    }
#endif

    EndDrawing();
}

int main(int argc, char **argv)
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
    SetExitKey(KEY_NULL); /* Escape closes native search rather than the window. */
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

#if !defined(PLATFORM_WEB)
    if (argc == 3 && strcmp(argv[1], "--lesson") == 0) {
        LessonPreset lesson = LESSON_COUNT;
        for (int i = 0; i < LESSON_COUNT; ++i) if (!strcmp(argv[2], lesson_name((LessonPreset)i))) lesson = (LessonPreset)i;
        if (!start_app_lesson(&app, lesson, 1, PHYSICS_VERLET, lesson_default_step(lesson))) {
            fprintf(stderr, "Unknown lesson: %s\n", argv[2]);
            simulation_session_destroy(&app.session); CloseWindow(); return 1;
        }
    }
    if (argc == 3 && strcmp(argv[1], "--experiment") == 0) {
        FILE *file = fopen(argv[2], "rb");
        char input[SOLAR_EXPERIMENT_TEXT_BYTES];
        size_t bytes = file ? fread(input,1,sizeof(input)-1,file) : 0;
        bool complete = file && !ferror(file) && fgetc(file) == EOF;
        if (file) fclose(file);
        input[bytes] = '\0';
        if (!complete || !simulation_session_start_experiment(&app.session,input)) {
            fprintf(stderr,"Invalid or unreadable catalog experiment: %s\n",argv[2]);
            simulation_session_destroy(&app.session); CloseWindow(); return 1;
        }
        frame_selected_body(&app);
    }
#else
    (void)argc; (void)argv;
#endif

#if defined(PLATFORM_WEB)
    /* Static app storage is shared by the browser loop and exported commands. */
    populate_web_bodies();
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

#include <raylib.h>
#include <rlgl.h>
#include <errno.h>
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
    double px, double py, double pz, double vx, double vy, double vz, int contact_mode,
    double min_factor, double contact_seconds, int grid, int labels), {
    Module.reportLabState({lesson, method, dt, ticks, factor, acceleration, specificEnergy: specific_energy,
        energy, energyChange: energy_change, isolated: !!isolated, momentum, angularMomentum: angular_momentum,
        magnification, trailFrame: trail_frame, vectors: !!vectors, position: [px, py, pz], velocity: [vx, vy, vz], contactMode: contact_mode,
        minFactor: min_factor, contactSeconds: contact_seconds, grid: !!grid, labels: !!labels});
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
#include "app/input_file.h"
#include "sim/constants.h"
#include "sim/solar_system.h"
#include "render/renderer.h"
#include "sim/experiment.h"

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
    bool grid; /* reference grid visibility (presentation only) */
    bool labels; /* in-canvas body names (presentation only) */
    RenderResources render;
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
    SOLAR_COMMAND_TRAILS, SOLAR_COMMAND_VECTORS, SOLAR_COMMAND_BACKGROUND, SOLAR_COMMAND_CONTACT, SOLAR_COMMAND_GRID, SOLAR_COMMAND_LABELS
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

/* Framing turns the camera to the framed body's sunlit side (about 40 degrees
 * off the Sun line) so a newly selected planet is seen by day, not as a dark
 * night-side disc. Auto-rotation continues from there; physics is untouched. */
static void face_sunlit_side(SolarApp *state, size_t body_index)
{
    const SolarSystem *system = &state->session.system;
    for (size_t i = 0; i < system->body_count; ++i) {
        if (system->bodies[i].kind != BODY_KIND_STAR || i == body_index) continue;
        Vec3d to_sun = vec3d_sub(renderer_body_position(system, i, state->render_mode),
            renderer_body_position(system, body_index, state->render_mode));
        state->orbit_camera.yaw_radians = orbit_camera_sunlit_yaw(to_sun.x, to_sun.z, 40.0f * (float)(acos(-1.0) / 180.0));
        return;
    }
}

/* `face_sun` turns the camera to the sunlit side: true for deliberate framing
 * (F/B, lessons, experiments), false when an existing framing is only re-fitted
 * after a reset, a scale change or a canvas resize, so the user's chosen
 * viewing angle is kept. */
static void frame_selected_system(SolarApp *state, bool face_sun)
{
    RenderSystemFrame frame = renderer_system_frame(&state->session.system, state->session.selected_body_index, state->render_mode);
    if (face_sun) face_sunlit_side(state, frame.root_index);
    orbit_camera_frame_sphere(&state->orbit_camera, (float)frame.radius, state->camera.fovy,
        (float)GetScreenWidth() / (float)GetScreenHeight());
    state->system_framed = true;
    state->body_framed = false;
}

static void frame_selected_body(SolarApp *state, bool face_sun)
{
    RenderSystemFrame frame = renderer_body_frame(&state->session.system, state->session.selected_body_index, state->render_mode);
    if (face_sun) face_sunlit_side(state, frame.root_index);
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
        frame_selected_system(state, true);
    }
    if (lesson == LESSON_ENCOUNTER) frame_selected_system(state, true);
    if (lesson == LESSON_RESONANCE) {
        state->session.selected_body_index = 0;
        frame_selected_system(state, true);
    }
    snprintf(state->feedback, sizeof(state->feedback), "Lesson: %s | Initial speed factor %.2f", lesson_name(lesson), factor);
    return true;
}

#if !defined(PLATFORM_WEB)
/* Snapshots never overwrite earlier ones: E claims the next free
 * solar-snapshot-001.csv ... -999.csv in the working directory. */
#define SOLAR_SNAPSHOT_MAX_FILES 999
#endif

/* Returns 0 on success, otherwise an errno value explaining the failure, so
 * the HUD can say why (permissions, full disk, every numbered name taken). */
static int export_snapshot(const SolarApp *state, char *name, size_t size)
{
#if defined(PLATFORM_WEB)
    /* The browser names the download; MEMFS only stages the bytes. */
    snprintf(name, size, "solar-snapshot.csv");
    FILE *stream = tmpfile();
#else
    FILE *stream = simulation_csv_create_numbered("solar-snapshot", SOLAR_SNAPSHOT_MAX_FILES, name, size);
#endif
    if (!stream) return errno ? errno : EIO;
    errno = 0;
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
    int error = ok ? 0 : errno ? errno : EIO;
#if !defined(PLATFORM_WEB)
    /* This call created the file exclusively, so removing a failed export
     * cannot delete anything the user already had. */
    if (!ok) remove(name);
#endif
    return error;
}

static void solar_app_command(SolarApp *state, SolarCommand command, int value)
{
    SimulationSession *session = &state->session;
    switch (command) {
        case SOLAR_COMMAND_PAUSE: session->paused = !session->paused; break;
        case SOLAR_COMMAND_STEP: simulation_session_single_step(session); break;
        case SOLAR_COMMAND_RESET:
            simulation_session_reset(session);
            if (state->system_framed) frame_selected_system(state, false);
            else if (state->body_framed) frame_selected_body(state, false);
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
            if (state->system_framed) frame_selected_system(state, false);
            else if (state->body_framed) frame_selected_body(state, false);
            break;
        case SOLAR_COMMAND_ZOOM: orbit_camera_apply_zoom(&state->orbit_camera, (float)value); break;
        case SOLAR_COMMAND_ROTATE: state->auto_rotate = !state->auto_rotate; break;
        case SOLAR_COMMAND_FRAME: frame_selected_system(state, true); break;
        case SOLAR_COMMAND_FRAME_BODY: frame_selected_body(state, true); break;
        case SOLAR_COMMAND_TRAILS: state->trail_frame = state->trail_frame == RENDER_TRAILS_ABSOLUTE ? RENDER_TRAILS_PARENT : RENDER_TRAILS_ABSOLUTE; break;
        case SOLAR_COMMAND_VECTORS: state->vectors = !state->vectors; break;
        case SOLAR_COMMAND_GRID: state->grid = !state->grid; break;
        case SOLAR_COMMAND_LABELS: state->labels = !state->labels; break;
        case SOLAR_COMMAND_BACKGROUND: simulation_session_set_background(session, value != 0); break;
        case SOLAR_COMMAND_CONTACT:
            if (session->lesson == LESSON_COLLISION) {
                CollisionMode mode = session->clock.collision_mode == COLLISION_BOUNCE ? COLLISION_MERGE : COLLISION_BOUNCE;
                /* On rejection the session is unchanged, so keep the current framing. */
                if (simulation_session_start_configured_lesson(session, session->lesson, session->velocity_factor,
                    session->clock.integrator, simulation_clock_step_seconds(&session->clock), mode))
                    frame_selected_system(state, true);
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
        selected->velocity_mps.x, selected->velocity_mps.y, selected->velocity_mps.z, session->clock.collision_mode,
        lesson_minimum_velocity_factor(session->lesson),
        (double)session->clock.contact_tick * simulation_clock_step_seconds(&session->clock), state->grid, state->labels);
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

/* Lets the lesson form use C's per-lesson lower speed bound as its minimum. */
EMSCRIPTEN_KEEPALIVE double solar_web_minimum_factor(int lesson)
{
    return lesson_minimum_velocity_factor((LessonPreset)lesson);
}

EMSCRIPTEN_KEEPALIVE int solar_web_export(void)
{
    char name[64];
    return export_snapshot(&app, name, sizeof(name)) == 0;
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
    frame_selected_body(&app, true);
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

/* The browser fetches the same texture files the native build reads from
 * assets/textures/, after the first frame (SPEC A65). JavaScript asks C for the
 * inventory so the two lists cannot drift, copies each file's bytes into WASM
 * memory, and hands them over here; C decodes and uploads them. */
EMSCRIPTEN_KEEPALIVE int solar_web_texture_count(void) { return RENDER_TEXTURE_COUNT; }
EMSCRIPTEN_KEEPALIVE const char *solar_web_texture_file(int slot)
{
    const char *file = render_texture_file((RenderTextureSlot)slot);
    return file ? file : "";
}
EMSCRIPTEN_KEEPALIVE int solar_web_load_texture(int slot, const unsigned char *bytes, int length)
{
    if (!app.render.ready || length <= 0) return 0;
    return renderer_load_texture_memory(&app.render, (RenderTextureSlot)slot, bytes, (size_t)length);
}
#endif

#if !defined(PLATFORM_WEB)
/* D walks a short ladder of lesson steps and wraps around. Contact lessons
 * stay within their 0.25 s limit; any other current step restarts the ladder. */
static double next_lesson_step(LessonPreset lesson, double dt)
{
    static const double orbit_steps[] = {15, 75, 150, 300};
    static const double contact_steps[] = {0.1, 0.2};
    bool contact = lesson == LESSON_COLLISION;
    const double *steps = contact ? contact_steps : orbit_steps;
    size_t count = contact ? sizeof(contact_steps) / sizeof(contact_steps[0]) : sizeof(orbit_steps) / sizeof(orbit_steps[0]);
    /* dt normally holds one of these exact literals, but compare with a
     * relative tolerance anyway: == on doubles breaks as soon as a value is
     * computed (e.g. 0.1 + 0.1) rather than copied from the table. */
    for (size_t i = 0; i < count; ++i)
        if (fabs(dt - steps[i]) <= 1e-9 * steps[i]) return steps[(i + 1) % count];
    return steps[0];
}

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
    SolarApp *state = user_data;

#if defined(PLATFORM_WEB)
    /* CSS owns the frame; keep raylib's projection/backing size in sync after
     * responsive layout changes instead of stretching the old framebuffer. */
    int width = solar_web_initial_canvas_width();
    int height = solar_web_initial_canvas_height();
    if (width != GetScreenWidth() || height != GetScreenHeight()) {
        SetWindowSize(width, height);
        if (state->system_framed) frame_selected_system(state, false);
        else if (state->body_framed) frame_selected_body(state, false);
    }
#endif

#if defined(PLATFORM_WEB)
    bool controls_active = solar_web_canvas_has_focus();
#else
    bool controls_active = !update_body_search(state);
#endif
    if (controls_active) {
        bool cycle_selection = IsKeyPressed(KEY_C);
#if !defined(PLATFORM_WEB)
        cycle_selection = cycle_selection || IsKeyPressed(KEY_TAB);
#endif
        if (cycle_selection) solar_app_command(state, SOLAR_COMMAND_SELECT,
            (int)((state->session.selected_body_index + 1) % state->session.system.body_count));
        for (int i = 0; i < 9; ++i) {
            if (IsKeyPressed(KEY_ONE + i)) solar_app_command(state, SOLAR_COMMAND_SELECT, i);
        }
        if (IsKeyPressed(KEY_ZERO)) solar_app_command(state, SOLAR_COMMAND_SELECT, 9);
        if (IsKeyPressed(KEY_SPACE)) solar_app_command(state, SOLAR_COMMAND_PAUSE, 0);
        if (IsKeyPressed(KEY_N)) solar_app_command(state, SOLAR_COMMAND_STEP, 0);
        if (IsKeyPressed(KEY_R)) solar_app_command(state, SOLAR_COMMAND_RESET, 0);
        if (IsKeyPressed(KEY_A)) solar_app_command(state, SOLAR_COMMAND_ROTATE, 0);
        if (IsKeyPressed(KEY_F)) solar_app_command(state, SOLAR_COMMAND_FRAME, 0);
        if (IsKeyPressed(KEY_B)) solar_app_command(state, SOLAR_COMMAND_FRAME_BODY, 0);
        if (IsKeyPressed(KEY_V)) solar_app_command(state, SOLAR_COMMAND_VIEW, 0);
        if (IsKeyPressed(KEY_LEFT_BRACKET)) solar_app_command(state, SOLAR_COMMAND_SPEED, state->session.speed_preset - 1);
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) solar_app_command(state, SOLAR_COMMAND_SPEED, state->session.speed_preset + 1);
        if (IsKeyPressed(KEY_T)) solar_app_command(state, SOLAR_COMMAND_TRAILS, 0);
        if (IsKeyPressed(KEY_X)) solar_app_command(state, SOLAR_COMMAND_VECTORS, 0);
        if (IsKeyPressed(KEY_G)) solar_app_command(state, SOLAR_COMMAND_GRID, 0);
        if (IsKeyPressed(KEY_H)) solar_app_command(state, SOLAR_COMMAND_LABELS, 0);
        if (IsKeyPressed(KEY_M)) solar_app_command(state, SOLAR_COMMAND_CONTACT, 0);
        if (IsKeyPressed(KEY_E)) {
            char name[64];
            int error = export_snapshot(state, name, sizeof(name));
            if (!error) snprintf(state->feedback, sizeof(state->feedback), "Snapshot saved: %s (SI units)", name);
#if !defined(PLATFORM_WEB)
            else if (error == EEXIST) snprintf(state->feedback, sizeof(state->feedback),
                "Snapshot not saved: solar-snapshot-001..%03d.csv all exist here; move some away.", SOLAR_SNAPSHOT_MAX_FILES);
#endif
            else snprintf(state->feedback, sizeof(state->feedback), "Snapshot not saved: %s", strerror(error));
        }
#if !defined(PLATFORM_WEB)
        if (IsKeyPressed(KEY_L)) {
            LessonPreset next = (LessonPreset)((state->session.lesson + 1) % LESSON_COUNT);
            start_app_lesson(state, next, 1, PHYSICS_VERLET, lesson_default_step(next));
        }
        if (!state->session.catalog_experiment && state->session.lesson != LESSON_CORE) {
            double dt = simulation_clock_step_seconds(&state->session.clock);
            if (IsKeyPressed(KEY_I)) start_app_lesson(state, state->session.lesson, state->session.velocity_factor,
                state->session.clock.integrator == PHYSICS_VERLET ? PHYSICS_EULER : PHYSICS_VERLET, dt);
            if (IsKeyPressed(KEY_D)) start_app_lesson(state, state->session.lesson, state->session.velocity_factor,
                state->session.clock.integrator, next_lesson_step(state->session.lesson, dt));
            if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_MINUS)) start_app_lesson(state, state->session.lesson,
                fmax(lesson_minimum_velocity_factor(state->session.lesson),
                    fmin(2, state->session.velocity_factor + (IsKeyPressed(KEY_EQUAL) ? 0.1 : -0.1))), state->session.clock.integrator, dt);
        }
#endif
    }

    float frame_time = GetFrameTime();
#if !defined(PLATFORM_WEB)
    simulation_session_set_background(&state->session, IsWindowMinimized());
#endif
    orbit_camera_apply_zoom(&state->orbit_camera, GetMouseWheelMove());
    /* A stalled frame (sleep, debugger) is discarded by the simulation clock;
     * the camera skips it too instead of jumping to an arbitrary angle. */
    if (state->auto_rotate && !simulation_frame_is_stall(frame_time)) orbit_camera_advance(&state->orbit_camera, frame_time);
    simulation_session_update(&state->session, frame_time);
#if defined(PLATFORM_WEB)
    if (state->web_body_count != state->session.system.body_count) populate_web_bodies();
#endif
    Vec3d origin = renderer_body_position(&state->session.system, camera_target_index(state), state->render_mode);
    apply_orbit_camera(&state->camera, &state->orbit_camera, (Vector3){0,0,0});

#if defined(PLATFORM_WEB)
    report_web_state(state);
#endif

    BeginDrawing();
    /* Deep blue-black rather than pure black reads as space behind the backdrop. */
    ClearBackground((Color){3, 5, 10, 255});

    /* Real-scale moon systems need a near plane smaller than raylib's default.
     * Clip distances follow the camera only; SI positions and radii stay intact. */
    double far_plane = fmax(1000.0, state->orbit_camera.distance * 4.0);
    rlSetClipPlanes(fmax(1e-9, state->orbit_camera.distance * 0.001), far_plane);
    /* Spin models count TDB days from J2000. Catalog experiments start at their
     * source epoch; the synthetic perihelion scene and lessons start at J2000. */
    double epoch_days = state->session.catalog_experiment ? SOLAR_CATALOG_EPOCH_JD - RENDER_J2000_JD : 0.0;
    RenderView view = {state->camera, epoch_days + state->session.system.elapsed_seconds / SOLAR_DAY_SECONDS,
        (float)far_plane, state->grid};
    BeginMode3D(state->camera);
    renderer_draw_solar_system(&state->session.system, &state->session.trails, state->render_mode, state->trail_frame,
        origin, &state->render, &view);
    if (state->vectors) renderer_draw_vectors(&state->session.system, state->session.selected_body_index, state->render_mode, origin);
    EndMode3D();
    if (state->labels) renderer_draw_labels(&state->session.system, state->render_mode, origin, &state->render,
        state->camera, state->session.selected_body_index);

#if !defined(PLATFORM_WEB)
    /* Astro presents these readouts outside the web canvas for accessibility
     * and mobile layout. Native builds retain their in-window HUD. */
    DrawText("Solar System Simulator", 20, 20, 20, RAYWHITE);
    BodyInspection body = simulation_session_inspect(&state->session);
    DrawText(TextFormat("%s | %.0f simulated seconds | %.0f sim s/real s", state->session.paused ? "Paused" : "Running",
        state->session.system.elapsed_seconds, simulation_session_time_scale(&state->session)), 20, 50, 18, RAYWHITE);
    DrawText(TextFormat("Selected: %s | Parent: %s | View: %s", body.name, body.parent_name,
        renderer_scale_mode_label(state->render_mode)), 20, 75, 18, RAYWHITE);
    const char *mass = body.mass_quality == PHYSICAL_UNKNOWN ? "Unknown (test particle)"
        : TextFormat("%.6g kg%s", body.mass_kg, body.mass_quality == PHYSICAL_ESTIMATED ? " (estimated)" : body.mass_quality == PHYSICAL_PUBLISHED ? " (published)" : "");
    const char *radius = body.radius_quality == PHYSICAL_UNKNOWN ? "Unknown (marker only)"
        : TextFormat("%.3f km%s", body.radius_m / 1000.0, body.radius_quality == PHYSICAL_ESTIMATED ? " (estimated)" : body.radius_quality == PHYSICAL_PUBLISHED ? " (published)" : "");
    DrawText(TextFormat("Mass: %s | Physical radius: %s", mass, radius), 20, 100, 18, RAYWHITE);
    DrawText(body.has_parent ? TextFormat("Parent-relative: %.3f km | %.6f km/s", body.distance_m / 1000.0, body.speed_mps / 1000.0)
        : "Parent-relative distance/speed: N/A (no parent)", 20, 125, 18, RAYWHITE);
    DrawText(TextFormat("Space: pause | N: +%.1f s (paused) | R: reset | [ / ]: speed", simulation_clock_step_seconds(&state->session.clock)), 20, 155, 18, RAYWHITE);
    DrawText("1-9 / 0 / Tab / C: select | V: scale | F: family | B: body | Wheel: zoom", 20, 180, 18, RAYWHITE);
    DrawText(TextFormat("A: camera rotation (%s) | Camera target: %s", state->auto_rotate ? "on" : "off",
        state->session.system.bodies[camera_target_index(state)].name), 20, 205, 18, RAYWHITE);
    DrawText(TextFormat("Achieved: %.2f days/second | Pending: %.3f days",
        state->session.paused ? 0 : state->session.achieved_time_scale / SOLAR_DAY_SECONDS,
        state->session.clock.pending_seconds / SOLAR_DAY_SECONDS), 20, 230, 18, RAYWHITE);
    DrawText(state->searching ? TextFormat("Find: %s | %s | Down: next | Enter: select | Esc: close", state->search,
        state->search_match >= 0 ? state->session.system.bodies[state->search_match].name : "No match")
        : "/: find body by name, designation, or moon group", 20, 280, 18, RAYWHITE);
    if (body_trails_recording_failed(&state->session.trails)) {
        DrawText("Trail recording paused: memory unavailable.", 20, 255, 18, RED);
    }
    PhysicsDiagnostics diagnostics = physics_diagnostics(&state->session.system);
    DrawText(TextFormat("%s | %s | dt %.1f s | tick %llu | Accel %.5g m/s^2", lesson_name(state->session.lesson),
        state->session.clock.integrator == PHYSICS_EULER ? "Euler (teaching)" : "Verlet",
        simulation_clock_step_seconds(&state->session.clock), (unsigned long long)state->session.clock.ticks, body.acceleration_mps2), 20, 310, 16, RAYWHITE);
    const char *magnification = body.radius_quality == PHYSICAL_UNKNOWN ? "Unknown radius (marker only)"
        : TextFormat("%.3gx", renderer_radius_magnification(&state->session.system.bodies[state->session.selected_body_index], state->render_mode));
    DrawText(TextFormat("Energy %.6g J | dE/(K0+|U0|) %.3g | Radius magnification %s", diagnostics.total_energy_j,
        simulation_session_energy_change(&state->session, &diagnostics),
        magnification), 20, 335, 16, RAYWHITE);
    DrawText("L: lesson | I: integrator | D: dt | - / =: initial speed | M: contact model (changes reset)", 20, 360, 16, RAYWHITE);
    DrawText(TextFormat("T: trails (%s) | X: vector directions | G: grid | H: labels | E: export SI snapshot",
        state->trail_frame == RENDER_TRAILS_PARENT ? "parent-relative" : "absolute"), 20, 385, 16, RAYWHITE);
    DrawText("Vectors: green velocity / orange acceleration; lengths are illustrative", 20, 410, 16, RAYWHITE);
    DrawText(state->feedback, 20, 435, 16, RAYWHITE);
    if (state->session.clock.contact_tick) {
        DrawText(TextFormat("Contact sphere crossed at %.0f s (a step's straight-line drift; may be a coarse-step artifact): lesson errors withheld",
            (double)state->session.clock.contact_tick * simulation_clock_step_seconds(&state->session.clock)), 20, 540, 16, RED);
    }
    ForceContribution forces[3];
    size_t force_count = physics_force_breakdown(&state->session.system, state->session.selected_body_index, forces, 3);
    for (size_t i = 0; i < force_count; ++i) {
        ForceContribution f = forces[i];
        DrawText(TextFormat("Gravity from %s: %.4g m/s^2 (%.2f%% of source magnitudes), [%.3g, %.3g, %.3g]",
            state->session.system.bodies[f.source_index].name, f.magnitude_mps2, f.magnitude_fraction * 100,
            f.acceleration_mps2.x, f.acceleration_mps2.y, f.acceleration_mps2.z), 20, 465 + (int)i * 20, 15, RAYWHITE);
    }
#endif

    EndDrawing();
}

#if !defined(PLATFORM_WEB)
/* Native builds read assets/textures/ from SOLAR_TEXTURE_DIR, the working
 * directory (make run) or next to build/ (a launched binary). A missing folder
 * only means flat-colour fallbacks, never a failed start. */
static void load_native_textures(RenderResources *render)
{
    const char *override = getenv("SOLAR_TEXTURE_DIR");
    const char *candidates[3] = {override, "assets/textures", TextFormat("%s../assets/textures", GetApplicationDirectory())};
    for (int i = 0; i < 3; ++i) {
        if (!candidates[i] || !render->ready || !DirectoryExists(candidates[i])) continue;
        int loaded = renderer_load_textures_from_directory(render, candidates[i]);
        TraceLog(LOG_INFO, "Loaded %d of %d textures from %s", loaded, RENDER_TEXTURE_COUNT, candidates[i]);
        if (loaded > 0) return;
    }
}
#endif

int main(int argc, char **argv)
{
    int screen_width = 1280;
    int screen_height = 720;

#if defined(PLATFORM_WEB)
    screen_width = solar_web_initial_canvas_width();
    screen_height = solar_web_initial_canvas_height();
#endif

    /* 4x multisampling smooths sphere edges, trails and grid lines; browsers
     * receive it as the WebGL context's antialias attribute. */
    SetConfigFlags(FLAG_MSAA_4X_HINT);
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
    /* Without the shader the renderer still draws flat coloured spheres. */
    if (!renderer_resources_init(&app.render)) TraceLog(LOG_WARNING, "Cinematic renderer unavailable; using flat colours");
#if !defined(PLATFORM_WEB)
    load_native_textures(&app.render);
#endif

    app.camera.position = (Vector3){0.0f, 8.0f, 18.0f};
    app.camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    app.camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    app.camera.fovy = 45.0f;
    app.camera.projection = CAMERA_PERSPECTIVE;
    app.orbit_camera = orbit_camera_default_state();
    app.session = simulation_session_create();
    app.auto_rotate = true;
    app.grid = true;
    app.labels = true;
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
        /* Same bounded reader as solar-lab: rejects oversized input and
         * embedded NUL bytes that would silently cut the experiment short. */
        char input[SOLAR_EXPERIMENT_TEXT_BYTES];
        if (!solar_read_text_file(argv[2], input, sizeof(input)) || !simulation_session_start_experiment(&app.session, input)) {
            fprintf(stderr,"Invalid or unreadable catalog experiment: %s\n",argv[2]);
            simulation_session_destroy(&app.session); CloseWindow(); return 1;
        }
        frame_selected_body(&app, true);
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
    renderer_resources_unload(&app.render);
    CloseWindow();
#endif
    return 0;
}

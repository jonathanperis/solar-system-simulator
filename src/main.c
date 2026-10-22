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

/* Backing pixels per CSS pixel (A114): sharp on HiDPI screens, capped at 2 so
 * a 3x phone does not shade 9 pixels for every CSS pixel. */
EM_JS(double, solar_web_pixel_ratio, (void), {
    const ratio = typeof devicePixelRatio === 'number' && devicePixelRatio > 0 ? devicePixelRatio : 1;
    return Math.min(2, Math.max(1, ratio));
})

/* Pointer cursor over a body (A118): a data attribute the stylesheet reads,
 * so no inline style is needed under the site's CSP. */
EM_JS(void, solar_web_set_hover, (int on), {
    if (Module.canvas) { if (on) Module.canvas.dataset.hover = ''; else delete Module.canvas.dataset.hover; }
})

/* Page panels floating over the canvas (HUD, inspector, dock, lesson strip),
 * as canvas-local rectangles in backing pixels; off-screen markers slide past
 * them (A119). Panels laid out beside the canvas (phones) do not overlap it. */
EM_JS(int, solar_web_overlay_rects, (float *out, int max), {
    const canvas = Module.canvas;
    if (!canvas) return 0;
    const c = canvas.getBoundingClientRect();
    const sx = canvas.width / Math.max(1, c.width), sy = canvas.height / Math.max(1, c.height);
    let n = 0;
    /* The HUD is a wide block holding a few short lines of text: block only
     * the text itself, line by line. Panels block their whole box. */
    const boxes = [];
    for (const element of document.querySelectorAll('.hud, .dock, .inspector, .lesson-strip')) {
        if (element.hidden || !element.offsetParent) continue;
        if (!element.classList.contains('hud')) { boxes.push(element.getBoundingClientRect()); continue; }
        const walker = document.createTreeWalker(element, NodeFilter.SHOW_TEXT);
        const range = document.createRange();
        for (let node = walker.nextNode(); node; node = walker.nextNode()) {
            if (!node.textContent.trim() || !node.parentElement.offsetParent) continue;
            range.selectNodeContents(node);
            const r = range.getBoundingClientRect();
            if (r.width && r.height) boxes.push(r);
        }
    }
    for (const r of boxes) {
        if (n >= max) break;
        const left = Math.max(r.left, c.left), top = Math.max(r.top, c.top);
        const right = Math.min(r.right, c.right), bottom = Math.min(r.bottom, c.bottom);
        if (right <= left || bottom <= top) continue;
        HEAPF32[(out >> 2) + n * 4] = (left - c.left) * sx;
        HEAPF32[(out >> 2) + n * 4 + 1] = (top - c.top) * sy;
        HEAPF32[(out >> 2) + n * 4 + 2] = (right - left) * sx;
        HEAPF32[(out >> 2) + n * 4 + 3] = (bottom - top) * sy;
        n++;
    }
    return n;
})

EM_JS(int, solar_web_prefers_reduced_motion, (void), {
    return typeof matchMedia === 'function' && matchMedia('(prefers-reduced-motion: reduce)').matches ? 1 : 0;
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
    int mass_quality, int radius_quality, double achieved_time_scale, double pending_seconds, int short_timescale,
    double orbital_period_s, int dated, double run_until_seconds, double earth_distance_m), {
    Module.reportState({body: UTF8ToString(body_name), parent: UTF8ToString(parent_name),
        view: UTF8ToString(view_mode), cameraTarget: UTF8ToString(camera_target), selected,
        paused: !!paused, speedPreset: speed_preset, autoRotate: !!auto_rotate,
        elapsedSeconds: elapsed_seconds, intervalSeconds: trail_interval_seconds,
        trailsFailed: !!trails_failed, hasParent: !!has_parent,
        distanceM: distance_m, speedMps: speed_mps, massKg: mass_kg, radiusM: radius_m, zoom,
        massQuality: mass_quality, radiusQuality: radius_quality,
        achievedTimeScale: achieved_time_scale, pendingSeconds: pending_seconds, shortTimescale: !!short_timescale,
        periodS: orbital_period_s, dated: !!dated,
        untilSeconds: run_until_seconds >= 0 ? run_until_seconds : null,
        earthM: Number.isFinite(earth_distance_m) ? earth_distance_m : null});
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
    double min_factor, double contact_seconds, int grid, int labels, int orbits, int follow, int touring, int belt,
    int constellations, int sky, double sky_fov), {
    Module.reportLabState({lesson, method, dt, ticks, factor, acceleration, specificEnergy: specific_energy,
        energy, energyChange: energy_change, isolated: !!isolated, momentum, angularMomentum: angular_momentum,
        magnification, trailFrame: trail_frame, vectors: !!vectors, position: [px, py, pz], velocity: [vx, vy, vz], contactMode: contact_mode,
        minFactor: min_factor, contactSeconds: contact_seconds, grid: !!grid, labels: !!labels, orbits: !!orbits,
        follow: !!follow, touring: !!touring, belt: !!belt, constellations: !!constellations,
        sky: !!sky, skyFov: sky_fov});
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
#include "render/render_scale.h"
#include "render/renderer.h"
#include "sim/experiment.h"
#include "sim/scene_epoch.h"
#include <time.h>

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
    bool orbits; /* osculating orbit guides (presentation only, A107) */
    bool belt;   /* small-body cloud (presentation only, A121) */
    bool constellations; /* IAU stick figures on the sky (A124) */
    /* View from Earth (A143): the camera sits at Earth's centre in real scale
     * and aims at the selected body (the Sun when Earth is selected); the
     * field of view zooms like a telescope. Leaving restores the scale and
     * rotation the view had. Presentation only. */
    bool sky;
    float sky_fov;
    RenderScaleMode sky_return_mode;
    bool sky_return_rotate;
    /* Camera presentation (A112): the view actually shown (eased toward the
     * framing in orbit_camera during a fly-to), pointer state for drag, pinch
     * and click-to-select, and a pause of auto-rotation after interaction. */
    OrbitCameraFlight flight;
    Vec3d shown_origin;
    OrbitCameraState shown_camera;
    bool reduced_motion;
    bool pointer_down;
    Vector2 pointer_last;
    float pointer_travel;
    float pinch_spread;
    float interaction_hold_seconds;
    float shown_seconds; /* wall time the view has been on screen (startup cuts, A112) */
    RenderScaleMode morph_from; /* scale morph (A125): mode being left */
    float morph_seconds;        /* time into the morph; >= SOLAR_FLIGHT_SECONDS when none */
    float pixel_scale; /* backing pixels per CSS pixel (A114) */
    int hovered;       /* body under a mouse pointer, -1 for none (A118) */
    bool gesture;      /* a two-finger gesture is in progress (never a tap) */
    Vector2 hover_at;  /* where the last hover pick looked */
    unsigned hover_frame;
    /* A120: double-click, phone gestures, follow camera and the tour. */
    double last_click_time;
    Vector2 last_click_at;
    float twist_angle;
    float press_seconds;
    bool long_pressed;
    bool follow;
    float follow_offset;
    bool steered;
    bool touring;
    int tour_step;
    float tour_seconds;
    bool touch_seen;   /* touch devices have no hover */
#if defined(PLATFORM_WEB)
    RenderResolutionGovernor governor; /* steps HiDPI down on slow GPUs (A116) */
#endif
    RenderResources render;
    char feedback[160];
#if defined(PLATFORM_WEB)
    size_t web_body_count;
    unsigned web_report_frame;
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
    SOLAR_COMMAND_TRAILS, SOLAR_COMMAND_VECTORS, SOLAR_COMMAND_BACKGROUND, SOLAR_COMMAND_CONTACT, SOLAR_COMMAND_GRID, SOLAR_COMMAND_LABELS,
    SOLAR_COMMAND_ORBITS, SOLAR_COMMAND_FOLLOW, SOLAR_COMMAND_TOUR, SOLAR_COMMAND_BELT, SOLAR_COMMAND_CONSTELLATIONS,
    SOLAR_COMMAND_RUN_UNTIL, SOLAR_COMMAND_SKY
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

/* Pointer input on the canvas (A112): one finger or the left mouse button
 * turns the view, two fingers pinch-zoom, and a press released without
 * moving selects the body under it. Presentation and selection only. */
#define SOLAR_INTERACTION_HOLD_SECONDS 4.0f
#define SOLAR_CLICK_SLOP_PIXELS 5.0f
static void solar_app_command(SolarApp *state, SolarCommand command, int value);
static void frame_selected_system(SolarApp *state, bool face_sun);
static void step_out(SolarApp *state);
static void sky_leave(SolarApp *state);
static void handle_pointer(SolarApp *state)
{
    int touches = GetTouchPointCount();
    if (touches > 0) state->touch_seen = true;
    /* A two-finger gesture stays a gesture until every finger is up: lifting
     * them one at a time must not end in a tap (review fix). */
    if (touches >= 2) state->gesture = true;
    else if (touches == 0 && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (state->gesture) state->pointer_down = false;
        state->gesture = false;
    }
    if (state->gesture && touches == 1) {
        state->pinch_spread = 0.0f;
        return;
    }
    if (touches >= 2) {
        Vector2 a = GetTouchPosition(0), b = GetTouchPosition(1);
        float spread = hypotf(a.x - b.x, a.y - b.y);
        if (state->pinch_spread > 0.0f && spread > 0.0f) {
            if (state->sky) state->sky_fov = orbit_camera_sky_zoom(state->sky_fov, state->pinch_spread / spread);
            else orbit_camera_pinch(&state->orbit_camera, spread / state->pinch_spread);
        }
        /* Two fingers turning about each other turn the view (A120). */
        float angle = atan2f(b.y - a.y, b.x - a.x);
        if (state->pinch_spread > 0.0f && !state->sky) {
            float turn = angle - state->twist_angle;
            if (turn > PI) turn -= 2 * PI;
            if (turn < -PI) turn += 2 * PI;
            orbit_camera_drag(&state->orbit_camera, -turn * (float)GetScreenHeight() / PI, 0.0f, (float)GetScreenHeight());
        }
        state->twist_angle = angle;
        state->pinch_spread = spread;
        state->steered = true;
        state->touring = false;
        /* A pinch is never a click, and lifting one finger must not jump. */
        state->pointer_down = false;
        state->pointer_travel = 1e9f;
        state->interaction_hold_seconds = SOLAR_INTERACTION_HOLD_SECONDS;
        return;
    }
    state->pinch_spread = 0.0f;
    bool down = touches == 1 || IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    Vector2 at = touches == 1 ? GetTouchPosition(0) : GetMousePosition();
    if (down) {
        state->touring = false; /* any press ends the tour */
        if (state->pointer_down) {
            float dx = at.x - state->pointer_last.x, dy = at.y - state->pointer_last.y;
            state->pointer_travel += (fabsf(dx) + fabsf(dy)) / state->pixel_scale;
            if (state->pointer_travel > SOLAR_CLICK_SLOP_PIXELS && (dx != 0.0f || dy != 0.0f)) {
                /* From Earth the aim stays on the selected body. */
                if (!state->sky) orbit_camera_drag(&state->orbit_camera, dx, dy, (float)GetScreenHeight());
                state->interaction_hold_seconds = SOLAR_INTERACTION_HOLD_SECONDS;
                state->steered = true;
            }
            /* A finger held still on a body for half a second shows its
             * name and numbers, like a mouse hover (A120). */
            state->press_seconds += GetFrameTime();
            if (touches == 1 && !state->long_pressed && state->pointer_travel <= SOLAR_CLICK_SLOP_PIXELS
                && state->press_seconds >= 0.5f) {
                int under = renderer_pick_target(at);
                if (under < 0) under = renderer_pick_body(&state->session.system, state->render_mode, state->shown_origin,
                    state->camera, at);
                if (under >= 0) {
                    state->hovered = under;
                    state->long_pressed = true;
                }
            }
        } else {
            state->pointer_travel = 0.0f;
            state->press_seconds = 0.0f;
            state->long_pressed = false;
            if (touches == 1) state->hovered = -1; /* a new touch clears a long-press tag */
        }
        state->pointer_last = at;
        state->pointer_down = true;
        return;
    }
    /* Hover (A118): a mouse resting on a body names and rings it. Touch
     * screens keep a long-press tag until the next touch. */
    int hovered = state->touch_seen ? state->hovered : -1;
    if (!state->touch_seen && IsCursorOnScreen()) {
        /* Picking projects every body: redo it when the mouse moves, and
         * otherwise every fourth frame (bodies and the camera drift only a
         * pixel or two in that time). */
        Vector2 mouse = GetMousePosition();
        bool moved = mouse.x != state->hover_at.x || mouse.y != state->hover_at.y || (++state->hover_frame & 3) == 0;
        if (moved) {
            hovered = renderer_pick_target(mouse);
            if (hovered < 0) hovered = renderer_pick_body(&state->session.system, state->render_mode, state->shown_origin,
                state->camera, mouse);
            state->hover_at = mouse;
        } else {
            hovered = state->hovered;
        }
    }
    if (hovered != state->hovered) {
        state->hovered = hovered;
#if defined(PLATFORM_WEB)
        solar_web_set_hover(hovered >= 0);
#endif
    }
    if (state->pointer_down && state->pointer_travel <= SOLAR_CLICK_SLOP_PIXELS && !state->long_pressed) {
        /* Labels and off-screen markers first (A119), then the bodies. */
        int picked = renderer_pick_target(state->pointer_last);
        if (picked < 0) picked = renderer_pick_body(&state->session.system, state->render_mode, state->shown_origin,
            state->camera, state->pointer_last);
        double now = GetTime();
        bool double_click = now - state->last_click_time < 0.35
            && hypotf(state->pointer_last.x - state->last_click_at.x, state->pointer_last.y - state->last_click_at.y)
                < 8.0f * state->pixel_scale;
        state->last_click_time = double_click ? -1.0 : now;
        state->last_click_at = state->pointer_last;
        if (double_click) {
            /* Double-click (A120): on a body, show its whole family; on empty
             * space, step back out like Esc. */
            if (picked >= 0) frame_selected_system(state, true);
            else step_out(state);
        } else if (picked >= 0 && (size_t)picked == state->session.selected_body_index && !state->body_framed
            && state->session.system.bodies[picked].radius_quality != PHYSICAL_UNKNOWN) {
            /* Clicking the selection again zooms in to it. */
            solar_app_command(state, SOLAR_COMMAND_FRAME_BODY, 0);
        } else if (picked >= 0 && (size_t)picked != state->session.selected_body_index) {
            solar_app_command(state, SOLAR_COMMAND_SELECT, picked);
            /* Clicking a body means "take me there": fly in to frame it.
             * Unknown-radius markers are only selected; a close-up of a
             * nonphysical marker shows nothing. */
            if (state->session.system.bodies[picked].radius_quality != PHYSICAL_UNKNOWN)
                solar_app_command(state, SOLAR_COMMAND_FRAME_BODY, 0);
        }
    }
    state->pointer_down = false;
}

/* Start easing from the view on screen to whatever framing the caller sets
 * next (A112). With reduced motion the flight has zero length: a cut. */
#define SOLAR_FLIGHT_SECONDS 1.6f
#define SOLAR_FLIGHT_STARTUP_SECONDS 1.0f
static void begin_camera_flight(SolarApp *state)
{
    double from[3] = {state->shown_origin.x, state->shown_origin.y, state->shown_origin.z};
    /* A deep link (?body=, ?lesson=) frames its body while the page starts:
     * open there directly instead of flying in from the default view. */
    bool starting = state->shown_seconds < SOLAR_FLIGHT_STARTUP_SECONDS;
    orbit_camera_flight_begin(&state->flight, from, &state->shown_camera,
        state->reduced_motion || starting ? 0.0f : SOLAR_FLIGHT_SECONDS);
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
    if (face_sun) begin_camera_flight(state);
    RenderSystemFrame frame = renderer_system_frame(&state->session.system, state->session.selected_body_index, state->render_mode);
    if (face_sun) face_sunlit_side(state, frame.root_index);
    orbit_camera_frame_sphere(&state->orbit_camera, (float)frame.radius, state->camera.fovy,
        (float)GetScreenWidth() / (float)GetScreenHeight());
    state->system_framed = true;
    state->body_framed = false;
}

static void frame_selected_body(SolarApp *state, bool face_sun)
{
    if (face_sun) begin_camera_flight(state);
    RenderSystemFrame frame = renderer_body_frame(&state->session.system, state->session.selected_body_index, state->render_mode);
    if (face_sun) face_sunlit_side(state, frame.root_index);
    orbit_camera_frame_sphere(&state->orbit_camera, (float)frame.radius, state->camera.fovy,
        (float)GetScreenWidth() / (float)GetScreenHeight());
    state->system_framed = false;
    state->body_framed = true;
}

/* Hover tooltip (A120): the pointed-at (or long-pressed) body's distance
 * from its parent and orbital period, beside the pointer, without changing
 * the selection. Numbers are C's own inspection of SI state. */
static void format_reading(char *out, size_t size, const BodyInspection *body)
{
    if (!body->has_parent) { snprintf(out, size, "no parent"); return; }
    double km = body->distance_m / 1000.0;
    char distance[48], period[32] = "";
    if (km >= 1e6) snprintf(distance, sizeof(distance), "%.3f AU", body->distance_m / SOLAR_AU_METERS);
    else snprintf(distance, sizeof(distance), "%.0f km", km);
    double days = body->orbital_period_s / SOLAR_DAY_SECONDS;
    if (isfinite(days)) {
        if (days >= 730.5) snprintf(period, sizeof(period), ", %.1f yr", days / 365.25);
        else if (days >= 1) snprintf(period, sizeof(period), ", %.1f d", days);
        else snprintf(period, sizeof(period), ", %.1f h", days * 24);
    }
    snprintf(out, size, "%s from %s%s", distance, body->parent_name, period);
}

static void draw_hover_tooltip(const SolarApp *state)
{
    if (state->hovered < 0 || (size_t)state->hovered >= state->session.system.body_count) return;
    if ((size_t)state->hovered == state->session.selected_body_index) return; /* the inspector shows it */
    BodyInspection body = simulation_session_inspect_body(&state->session, (size_t)state->hovered);
    char line[160];
    format_reading(line, sizeof(line), &body);
    const float s = state->pixel_scale, size = 13.0f * s, spacing = 0.4f * s;
    Font font = state->render.label_font_ready ? state->render.label_font : GetFontDefault();
    Vector2 text = MeasureTextEx(font, line, size, spacing);
    Vector2 at = state->touch_seen ? state->pointer_last : GetMousePosition();
    float x = at.x + 16 * s, y = at.y + 18 * s;
    if (x + text.x + 12 * s > GetScreenWidth()) x = at.x - text.x - 20 * s;
    if (y + text.y + 8 * s > GetScreenHeight()) y = at.y - text.y - 22 * s;
    DrawRectangleRec((Rectangle){x, y, text.x + 12 * s, text.y + 8 * s}, (Color){14, 17, 24, 225});
    DrawRectangleLinesEx((Rectangle){x, y, text.x + 12 * s, text.y + 8 * s}, s, (Color){60, 66, 80, 255});
    DrawTextEx(font, line, (Vector2){x + 6 * s, y + 4 * s}, size, spacing, (Color){228, 222, 206, 255});
}

/* Follow camera and tour (A120), once per frame before the view is built.
 * Follow keeps the camera's yaw at a fixed offset from the selection's
 * direction around its parent; steering resets the offset. The tour flies
 * through the Sun, the planets and the dwarf planets, 7 s each, until any
 * press or key ends it. Presentation only. */
#define SOLAR_TOUR_SECONDS 7.0f
static void update_follow_and_tour(SolarApp *state, float dt)
{
    const SolarSystem *system = &state->session.system;
    size_t selected = state->session.selected_body_index;
    int parent = solar_system_parent_index(system, selected);
    if (state->follow && parent >= 0) {
        Vec3d rel = vec3d_sub(system->bodies[selected].position_m, system->bodies[parent].position_m);
        float angle = (float)atan2(rel.x, rel.z);
        if (state->steered) state->follow_offset = state->orbit_camera.yaw_radians - angle;
        else state->orbit_camera.yaw_radians = fmodf(state->follow_offset + angle + 4.0f * PI, 2.0f * PI);
    }
    state->steered = false;
    if (!state->touring) return;
    state->tour_seconds += dt;
    if (state->tour_seconds < SOLAR_TOUR_SECONDS) return;
    state->tour_seconds = 0.0f;
    /* Next stop: the Sun, then each planet and dwarf planet in catalog order. */
    for (size_t k = 1; k <= system->body_count; ++k) {
        size_t i = (size_t)(state->tour_step + (int)k) % system->body_count;
        BodyKind kind = system->bodies[i].kind;
        if (kind != BODY_KIND_STAR && kind != BODY_KIND_PLANET && kind != BODY_KIND_DWARF_PLANET) continue;
        state->tour_step = (int)i;
        bool touring = state->touring;
        solar_app_command(state, SOLAR_COMMAND_SELECT, (int)i);
        solar_app_command(state, SOLAR_COMMAND_FRAME_BODY, 0);
        state->touring = touring;
        return;
    }
}

/* Escape steps back out (A119): a framed body widens to its family (a moon
 * to its planet's system, a planet to its moons), and from a family view to
 * the whole scene. */
static void step_out(SolarApp *state)
{
    if (state->sky) { sky_leave(state); return; }
    const SolarSystem *system = &state->session.system;
    size_t selected = state->session.selected_body_index;
    int parent = solar_system_parent_index(system, selected);
    bool has_family = parent >= 0 && system->bodies[parent].kind != BODY_KIND_STAR;
    for (size_t i = 0; i < system->body_count && !has_family; ++i)
        has_family = solar_system_parent_index(system, i) == (int)selected;
    if (state->body_framed && has_family) {
        frame_selected_system(state, true);
        return;
    }
    if (selected != 0) solar_app_command(state, SOLAR_COMMAND_SELECT, 0);
    frame_selected_system(state, true);
}

static bool start_app_lesson(SolarApp *state, LessonPreset lesson, double factor, PhysicsIntegrator method, double dt)
{
    /* A new scene or lesson starts from the orbit view (Bugbot). */
    sky_leave(state);
    if (!simulation_session_start_lesson(&state->session, lesson, factor, method, dt)) return false;
    state->orbit_camera = orbit_camera_default_state();
    state->system_framed = state->body_framed = false;
    if (lesson == LESSON_COLLISION) {
        state->render_mode = RENDER_SCALE_REAL;
        frame_selected_system(state, true);
    }
    /* Two-body lessons open on the whole pair: framing the small partner alone
     * hides the body it orbits. */
    if (lesson == LESSON_ENCOUNTER || lesson == LESSON_PLUTO_CHARON || lesson == LESSON_DART)
        frame_selected_system(state, true);
    if (lesson == LESSON_RESONANCE) {
        state->session.selected_body_index = 0;
        frame_selected_system(state, true);
    }
    /* A family scene opens framed on its planet and complete moon system. */
    if (lesson_family_planet(lesson) != BODY_ID_NONE) frame_selected_system(state, true);
    if (lesson_is_scene(lesson))
        snprintf(state->feedback, sizeof(state->feedback), "Scene: %s | %zu bodies", lesson_name(lesson), state->session.system.body_count);
    else
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

static int earth_index(const SolarSystem *system)
{
    for (size_t i = 0; i < system->body_count; ++i)
        if (system->bodies[i].id == BODY_ID_EARTH) return (int)i;
    return -1;
}

/* What the view from Earth aims at: the selection, or with Earth selected
 * the star (any other body in a starless lesson), never index 0 blindly:
 * in the Earth-Moon lesson index 0 is Earth itself (Bugbot). */
static size_t sky_target_index(const SolarSystem *system, int earth, size_t selected)
{
    if (selected != (size_t)earth) return selected;
    for (size_t i = 0; i < system->body_count; ++i)
        if (system->bodies[i].kind == BODY_KIND_STAR) return i;
    for (size_t i = 0; i < system->body_count; ++i)
        if (i != (size_t)earth) return i;
    return selected;
}

static void sky_leave(SolarApp *state)
{
    if (!state->sky) return;
    state->sky = false;
    state->render_mode = state->sky_return_mode;
    state->auto_rotate = state->sky_return_rotate;
    renderer_set_observer(-1);
    /* Framing reads camera.fovy: restore the orbit view's field first, or a
     * telescope field (down to 72 arcseconds) frames the body from far away
     * (Bugbot). */
    state->camera.fovy = 45.0f;
    frame_selected_body(state, false);
}

static void sky_enter(SolarApp *state)
{
    if (state->sky || earth_index(&state->session.system) < 0) return;
    state->sky = true;
    state->sky_fov = ORBIT_CAMERA_SKY_FOV_DEFAULT;
    state->sky_return_mode = state->render_mode;
    state->sky_return_rotate = state->auto_rotate;
    /* Directions from Earth are only true in real scale. */
    state->render_mode = RENDER_SCALE_REAL;
    state->morph_seconds = SOLAR_FLIGHT_SECONDS;
    state->auto_rotate = state->follow = state->touring = false;
    state->flight.active = false;
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
            begin_camera_flight(state);
            simulation_session_select_body(session, value);
            state->system_framed = false;
            state->body_framed = false;
            state->orbit_camera = orbit_camera_default_state();
            state->steered = true;  /* follow: recapture the angle for the new body */
            state->touring = false; /* a choice of body ends the tour (it restores its own) */
            break;
        case SOLAR_COMMAND_SKY:
            if (state->sky) sky_leave(state);
            else sky_enter(state);
            break;
        case SOLAR_COMMAND_VIEW:
            /* The view from Earth shows real scale, so V (or choosing
             * Illustrative) leaves it for illustrative scale: restore what
             * it left, and toggle on if that was real scale too. */
            if (state->sky) {
                sky_leave(state);
                if (state->render_mode != RENDER_SCALE_REAL) break;
            }
            begin_camera_flight(state);
            /* The target body glides with the morph itself: the camera looks
             * straight at it rather than easing toward an eased point. */
            state->flight.track_origin = true;
            /* Morph from the old scale's sizes and places (A125). Pressed
             * again mid-morph, it reverses from where things are: the eased
             * blend is symmetric, so restarting at 1 - t continues smoothly. */
            if (state->morph_seconds < SOLAR_FLIGHT_SECONDS && !state->reduced_motion) {
                state->morph_from = state->render_mode;
                state->morph_seconds = SOLAR_FLIGHT_SECONDS - state->morph_seconds;
            } else {
                state->morph_from = state->render_mode;
                state->morph_seconds = state->reduced_motion ? SOLAR_FLIGHT_SECONDS : 0.0f;
            }
            state->render_mode = next_render_scale_mode(state->render_mode);
            if (state->system_framed) frame_selected_system(state, false);
            else if (state->body_framed) frame_selected_body(state, false);
            break;
        case SOLAR_COMMAND_ZOOM:
            if (state->sky) state->sky_fov = orbit_camera_sky_zoom(state->sky_fov, value > 0 ? 0.7f : 1.0f / 0.7f);
            else orbit_camera_apply_zoom(&state->orbit_camera, (float)value);
            break;
        case SOLAR_COMMAND_ROTATE: state->auto_rotate = !state->auto_rotate; break;
        case SOLAR_COMMAND_FRAME: sky_leave(state); frame_selected_system(state, true); break;
        /* From Earth the view already follows the selection. */
        case SOLAR_COMMAND_FRAME_BODY: if (!state->sky) frame_selected_body(state, true); break;
        case SOLAR_COMMAND_TRAILS: state->trail_frame = state->trail_frame == RENDER_TRAILS_ABSOLUTE ? RENDER_TRAILS_PARENT : RENDER_TRAILS_ABSOLUTE; break;
        case SOLAR_COMMAND_VECTORS: state->vectors = !state->vectors; break;
        case SOLAR_COMMAND_GRID: state->grid = !state->grid; break;
        case SOLAR_COMMAND_LABELS: state->labels = !state->labels; break;
        case SOLAR_COMMAND_ORBITS: state->orbits = !state->orbits; break;
        case SOLAR_COMMAND_FOLLOW: {
            /* Follow (A120): keep the camera's angle to the selection's orbit
             * fixed, so a moon holds its place beside its planet on screen. */
            state->follow = !state->follow;
            state->steered = true; /* capture the current angle as the offset */
            break;
        }
        case SOLAR_COMMAND_BELT: state->belt = !state->belt; break;
        case SOLAR_COMMAND_CONSTELLATIONS: state->constellations = !state->constellations; break;
        case SOLAR_COMMAND_TOUR:
            state->touring = !state->touring;
            state->tour_step = -1;
            state->tour_seconds = 1e9f; /* start at once */
            break;
        case SOLAR_COMMAND_BACKGROUND: simulation_session_set_background(session, value != 0); break;
        case SOLAR_COMMAND_RUN_UNTIL:
            /* "Today" (A133): play forward to `value` seconds after the scene
             * epoch. Only dated scenes have a calendar to run to; a target
             * behind the clock restarts the scene first, since physics only
             * runs forward. */
            if (!simulation_session_is_dated(session)) break;
            if (value < session->system.elapsed_seconds) solar_app_command(state, SOLAR_COMMAND_RESET, 0);
            simulation_session_run_until(session, value);
            break;
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

/* The animation loop reports state to the page on every fourth frame. */
#define SOLAR_WEB_REPORT_FRAMES 4

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
        session->lesson == LESSON_COLLISION, body.orbital_period_s,
        simulation_session_is_dated(session), session->run_until_seconds, body.earth_distance_m);
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
        (double)session->clock.contact_tick * simulation_clock_step_seconds(&session->clock), state->grid, state->labels,
        state->orbits, state->follow, state->touring, state->belt, state->constellations, state->sky, state->sky_fov);
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
    sky_leave(&app);
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
    sky_leave(&app);
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
/* The small-body sample (A121), fetched by the page after the textures. */
EMSCRIPTEN_KEEPALIVE int solar_web_load_belt(const unsigned char *bytes, int length)
{
    if (!app.render.ready || length <= 0) return 0;
    return renderer_load_belt_memory(&app.render, bytes, (size_t)length);
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
     * responsive layout changes instead of stretching the old framebuffer.
     * The backing store has pixel_scale pixels per CSS pixel; the canvas keeps
     * its CSS size (Emscripten clears any inline size when it resizes). */
    /* The governor lowers the scale if this device cannot keep up (A116). */
    float governed = render_resolution_governor_update(&state->governor, GetFrameTime());
    float ratio = fminf((float)solar_web_pixel_ratio(), governed);
    int width = (int)lroundf((float)solar_web_initial_canvas_width() * ratio);
    int height = (int)lroundf((float)solar_web_initial_canvas_height() * ratio);
    if (ratio != state->pixel_scale) {
        state->pixel_scale = ratio;
        renderer_set_pixel_scale(ratio);
    }
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
        if (IsKeyPressed(KEY_O)) solar_app_command(state, SOLAR_COMMAND_ORBITS, 0);
        if (IsKeyPressed(KEY_W)) solar_app_command(state, SOLAR_COMMAND_SKY, 0);
        /* P runs the sky to the present (A133); Emscripten's time() is the
         * browser's clock, so the page's Today button and P agree. */
        if (IsKeyPressed(KEY_P)) solar_app_command(state, SOLAR_COMMAND_RUN_UNTIL,
            (int)scene_epoch_seconds_at_unix_time((double)time(NULL)));
        if (IsKeyPressed(KEY_ESCAPE)) step_out(state);
        if (IsKeyPressed(KEY_Y)) solar_app_command(state, SOLAR_COMMAND_FOLLOW, 0);
        if (IsKeyPressed(KEY_Q)) solar_app_command(state, SOLAR_COMMAND_BELT, 0);
        if (IsKeyPressed(KEY_S)) solar_app_command(state, SOLAR_COMMAND_CONSTELLATIONS, 0);
        /* Arrow keys hop through the family tree (A120): siblings left and
         * right, parent up, first moon down; each hop flies to the body. */
        const int arrows[4] = {KEY_RIGHT, KEY_LEFT, KEY_UP, KEY_DOWN};
        const RenderNavigation hops[4] = {RENDER_NAV_NEXT, RENDER_NAV_PREVIOUS, RENDER_NAV_UP, RENDER_NAV_DOWN};
        for (int k = 0; k < 4; ++k) {
            if (!IsKeyPressed(arrows[k])) continue;
            int next = renderer_navigation_neighbor(&state->session.system, state->session.selected_body_index, hops[k]);
            if (next < 0) continue;
            solar_app_command(state, SOLAR_COMMAND_SELECT, next);
            if (hops[k] == RENDER_NAV_UP) frame_selected_system(state, true);
            else solar_app_command(state, SOLAR_COMMAND_FRAME_BODY, 0);
        }
        if (state->touring && GetKeyPressed() != 0) state->touring = false;
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
            /* L walks the guided lessons; K walks the astronomy scenes. */
            LessonPreset next = state->session.lesson;
            do next = (LessonPreset)((next + 1) % LESSON_COUNT); while (lesson_family_planet(next) != BODY_ID_NONE);
            start_app_lesson(state, next, 1, PHYSICS_VERLET, lesson_default_step(next));
        }
        if (IsKeyPressed(KEY_K)) {
            LessonPreset next = state->session.lesson;
            do next = (LessonPreset)((next + 1) % LESSON_COUNT); while (!lesson_is_scene(next));
            start_app_lesson(state, next, 1, PHYSICS_VERLET, lesson_default_step(next));
        }
        if (!state->session.catalog_experiment && !lesson_is_scene(state->session.lesson)) {
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
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) state->interaction_hold_seconds = SOLAR_INTERACTION_HOLD_SECONDS;
    if (state->sky) state->sky_fov = orbit_camera_sky_zoom(state->sky_fov, expf(-0.15f * wheel));
    else orbit_camera_apply_zoom(&state->orbit_camera, wheel);
    handle_pointer(state);
    bool stalled = simulation_frame_is_stall(frame_time);
    if (state->interaction_hold_seconds > 0.0f && !stalled) state->interaction_hold_seconds -= frame_time;
    /* A stalled frame (sleep, debugger) is discarded by the simulation clock;
     * the camera skips it too instead of jumping to an arbitrary angle. Auto-
     * rotation also waits while someone is steering the view. */
    if (state->auto_rotate && !stalled && state->interaction_hold_seconds <= 0.0f && !state->follow)
        orbit_camera_advance(&state->orbit_camera, frame_time);
    update_follow_and_tour(state, stalled ? 0.0f : frame_time);
    simulation_session_update(&state->session, frame_time);
#if defined(PLATFORM_WEB)
    if (state->web_body_count != state->session.system.body_count) populate_web_bodies();
#endif
    /* The framing lives in orbit_camera and the target body; a fly-to eases
     * the shown view toward it (A112). The target keeps moving during the
     * flight, so the blend reads its current position every frame. */
    Vec3d origin = renderer_body_position(&state->session.system, camera_target_index(state), state->render_mode);
    /* Scale morph (A125), timed and eased like the camera flight, set before
     * any position is read this frame. */
    if (state->morph_seconds < SOLAR_FLIGHT_SECONDS) {
        state->morph_seconds += stalled ? 0.0f : frame_time;
        float t = fminf(state->morph_seconds / SOLAR_FLIGHT_SECONDS, 1.0f);
        renderer_set_scale_morph(state->morph_from, t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f));
        origin = renderer_body_position(&state->session.system, camera_target_index(state), state->render_mode);
    } else {
        renderer_set_scale_morph(state->render_mode, 1.0f);
    }
    OrbitCameraState shown = state->orbit_camera;
    if (state->flight.active) {
        float progress = orbit_camera_flight_advance(&state->flight, stalled ? 0.0f : frame_time);
        double to[3] = {origin.x, origin.y, origin.z}, at[3];
        orbit_camera_flight_blend(&state->flight, progress, to, &state->orbit_camera, at, &shown);
        origin = (Vec3d){at[0], at[1], at[2]};
    }
    int earth = state->sky ? earth_index(&state->session.system) : -1;
    if (state->sky && earth < 0) sky_leave(state); /* a scene without Earth */
    if (state->sky) {
        /* View from Earth (A143): the origin is Earth's centre, so the camera
         * sits at (0,0,0) and looks along the true direction to the target;
         * ecliptic north (+y) is up. Real scale, so every direction is the
         * SI geometry. */
        size_t target = sky_target_index(&state->session.system, earth, state->session.selected_body_index);
        origin = renderer_body_position(&state->session.system, (size_t)earth, state->render_mode);
        Vec3d aim = vec3d_sub(renderer_body_position(&state->session.system, target, state->render_mode), origin);
        renderer_set_observer(earth);
        state->camera.position = (Vector3){0, 0, 0};
        state->camera.target = (Vector3){(float)aim.x, (float)aim.y, (float)aim.z};
        state->camera.up = (Vector3){0, 1, 0};
        state->camera.fovy = state->sky_fov;
        state->shown_origin = origin;
        state->shown_camera = shown;
    } else {
        state->camera.fovy = 45.0f;
        state->shown_origin = origin;
        state->shown_camera = shown;
        apply_orbit_camera(&state->camera, &shown, (Vector3){0,0,0});
    }
    if (!stalled && state->shown_seconds < SOLAR_FLIGHT_STARTUP_SECONDS) state->shown_seconds += frame_time;

#if defined(PLATFORM_WEB)
    /* Readouts for people: 15 updates a second read as live, and each report
     * crosses into JavaScript with strings and objects (garbage every frame)
     * and recomputes the Data sheet's diagnostics and force breakdown. Web
     * commands still report at once, so controls answer immediately. */
    if (state->web_report_frame++ % SOLAR_WEB_REPORT_FRAMES == 0) report_web_state(state);
    if (state->web_report_frame % 15 == 1) {
        float rects[RENDER_MAX_BLOCKERS * 4];
        renderer_set_blockers(rects, solar_web_overlay_rects(rects, RENDER_MAX_BLOCKERS));
    }
#endif

    BeginDrawing();
    /* Deep blue-black rather than pure black reads as space behind the backdrop. */
    ClearBackground((Color){3, 5, 10, 255});

    /* Real-scale moon systems need a near plane smaller than raylib's default.
     * Clip distances follow the camera only; SI positions and radii stay intact. */
    double far_plane = fmax(1000.0, state->shown_camera.distance * 4.0);
    double near_plane = fmax(1e-9, state->shown_camera.distance * 0.001);
    if (state->sky) {
        /* From Earth's centre: nothing nearer than its own surface is drawn,
         * and Neptune lies within about 31 AU. */
        near_plane = meters_to_render_units(2.0 * SOLAR_EARTH_RADIUS_M);
        far_plane = meters_to_render_units(200.0 * SOLAR_AU_METERS);
    }
    rlSetClipPlanes(near_plane, far_plane);
    /* Spin models count TDB days from J2000. Scenes and catalog experiments
     * start at the scene epoch (2026-06-09); synthetic lessons at J2000. */
    bool dated = simulation_session_is_dated(&state->session);
    double epoch_days = dated ? SOLAR_SCENE_EPOCH_JD - RENDER_J2000_JD : 0.0;
    RenderView view = {.camera = state->camera,
        .orientation_days = epoch_days + state->session.system.elapsed_seconds / SOLAR_DAY_SECONDS,
        .far_plane = (float)far_plane, .show_grid = state->grid && !state->sky, .show_orbits = state->orbits,
        .selected = state->session.selected_body_index,
        /* The cloud is the real sky's small bodies: dated scenes only. */
        .show_belt = state->belt && dated,
        .days_since_epoch = state->session.system.elapsed_seconds / SOLAR_DAY_SECONDS,
        .show_constellations = state->constellations};
    BeginMode3D(state->camera);
    renderer_draw_solar_system(&state->session.system, &state->session.trails, state->render_mode, state->trail_frame,
        origin, &state->render, &view);
    if (state->vectors) renderer_draw_vectors(&state->session.system, state->session.selected_body_index, state->render_mode, origin);
    EndMode3D();
    renderer_draw_markers(&state->session.system, state->render_mode, origin, &state->render, state->camera,
        state->session.selected_body_index, state->hovered);
    draw_hover_tooltip(state);
    if (state->sky) {
        /* The field of view, above the scale bar (A143). */
        char field[32], caption[64];
        orbit_camera_sky_fov_label(state->sky_fov, field, sizeof(field));
        snprintf(caption, sizeof(caption), "From Earth | field %s", field);
        const float s = state->pixel_scale;
        Font font = state->render.label_font_ready ? state->render.label_font : GetFontDefault();
        DrawTextEx(font, caption, (Vector2){16 * s, (float)GetScreenHeight() - 58 * s}, 13 * s, 0.4f * s, (Color){240, 190, 90, 255});
    }
    if (state->labels || state->hovered >= 0) renderer_draw_labels(&state->session.system, state->render_mode, origin,
        &state->render, state->camera, state->session.selected_body_index, state->hovered, state->labels);

#if !defined(PLATFORM_WEB)
    /* Astro presents these readouts outside the web canvas for accessibility
     * and mobile layout. Native builds retain their in-window HUD. */
    DrawText("Solar System Simulator", 20, 20, 20, RAYWHITE);
    BodyInspection body = simulation_session_inspect(&state->session);
    char date[32] = "";
    if (simulation_session_is_dated(&state->session))
        scene_epoch_format_date(state->session.system.elapsed_seconds, date, sizeof(date));
    DrawText(TextFormat("%s | %s%s%.0f simulated seconds | %.0f sim s/real s", state->session.paused ? "Paused" : "Running",
        date, date[0] ? " | " : "", state->session.system.elapsed_seconds, simulation_session_time_scale(&state->session)),
        20, 50, 18, RAYWHITE);
    DrawText(TextFormat("Selected: %s | Parent: %s | View: %s", body.name, body.parent_name,
        renderer_scale_mode_label(state->render_mode)), 20, 75, 18, RAYWHITE);
    const char *mass = body.mass_quality == PHYSICAL_UNKNOWN ? "Unknown (test particle)"
        : TextFormat("%.6g kg%s", body.mass_kg, body.mass_quality == PHYSICAL_ESTIMATED ? " (estimated)" : body.mass_quality == PHYSICAL_PUBLISHED ? " (published)" : "");
    const char *radius = body.radius_quality == PHYSICAL_UNKNOWN ? "Unknown (marker only)"
        : TextFormat("%.3f km%s", body.radius_m / 1000.0, body.radius_quality == PHYSICAL_ESTIMATED ? " (estimated)" : body.radius_quality == PHYSICAL_PUBLISHED ? " (published)" : "");
    DrawText(TextFormat("Mass: %s | Physical radius: %s", mass, radius), 20, 100, 18, RAYWHITE);
    DrawText(body.has_parent ? (isfinite(body.orbital_period_s)
            ? TextFormat("Parent-relative: %.3f km | %.6f km/s | period %.4f days (two-body)", body.distance_m / 1000.0,
                body.speed_mps / 1000.0, body.orbital_period_s / SOLAR_DAY_SECONDS)
            : TextFormat("Parent-relative: %.3f km | %.6f km/s | unbound", body.distance_m / 1000.0, body.speed_mps / 1000.0))
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
    DrawText("K: scene | L: lesson | I: integrator | D: dt | - / =: initial speed | M: contact model (changes reset)", 20, 360, 16, RAYWHITE);
    DrawText(TextFormat("T: trails (%s) | O: orbit guides | X: vectors | G: grid | H: labels | P: run to present | E: export SI",
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
        if (loaded > 0) break;
    }
    /* The small-body sample (A121) sits beside the textures. */
    const char *belts[2] = {"assets/belt/small_bodies.bin", TextFormat("%s../assets/belt/small_bodies.bin", GetApplicationDirectory())};
    for (int i = 0; i < 2; ++i) {
        if (!render->ready || !FileExists(belts[i])) continue;
        int size = 0;
        unsigned char *bytes = LoadFileData(belts[i], &size);
        bool loaded = bytes && size > 0 && renderer_load_belt_memory(render, bytes, (size_t)size);
        UnloadFileData(bytes);
        TraceLog(LOG_INFO, "Small-body cloud %s from %s", loaded ? "loaded" : "unavailable", belts[i]);
        if (loaded) return;
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
#if !defined(PLATFORM_WEB)
    SetTargetFPS(60);
#endif
    /* The browser paces frames with requestAnimationFrame (the main loop
     * below passes fps 0). A raylib target there would make EndDrawing wait
     * out its own 1/60 s clock, and Emscripten's nanosleep spins on the main
     * thread: every frame burned CPU until that clock expired, and the two
     * clocks' drift made frames that fit the budget miss vsync. */
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
    /* Open on the inner planets out to Mars, with room around them (A120). */
    orbit_camera_frame_sphere(&app.orbit_camera, meters_to_render_units(1.9 * SOLAR_AU_METERS), app.camera.fovy,
        (float)GetScreenWidth() / (float)GetScreenHeight());
    app.shown_camera = app.orbit_camera;
    app.session = simulation_session_create();
    app.auto_rotate = true;
    app.pixel_scale = 1.0f;
    app.hovered = -1;
    app.morph_seconds = SOLAR_FLIGHT_SECONDS; /* no morph at start */
#if defined(PLATFORM_WEB)
    render_resolution_governor_init(&app.governor, 2.0f);
#endif
#if defined(PLATFORM_WEB)
    /* prefers-reduced-motion (DESIGN.md): no auto-rotation, and framing cuts
     * instead of flying. The page unticks its Rotate box to match. */
    app.reduced_motion = solar_web_prefers_reduced_motion() != 0;
    if (app.reduced_motion) app.auto_rotate = false;
#endif
    app.grid = true;
    app.labels = true;
    app.orbits = true;
    app.belt = true;
    app.constellations = false; /* off by default: the sky stays quiet until asked */
    /* Moon trails relative to their planet trace the same ellipse as the
     * orbit guides; absolute (heliocentric) spirals stay one key away (T). */
    app.trail_frame = RENDER_TRAILS_PARENT;
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

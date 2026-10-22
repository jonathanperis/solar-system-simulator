#include "require_assert.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "app/orbit_camera.h"

static void assert_close_float(float actual, float expected, float epsilon)
{
    assert(fabsf(actual - expected) <= epsilon);
}

static void test_default_orbit_camera_matches_initial_view_angle(void)
{
    OrbitCameraState state = orbit_camera_default_state();
    OrbitCameraVec3 target = {0.0f, 0.0f, 0.0f};
    OrbitCameraVec3 position = orbit_camera_position(target, &state);

    assert_close_float(position.x, 0.0f, 1e-5f);
    assert_close_float(position.y, 8.0f, 1e-5f);
    assert_close_float(position.z, 18.0f, 1e-5f);
}

static void test_orbit_camera_position_offsets_from_focused_target(void)
{
    OrbitCameraState state = orbit_camera_default_state();
    OrbitCameraVec3 target = {2.5f, -1.0f, 4.0f};
    OrbitCameraVec3 position = orbit_camera_position(target, &state);

    assert_close_float(position.x, 2.5f, 1e-5f);
    assert_close_float(position.y, 7.0f, 1e-5f);
    assert_close_float(position.z, 22.0f, 1e-5f);
}

static void test_orbit_camera_advances_yaw_without_changing_pitch_or_distance(void)
{
    OrbitCameraState state = orbit_camera_default_state();
    float pitch = state.pitch_radians;
    float distance = state.distance;

    orbit_camera_advance(&state, 2.0f);

    assert(state.yaw_radians > 0.0f);
    assert_close_float(state.pitch_radians, pitch, 1e-6f);
    assert_close_float(state.distance, distance, 1e-6f);
}

static void test_orbit_camera_zoom_and_auto_orbit_preserve_pitch(void)
{
    OrbitCameraState state = orbit_camera_default_state();
    float pitch = state.pitch_radians;

    orbit_camera_apply_zoom(&state, 1000.0f);
    orbit_camera_advance(&state, 2.0f);
    orbit_camera_apply_zoom(&state, -3.0f);

    assert(state.distance > state.min_distance);
    assert_close_float(state.pitch_radians, pitch, 1e-6f);
}

static void test_orbit_camera_zoom_clamps_at_minimum_without_changing_angle(void)
{
    OrbitCameraState state = orbit_camera_default_state();
    float yaw = state.yaw_radians;
    float pitch = state.pitch_radians;

    orbit_camera_apply_zoom(&state, 1000.0f);

    assert_close_float(state.distance, state.min_distance, 1e-6f);
    assert_close_float(state.yaw_radians, yaw, 1e-6f);
    assert_close_float(state.pitch_radians, pitch, 1e-6f);
}

static void test_orbit_camera_zoom_out_after_minimum_keeps_same_angle(void)
{
    OrbitCameraState state = orbit_camera_default_state();
    float yaw = state.yaw_radians;
    float pitch = state.pitch_radians;

    orbit_camera_apply_zoom(&state, 1000.0f);
    orbit_camera_apply_zoom(&state, -3.0f);

    assert(state.distance > state.min_distance);
    assert_close_float(state.yaw_radians, yaw, 1e-6f);
    assert_close_float(state.pitch_radians, pitch, 1e-6f);
}

static void test_orbit_camera_zoom_clamps_at_maximum_without_changing_angle(void)
{
    OrbitCameraState state = orbit_camera_default_state();
    float yaw = state.yaw_radians;
    float pitch = state.pitch_radians;

    orbit_camera_apply_zoom(&state, -1000.0f);

    assert_close_float(state.distance, state.max_distance, 1e-6f);
    assert_close_float(state.yaw_radians, yaw, 1e-6f);
    assert_close_float(state.pitch_radians, pitch, 1e-6f);
}

static void test_frame_fits_bounding_sphere_at_solar_and_moon_scales(void)
{
    const float radii[] = {25.0f, 0.2f, 0.0002f};
    const float aspects[] = {16.0f / 9.0f, 4.0f / 3.0f, 0.5f};
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            OrbitCameraState state = orbit_camera_default_state();
            float pitch = state.pitch_radians;
            orbit_camera_frame_sphere(&state, radii[i], 45.0f, aspects[j]);
            float angle = asinf(radii[i] / state.distance);
            float vertical = 45.0f * (float)acos(-1.0) / 360.0f;
            float horizontal = atanf(tanf(vertical) * aspects[j]);
            assert(angle < vertical && angle < horizontal);
            assert(state.min_distance > radii[i]);
            assert(state.max_distance >= state.distance);
            assert(state.pitch_radians == pitch);
            float framed_distance = state.distance;
            orbit_camera_apply_zoom(&state, 1.0f);
            assert(state.distance < framed_distance && state.distance > state.min_distance);
        }
    }
}

static void test_auto_rotation_yaw_stays_bounded_and_keeps_moving(void)
{
    /* Eight days of 60 fps auto-rotation. An unbounded float yaw reaches
     * ~1.7e5 radians, where one frame's 0.004-radian step is below half a
     * float ULP: rotation first stutters, then freezes. */
    OrbitCameraState state = orbit_camera_default_state();
    const float dt = 1.0f / 60.0f;
    const float two_pi = 2.0f * acosf(-1.0f);
    for (long frame = 0; frame < 8L * 86400L * 60L; ++frame) orbit_camera_advance(&state, dt);
    assert(state.yaw_radians >= 0.0f && state.yaw_radians < two_pi);
    float before = state.yaw_radians;
    orbit_camera_advance(&state, dt);
    float step = state.yaw_radians - before;
    if (step < 0.0f) step += two_pi; /* The step may cross the wrap point. */
    assert_close_float(step, state.auto_orbit_speed_radians_per_second * dt, 1e-5f);
    /* Reverse motion wraps into the same interval. */
    orbit_camera_advance(&state, -100.0f);
    assert(state.yaw_radians >= 0.0f && state.yaw_radians < two_pi);
}

static void test_sunlit_yaw_places_camera_on_the_day_side(void)
{
    /* Camera offset is (sin yaw, cos yaw) in x/z. With the Sun along +X from
     * the body, a zero offset puts the camera on that same line (full phase);
     * a 40-degree offset keeps it on the day side with a visible terminator. */
    const float pi = 3.14159265f;
    float yaw = orbit_camera_sunlit_yaw(1.0, 0.0, 0.0f);
    assert(fabsf(sinf(yaw) - 1.0f) < 1e-5f && fabsf(cosf(yaw)) < 1e-5f);
    yaw = orbit_camera_sunlit_yaw(0.0, -3.0, 40.0f * pi / 180.0f);
    float camera_x = sinf(yaw), camera_z = cosf(yaw);
    float lit = camera_x * 0.0f + camera_z * -1.0f; /* cosine to the sunward direction */
    assert(fabsf(lit - cosf(40.0f * pi / 180.0f)) < 1e-5f);
    assert(yaw >= 0.0f && yaw < 2.0f * pi);
    /* A degenerate (Sun straight above or below) direction keeps a valid yaw. */
    yaw = orbit_camera_sunlit_yaw(0.0, 0.0, 0.5f);
    assert(yaw >= 0.0f && yaw < 2.0f * pi);
}

/* Direct manipulation (SPEC A112): dragging turns the view, pinching zooms. */
static void test_drag_turns_the_view_and_pitch_stays_short_of_the_poles(void)
{
    OrbitCameraState state = orbit_camera_default_state();
    float yaw = state.yaw_radians, pitch = state.pitch_radians, distance = state.distance;
    orbit_camera_drag(&state, 100.0f, 0.0f, 800.0f);
    assert(state.yaw_radians != yaw && state.pitch_radians == pitch && state.distance == distance);
    /* A drag right then the same drag left returns to the start. */
    orbit_camera_drag(&state, -100.0f, 0.0f, 800.0f);
    assert_close_float(state.yaw_radians, yaw, 1e-5f);
    /* Dragging down raises the camera; it never flips over a pole. */
    for (int i = 0; i < 50; ++i) orbit_camera_drag(&state, 0.0f, 400.0f, 800.0f);
    assert(state.pitch_radians <= ORBIT_CAMERA_MAX_PITCH_RADIANS && state.pitch_radians > 1.0f);
    for (int i = 0; i < 100; ++i) orbit_camera_drag(&state, 0.0f, -400.0f, 800.0f);
    assert(state.pitch_radians >= -ORBIT_CAMERA_MAX_PITCH_RADIANS && state.pitch_radians < -1.0f);
    assert(state.yaw_radians >= 0.0f && state.yaw_radians < 6.2832f);
}

static void test_pinch_zooms_by_ratio_within_limits(void)
{
    OrbitCameraState state = orbit_camera_default_state();
    float distance = state.distance;
    orbit_camera_pinch(&state, 2.0f); /* fingers twice as far apart: twice as close */
    assert_close_float(state.distance, distance / 2.0f, 1e-4f);
    orbit_camera_pinch(&state, 1000.0f);
    assert(state.distance == state.min_distance);
    orbit_camera_pinch(&state, 1e-6f);
    assert(state.distance == state.max_distance);
    orbit_camera_pinch(&state, 0.0f); /* degenerate input changes nothing */
    assert(state.distance == state.max_distance);
}

/* Fly-to (SPEC A112): the camera eases from where it was to the new framing. */
static void test_flight_eases_between_framings(void)
{
    OrbitCameraState from = orbit_camera_default_state(), to = from;
    from.yaw_radians = 6.0f;
    to.yaw_radians = 0.3f; /* shortest way is forward through 2 pi */
    to.distance = from.distance / 1000.0f;
    to.pitch_radians = 0.1f;
    double origin_a[3] = {0, 0, 0}, origin_b[3] = {300, 0, 0};
    OrbitCameraFlight flight;
    orbit_camera_flight_begin(&flight, origin_a, &from, 1.5f);
    assert(flight.active);
    double origin[3];
    OrbitCameraState view;
    /* Start: exactly the old view. */
    orbit_camera_flight_blend(&flight, 0.0f, origin_b, &to, origin, &view);
    assert(origin[0] == 0 && view.distance == from.distance && view.yaw_radians == from.yaw_radians);
    /* Midway: the yaw went the short way and the camera pulled back for the
     * long hop, farther than either end. */
    orbit_camera_flight_blend(&flight, 0.5f, origin_b, &to, origin, &view);
    assert(origin[0] > 100 && origin[0] < 200);
    assert(view.yaw_radians > 6.0f || view.yaw_radians < 0.3f);
    assert(view.distance > 1.5f * sqrtf(from.distance * to.distance)); /* above the plain log path */
    {
        /* Between two framings at the same distance, a long hop pulls back
         * beyond both; reframing in place does not move the distance. */
        OrbitCameraState same = from;
        same.yaw_radians = from.yaw_radians;
        OrbitCameraState pulled;
        double here[3];
        orbit_camera_flight_blend(&flight, 0.5f, origin_b, &same, here, &pulled);
        assert(pulled.distance > 2.0f * from.distance);
        orbit_camera_flight_blend(&flight, 0.5f, origin_a, &same, here, &pulled);
        assert_close_float(pulled.distance, from.distance, 1e-3f);
    }
    /* End: exactly the destination. */
    orbit_camera_flight_blend(&flight, 1.0f, origin_b, &to, origin, &view);
    assert(origin[0] == 300 && fabsf(view.distance - to.distance) < 1e-6f * to.distance);
    assert_close_float(view.yaw_radians, to.yaw_radians, 1e-5f);
    assert_close_float(view.pitch_radians, to.pitch_radians, 1e-6f);
    /* Progress eases in and out and ends the flight. */
    float p0 = orbit_camera_flight_advance(&flight, 0.15f);
    float p1 = orbit_camera_flight_advance(&flight, 0.6f);
    assert(p0 > 0 && p0 < 0.1f && p1 > 0.4f && p1 < 0.6f && flight.active);
    assert(orbit_camera_flight_advance(&flight, 10.0f) == 1.0f && !flight.active);
    /* A flight that tracks its origin (a scale change, where the target body
     * itself glides) looks straight at the destination origin at every step,
     * so the eased body is not eased a second time (review fix). */
    orbit_camera_flight_begin(&flight, origin_a, &from, 1.5f);
    flight.track_origin = true;
    orbit_camera_flight_blend(&flight, 0.3f, origin_b, &to, origin, &view);
    assert(origin[0] == 300 && origin[1] == 0 && origin[2] == 0);
    assert(view.distance != to.distance); /* distance and angles still ease */
    /* A zero-length flight (reduced motion) lands at once. */
    orbit_camera_flight_begin(&flight, origin_a, &from, 0.0f);
    assert(orbit_camera_flight_advance(&flight, 0.016f) == 1.0f && !flight.active);
}

/* View from Earth (A143): the telescope's field of view zooms by a factor
 * and stays between a wide naked-eye field and a narrow telescope field. */
static void test_sky_zoom_narrows_and_widens_within_bounds(void)
{
    float fov = 30.0f;
    fov = orbit_camera_sky_zoom(fov, 0.5f);
    assert(fabsf(fov - 15.0f) < 1e-5f);
    assert(orbit_camera_sky_zoom(fov, 2.0f) == 30.0f);
    assert(orbit_camera_sky_zoom(0.03f, 0.01f) == ORBIT_CAMERA_SKY_FOV_MIN);
    assert(orbit_camera_sky_zoom(60.0f, 100.0f) == ORBIT_CAMERA_SKY_FOV_MAX);
    /* A broken factor never poisons the view. */
    assert(orbit_camera_sky_zoom(12.0f, NAN) == 12.0f);
    assert(orbit_camera_sky_zoom(12.0f, -1.0f) == 12.0f);
    /* Narrow enough to show Jupiter's disc (about 47 arcseconds) as a disc. */
    assert(ORBIT_CAMERA_SKY_FOV_MIN < 47.0f / 3600.0f * 10.0f);
    /* The caption names the field in the unit an observer would use. */
    char label[32];
    orbit_camera_sky_fov_label(40.0f, label, sizeof(label));
    assert(strcmp(label, "40 deg") == 0);
    orbit_camera_sky_fov_label(2.5f, label, sizeof(label));
    assert(strcmp(label, "2.5 deg") == 0);
    orbit_camera_sky_fov_label(0.25f, label, sizeof(label));
    assert(strcmp(label, "15 arcmin") == 0);
    orbit_camera_sky_fov_label(0.02f, label, sizeof(label));
    assert(strcmp(label, "72 arcsec") == 0);
}

/* Strict C11 headers need not define M_PI. */
#define SKY_PI 3.14159265358979323846

/* Looking around from Earth (A144): the aim is the target direction turned
 * by yaw about ecliptic north (+y) and by pitch toward it. */
static void test_sky_aim_turns_about_north_and_stops_short_of_the_poles(void)
{
    const double east[3] = {2.0, 0.0, 0.0};
    double out[3];
    orbit_camera_sky_aim(east, 0.0f, 0.0f, out);
    assert(fabs(out[0] - 2.0) < 1e-9 && fabs(out[1]) < 1e-9 && fabs(out[2]) < 1e-9);
    /* A quarter turn about +y keeps the length and the elevation. */
    orbit_camera_sky_aim(east, (float)(SKY_PI / 2), 0.0f, out);
    assert(fabs(sqrt(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]) - 2.0) < 1e-6);
    assert(fabs(out[0]) < 1e-6 && fabs(out[1]) < 1e-9 && fabs(fabs(out[2]) - 2.0) < 1e-6);
    /* Pitch raises the aim toward north, but never onto the pole (where
     * "up" would be undefined). */
    orbit_camera_sky_aim(east, 0.0f, (float)(SKY_PI / 6), out);
    assert(fabs(asin(out[1] / 2.0) - SKY_PI / 6) < 1e-6);
    orbit_camera_sky_aim(east, 0.0f, 10.0f, out);
    assert(asin(out[1] / 2.0) < SKY_PI / 2 - 0.01 && asin(out[1] / 2.0) > SKY_PI / 2 - 0.05);
    /* Stored pitch is clamped where the aim stops, so dragging back from a
     * pole answers at once (Bugbot): the limit is measured from the target's
     * own elevation. */
    const double raised[3] = {1.0, 1.0, 0.0}; /* 45 degrees up */
    float clamped = orbit_camera_sky_clamp_pitch(raised, 5.0f);
    assert(fabsf(clamped - (float)(SKY_PI / 4 - SKY_PI / 180)) < 1e-5f);
    assert(fabsf(orbit_camera_sky_clamp_pitch(raised, -5.0f) - (float)(-3 * SKY_PI / 4 + SKY_PI / 180)) < 1e-5f);
    assert(orbit_camera_sky_clamp_pitch(raised, 0.2f) == 0.2f);
    /* One viewport height of drag sweeps the whole vertical field. */
    float swept = orbit_camera_sky_drag_radians(900.0f, 40.0f, 900.0f);
    assert(fabsf(swept - 40.0f * (float)SKY_PI / 180.0f) < 1e-6f);
    assert(orbit_camera_sky_drag_radians(10.0f, 40.0f, 0.0f) == 0.0f);
}

int main(void)
{
    test_sky_aim_turns_about_north_and_stops_short_of_the_poles();
    test_sky_zoom_narrows_and_widens_within_bounds();
    test_sunlit_yaw_places_camera_on_the_day_side();
    test_auto_rotation_yaw_stays_bounded_and_keeps_moving();
    test_frame_fits_bounding_sphere_at_solar_and_moon_scales();
    test_default_orbit_camera_matches_initial_view_angle();
    test_orbit_camera_position_offsets_from_focused_target();
    test_orbit_camera_advances_yaw_without_changing_pitch_or_distance();
    test_orbit_camera_zoom_and_auto_orbit_preserve_pitch();
    test_orbit_camera_zoom_clamps_at_minimum_without_changing_angle();
    test_orbit_camera_zoom_out_after_minimum_keeps_same_angle();
    test_orbit_camera_zoom_clamps_at_maximum_without_changing_angle();
    test_drag_turns_the_view_and_pitch_stays_short_of_the_poles();
    test_pinch_zooms_by_ratio_within_limits();
    test_flight_eases_between_framings();
    puts("test_orbit_camera passed");
    return 0;
}

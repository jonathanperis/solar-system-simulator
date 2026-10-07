#include "require_assert.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

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

int main(void)
{
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
    puts("test_orbit_camera passed");
    return 0;
}

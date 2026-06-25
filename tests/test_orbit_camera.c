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

int main(void)
{
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

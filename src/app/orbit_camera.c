#include "orbit_camera.h"

#include <math.h>

static float clamp_float(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

OrbitCameraState orbit_camera_default_state(void)
{
    const float initial_height = 6.0f;
    const float initial_depth = 14.0f;

    return (OrbitCameraState){
        .yaw_radians = 0.0f,
        .pitch_radians = atan2f(initial_height, initial_depth),
        .distance = sqrtf(initial_height * initial_height + initial_depth * initial_depth),
        .min_distance = 4.0f,
        .max_distance = 80.0f,
        .zoom_speed = 2.0f,
        .auto_orbit_speed_radians_per_second = 0.25f,
    };
}

OrbitCameraVec3 orbit_camera_position(OrbitCameraVec3 target, const OrbitCameraState *state)
{
    float cos_pitch = cosf(state->pitch_radians);

    return (OrbitCameraVec3){
        .x = target.x + state->distance * sinf(state->yaw_radians) * cos_pitch,
        .y = target.y + state->distance * sinf(state->pitch_radians),
        .z = target.z + state->distance * cosf(state->yaw_radians) * cos_pitch,
    };
}

OrbitCameraVec3 orbit_camera_smooth_target(OrbitCameraVec3 current, OrbitCameraVec3 desired, float dt_seconds)
{
    if (dt_seconds >= 1.0f) {
        return desired;
    }

    float alpha = 1.0f - expf(-6.5f * dt_seconds);
    alpha = clamp_float(alpha, 0.0f, 1.0f);
    return (OrbitCameraVec3){
        .x = current.x + (desired.x - current.x) * alpha,
        .y = current.y + (desired.y - current.y) * alpha,
        .z = current.z + (desired.z - current.z) * alpha,
    };
}

void orbit_camera_apply_zoom(OrbitCameraState *state, float wheel_move)
{
    state->distance = clamp_float(
        state->distance - wheel_move * state->zoom_speed,
        state->min_distance,
        state->max_distance
    );
}

void orbit_camera_advance(OrbitCameraState *state, float dt_seconds)
{
    state->yaw_radians += state->auto_orbit_speed_radians_per_second * dt_seconds;
}

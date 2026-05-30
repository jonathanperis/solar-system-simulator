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
    const float initial_height = 8.0f;
    const float initial_depth = 18.0f;

    return (OrbitCameraState){
        .yaw_radians = 0.0f,
        .pitch_radians = atan2f(initial_height, initial_depth),
        .distance = sqrtf(initial_height * initial_height + initial_depth * initial_depth),
        .min_distance = 4.0f,
        .max_distance = 80.0f,
        .zoom_speed = 2.0f,
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

void orbit_camera_apply_zoom(OrbitCameraState *state, float wheel_move)
{
    state->distance = clamp_float(
        state->distance - wheel_move * state->zoom_speed,
        state->min_distance,
        state->max_distance
    );
}

#include "orbit_camera.h"

#include <math.h>
#include <stdio.h>

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

void orbit_camera_apply_zoom(OrbitCameraState *state, float wheel_move)
{
    state->distance = clamp_float(
        state->distance - wheel_move * state->zoom_speed,
        state->min_distance,
        state->max_distance
    );
}

float orbit_camera_sunlit_yaw(double to_sun_x, double to_sun_z, float offset_radians)
{
    /* The camera's horizontal offset is (sin yaw, cos yaw), so the yaw that
     * looks from the Sun's side is atan2(x, z). atan2(0, 0) is 0, which keeps
     * a valid yaw when the Sun is straight above or below. */
    const float full_turn = 2.0f * acosf(-1.0f);
    float yaw = fmodf((float)atan2(to_sun_x, to_sun_z) + offset_radians, full_turn);
    if (yaw < 0.0f) yaw += full_turn;
    return yaw < full_turn ? yaw : 0.0f;
}

void orbit_camera_advance(OrbitCameraState *state, float dt_seconds)
{
    /* Keep yaw in [0, 2*pi). A float has 24 significant bits, so an ever-growing
     * angle loses the small per-frame increment: after a few days at 60 fps the
     * rotation stutters and then stops. Only the angle modulo a full turn
     * matters to sinf/cosf, so wrapping changes nothing visible. */
    const float full_turn = 2.0f * acosf(-1.0f);
    float yaw = fmodf(state->yaw_radians + state->auto_orbit_speed_radians_per_second * dt_seconds, full_turn);
    if (yaw < 0.0f) yaw += full_turn; /* fmodf keeps the dividend's sign. */
    /* A tiny negative remainder plus 2*pi can round up to exactly 2*pi. */
    state->yaw_radians = yaw < full_turn ? yaw : 0.0f;
}

void orbit_camera_frame_sphere(OrbitCameraState *state, float radius, float vertical_fov_degrees, float aspect)
{
    /* The sphere's angular radius must fit the narrower field of view. Using
     * sin rather than tan accounts for the nearest surface, with 15% padding. */
    float vertical_half_angle = vertical_fov_degrees * acosf(-1.0f) / 360.0f;
    float horizontal_half_angle = atanf(tanf(vertical_half_angle) * aspect);
    state->distance = 1.15f * radius / sinf(fminf(vertical_half_angle, horizontal_half_angle));
    state->min_distance = 1.05f * radius;
    state->max_distance = fmaxf(80.0f, state->distance * 4.0f);
    state->zoom_speed = state->distance * 0.1f;
}

static float wrap_turn(float angle)
{
    const float full_turn = 2.0f * acosf(-1.0f);
    float wrapped = fmodf(angle, full_turn);
    if (wrapped < 0.0f) wrapped += full_turn;
    return wrapped < full_turn ? wrapped : 0.0f;
}

void orbit_camera_drag(OrbitCameraState *state, float dx_pixels, float dy_pixels, float viewport_height)
{
    if (!(viewport_height > 0.0f)) return;
    const float radians_per_pixel = acosf(-1.0f) / viewport_height;
    state->yaw_radians = wrap_turn(state->yaw_radians - dx_pixels * radians_per_pixel);
    state->pitch_radians = clamp_float(state->pitch_radians + dy_pixels * radians_per_pixel,
        -ORBIT_CAMERA_MAX_PITCH_RADIANS, ORBIT_CAMERA_MAX_PITCH_RADIANS);
}

void orbit_camera_pinch(OrbitCameraState *state, float ratio)
{
    if (!(ratio > 0.0f) || !isfinite(ratio)) return;
    state->distance = clamp_float(state->distance / ratio, state->min_distance, state->max_distance);
}

void orbit_camera_flight_begin(OrbitCameraFlight *flight, const double origin[3], const OrbitCameraState *current,
    float duration_seconds)
{
    *flight = (OrbitCameraFlight){.active = true, .duration_seconds = duration_seconds > 0.0f ? duration_seconds : 0.0f,
        .from_origin = {origin[0], origin[1], origin[2]}, .from = *current};
}

float orbit_camera_flight_advance(OrbitCameraFlight *flight, float dt_seconds)
{
    if (!flight->active) return 1.0f;
    flight->elapsed_seconds += dt_seconds > 0.0f ? dt_seconds : 0.0f;
    float t = flight->duration_seconds > 0.0f ? flight->elapsed_seconds / flight->duration_seconds : 1.0f;
    if (t >= 1.0f) {
        flight->active = false;
        return 1.0f;
    }
    /* Smootherstep (6t^5 - 15t^4 + 10t^3): zero speed and acceleration at
     * both ends, so the camera neither lurches off nor bumps into place. */
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

void orbit_camera_flight_blend(const OrbitCameraFlight *flight, float progress, const double to_origin[3],
    const OrbitCameraState *to, double out_origin[3], OrbitCameraState *out)
{
    const float pi = acosf(-1.0f);
    float e = clamp_float(progress, 0.0f, 1.0f);
    double travel = 0;
    for (int k = 0; k < 3; ++k) {
        double delta = to_origin[k] - flight->from_origin[k];
        out_origin[k] = e >= 1.0f || flight->track_origin ? to_origin[k] : flight->from_origin[k] + delta * e;
        travel += delta * delta;
    }
    *out = *to;
    /* Turn the short way round. */
    float turn = fmodf(to->yaw_radians - flight->from.yaw_radians, 2.0f * pi);
    if (turn > pi) turn -= 2.0f * pi;
    if (turn < -pi) turn += 2.0f * pi;
    out->yaw_radians = e >= 1.0f ? to->yaw_radians : wrap_turn(flight->from.yaw_radians + turn * e);
    out->pitch_radians = flight->from.pitch_radians + (to->pitch_radians - flight->from.pitch_radians) * e;
    double d0 = flight->from.distance > 1e-30f ? flight->from.distance : 1e-30, d1 = to->distance > 1e-30f ? to->distance : 1e-30;
    double hop = 0.6 * log(1.0 + sqrt(travel) / (d0 > d1 ? d0 : d1));
    double log_distance = log(d0) + (log(d1) - log(d0)) * e + hop * sin(3.14159265358979323846 * (double)e);
    out->distance = e >= 1.0f ? to->distance : (float)exp(log_distance);
}

float orbit_camera_sky_zoom(float fov_degrees, float factor)
{
    if (!(factor > 0.0f) || !isfinite(factor)) return fov_degrees;
    float fov = fov_degrees * factor;
    return fov < ORBIT_CAMERA_SKY_FOV_MIN ? ORBIT_CAMERA_SKY_FOV_MIN : fov > ORBIT_CAMERA_SKY_FOV_MAX ? ORBIT_CAMERA_SKY_FOV_MAX : fov;
}

void orbit_camera_sky_fov_label(float fov_degrees, char *out, size_t size)
{
    if (fov_degrees >= 10.0f) snprintf(out, size, "%.0f deg", fov_degrees);
    else if (fov_degrees >= 1.0f) snprintf(out, size, "%.1f deg", fov_degrees);
    else if (fov_degrees * 60.0f >= 2.0f) snprintf(out, size, "%.0f arcmin", fov_degrees * 60.0f);
    else snprintf(out, size, "%.0f arcsec", fov_degrees * 3600.0f);
}

void orbit_camera_sky_aim(const double target[3], float yaw, float pitch, double out[3])
{
    const double pi = 3.14159265358979323846, limit = pi / 2 - pi / 180;
    double length = sqrt(target[0] * target[0] + target[1] * target[1] + target[2] * target[2]);
    if (!(length > 0)) { out[0] = target[0]; out[1] = target[1]; out[2] = target[2]; return; }
    /* Azimuth in the ecliptic plane and elevation above it. */
    double azimuth = atan2(target[2], target[0]) - yaw;
    double elevation = asin(fmax(-1.0, fmin(1.0, target[1] / length))) + pitch;
    elevation = fmax(-limit, fmin(limit, elevation));
    out[0] = length * cos(elevation) * cos(azimuth);
    out[1] = length * sin(elevation);
    out[2] = length * cos(elevation) * sin(azimuth);
}

float orbit_camera_sky_drag_radians(float pixels, float fov_degrees, float viewport_height)
{
    if (!(viewport_height > 0.0f)) return 0.0f;
    return pixels / viewport_height * fov_degrees * 3.14159265358979323846f / 180.0f;
}

float orbit_camera_sky_clamp_pitch(const double target[3], float pitch)
{
    const double pi = 3.14159265358979323846, limit = pi / 2 - pi / 180;
    double length = sqrt(target[0] * target[0] + target[1] * target[1] + target[2] * target[2]);
    if (!(length > 0)) return pitch;
    double elevation = asin(fmax(-1.0, fmin(1.0, target[1] / length)));
    double low = -limit - elevation, high = limit - elevation;
    return pitch < low ? (float)low : pitch > high ? (float)high : pitch;
}

double orbit_camera_sky_near(const double (*offsets)[3], const double *radii, size_t count,
    const double forward[3], float fov_degrees, float aspect, double floor)
{
    const double pi = 3.14159265358979323846;
    /* Half the field's diagonal plus a margin, as the angle a body's centre
     * may sit off the axis and still be on screen. */
    double half_v = fov_degrees * pi / 360.0, half_h = atan(tan(half_v) * aspect);
    double cone = atan(sqrt(tan(half_v) * tan(half_v) + tan(half_h) * tan(half_h))) * 1.1;
    double fl = sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
    double nearest = INFINITY;
    for (size_t i = 0; i < count && fl > 0; ++i) {
        const double *o = offsets[i];
        double d = sqrt(o[0] * o[0] + o[1] * o[1] + o[2] * o[2]);
        if (!(d > radii[i])) continue;
        double c = (o[0] * forward[0] + o[1] * forward[1] + o[2] * forward[2]) / (d * fl);
        /* The body's own angular radius widens the cone it can touch. */
        double off = acos(fmax(-1.0, fmin(1.0, c))) - asin(radii[i] / d);
        if (off <= cone && d - radii[i] < nearest) nearest = d - radii[i];
    }
    double near = isfinite(nearest) ? 0.5 * nearest : floor;
    return near > floor ? near : floor;
}

float orbit_camera_sky_vertical_fov(float field_degrees, float aspect)
{
    if (!(aspect > 0.0f) || aspect >= 1.0f) return field_degrees;
    const double pi = 3.14159265358979323846;
    return (float)(2.0 * atan(tan(field_degrees * pi / 360.0) / aspect) * 180.0 / pi);
}

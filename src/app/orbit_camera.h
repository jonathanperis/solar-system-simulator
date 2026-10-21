#ifndef ORBIT_CAMERA_H
#define ORBIT_CAMERA_H

#include <stdbool.h>

typedef struct OrbitCameraVec3 {
    float x;
    float y;
    float z;
} OrbitCameraVec3;

typedef struct OrbitCameraState {
    float yaw_radians;
    float pitch_radians;
    float distance;
    float min_distance;
    float max_distance;
    float zoom_speed;
    float auto_orbit_speed_radians_per_second;
} OrbitCameraState;

OrbitCameraState orbit_camera_default_state(void);
OrbitCameraVec3 orbit_camera_position(OrbitCameraVec3 target, const OrbitCameraState *state);
void orbit_camera_apply_zoom(OrbitCameraState *state, float wheel_move);
/* Yaw that places the camera on the sunlit side of a framed body. (to_sun_x,
 * to_sun_z) is the horizontal direction from the body toward the Sun; the
 * camera sits `offset_radians` around from that line so the view shows the
 * day side with a sliver of terminator. Presentation only. Returns [0, 2*pi). */
float orbit_camera_sunlit_yaw(double to_sun_x, double to_sun_z, float offset_radians);
void orbit_camera_advance(OrbitCameraState *state, float dt_seconds);
void orbit_camera_frame_sphere(OrbitCameraState *state, float radius, float vertical_fov_degrees, float aspect);

/* Direct manipulation (SPEC A112). A drag of (dx, dy) pixels on a viewport
 * `viewport_height` pixels tall turns the view as if grabbing the scene: a
 * full-height drag is half a turn. Pitch stops short of the poles so the
 * camera's up vector never flips. A pinch whose finger spread grew by `ratio`
 * moves the camera `ratio` times closer, within the zoom limits. */
#define ORBIT_CAMERA_MAX_PITCH_RADIANS 1.45f
void orbit_camera_drag(OrbitCameraState *state, float dx_pixels, float dy_pixels, float viewport_height);
void orbit_camera_pinch(OrbitCameraState *state, float ratio);

/* Fly-to (SPEC A112): when the framing changes, the shown camera eases from
 * the old view to the new one instead of cutting. The destination is passed
 * to every blend, so a moving body stays the target throughout. The origin
 * (the point the camera orbits, in world render units) is interpolated in
 * double precision; distance moves in log space (a hop from 1 AU away to
 * 1,000 km away reads as a steady zoom) and pulls back mid-flight in
 * proportion to the distance travelled, so long hops show where they go.
 * Presentation only. */
typedef struct OrbitCameraFlight {
    bool active;
    /* The origin goes straight to the destination each frame (set for scale
     * changes, where the destination already glides with the morph). */
    bool track_origin;
    float elapsed_seconds, duration_seconds;
    double from_origin[3];
    OrbitCameraState from;
} OrbitCameraFlight;
void orbit_camera_flight_begin(OrbitCameraFlight *flight, const double origin[3], const OrbitCameraState *current,
    float duration_seconds);
/* Advance by dt and return eased progress in [0, 1]; the flight ends at 1. */
float orbit_camera_flight_advance(OrbitCameraFlight *flight, float dt_seconds);
void orbit_camera_flight_blend(const OrbitCameraFlight *flight, float progress, const double to_origin[3],
    const OrbitCameraState *to, double out_origin[3], OrbitCameraState *out);

#endif

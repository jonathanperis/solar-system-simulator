#ifndef ORBIT_CAMERA_H
#define ORBIT_CAMERA_H

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
} OrbitCameraState;

OrbitCameraState orbit_camera_default_state(void);
OrbitCameraVec3 orbit_camera_position(OrbitCameraVec3 target, const OrbitCameraState *state);
void orbit_camera_apply_zoom(OrbitCameraState *state, float wheel_move);

#endif

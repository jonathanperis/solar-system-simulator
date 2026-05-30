#ifndef SOLAR_VEC3D_H
#define SOLAR_VEC3D_H

typedef struct Vec3d {
    double x;
    double y;
    double z;
} Vec3d;

Vec3d vec3d_zero(void);
Vec3d vec3d_add(Vec3d a, Vec3d b);
Vec3d vec3d_sub(Vec3d a, Vec3d b);
Vec3d vec3d_scale(Vec3d v, double scalar);
double vec3d_dot(Vec3d a, Vec3d b);
double vec3d_length_squared(Vec3d v);
double vec3d_length(Vec3d v);

#endif

#include "vec3d.h"

#include <math.h>

Vec3d vec3d_zero(void)
{
    return (Vec3d){0.0, 0.0, 0.0};
}

Vec3d vec3d_add(Vec3d a, Vec3d b)
{
    return (Vec3d){a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3d vec3d_sub(Vec3d a, Vec3d b)
{
    return (Vec3d){a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3d vec3d_scale(Vec3d v, double scalar)
{
    return (Vec3d){v.x * scalar, v.y * scalar, v.z * scalar};
}

double vec3d_dot(Vec3d a, Vec3d b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

double vec3d_length_squared(Vec3d v)
{
    return vec3d_dot(v, v);
}

double vec3d_length(Vec3d v)
{
    return sqrt(vec3d_length_squared(v));
}

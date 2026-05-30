#include "body.h"

Body body_create(
    const char *name,
    BodyKind kind,
    double mass_kg,
    double radius_m,
    Vec3d position_m,
    Vec3d velocity_mps,
    bool fixed
)
{
    return (Body){
        .name = name,
        .kind = kind,
        .mass_kg = mass_kg,
        .radius_m = radius_m,
        .position_m = position_m,
        .velocity_mps = velocity_mps,
        .acceleration_mps2 = vec3d_zero(),
        .fixed = fixed,
    };
}

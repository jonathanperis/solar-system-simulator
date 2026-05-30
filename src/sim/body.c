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
    return body_create_identified(
        name,
        kind,
        BODY_ID_UNKNOWN,
        BODY_ID_NONE,
        mass_kg,
        radius_m,
        position_m,
        velocity_mps,
        fixed
    );
}

Body body_create_identified(
    const char *name,
    BodyKind kind,
    BodyId id,
    BodyId parent_id,
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
        .id = id,
        .parent_id = parent_id,
        .mass_kg = mass_kg,
        .radius_m = radius_m,
        .position_m = position_m,
        .velocity_mps = velocity_mps,
        .acceleration_mps2 = vec3d_zero(),
        .fixed = fixed,
    };
}

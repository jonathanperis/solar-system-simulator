#include "solar_system.h"

#include "constants.h"
#include "physics.h"

SolarSystem solar_system_create_sun_only(void)
{
    SolarSystem system = {
        .bodies = {
            body_create(
                "Sun",
                BODY_KIND_STAR,
                SOLAR_SUN_MASS_KG,
                SOLAR_SUN_RADIUS_M,
                vec3d_zero(),
                vec3d_zero(),
                true
            ),
        },
        .body_count = 1,
        .elapsed_seconds = 0.0,
    };

    return system;
}

void solar_system_step(SolarSystem *system, double dt_seconds)
{
    physics_step(system->bodies, system->body_count, dt_seconds);
    system->elapsed_seconds += dt_seconds;
}

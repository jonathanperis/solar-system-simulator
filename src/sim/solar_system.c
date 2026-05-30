#include "solar_system.h"

#include "constants.h"
#include "physics.h"

static Body create_sun(void)
{
    return body_create(
        "Sun",
        BODY_KIND_STAR,
        SOLAR_SUN_MASS_KG,
        SOLAR_SUN_RADIUS_M,
        vec3d_zero(),
        vec3d_zero(),
        true
    );
}

Body solar_system_create_mercury_at_perihelion(void)
{
    return body_create(
        "Mercury",
        BODY_KIND_PLANET,
        SOLAR_MERCURY_MASS_KG,
        SOLAR_MERCURY_RADIUS_M,
        (Vec3d){SOLAR_MERCURY_PERIHELION_M, 0.0, 0.0},
        (Vec3d){0.0, 0.0, SOLAR_MERCURY_PERIHELION_SPEED_MPS},
        false
    );
}

Body solar_system_create_venus_at_perihelion(void)
{
    return body_create(
        "Venus",
        BODY_KIND_PLANET,
        SOLAR_VENUS_MASS_KG,
        SOLAR_VENUS_RADIUS_M,
        (Vec3d){-SOLAR_VENUS_PERIHELION_M, 0.0, 0.0},
        (Vec3d){0.0, 0.0, -SOLAR_VENUS_PERIHELION_SPEED_MPS},
        false
    );
}

Body solar_system_create_earth_at_perihelion(void)
{
    return body_create(
        "Earth",
        BODY_KIND_PLANET,
        SOLAR_EARTH_MASS_KG,
        SOLAR_EARTH_RADIUS_M,
        (Vec3d){0.0, 0.0, SOLAR_EARTH_PERIHELION_M},
        (Vec3d){SOLAR_EARTH_PERIHELION_SPEED_MPS, 0.0, 0.0},
        false
    );
}

SolarSystem solar_system_create_sun_only(void)
{
    SolarSystem system = {
        .bodies = {
            create_sun(),
        },
        .body_count = 1,
        .elapsed_seconds = 0.0,
    };

    return system;
}

SolarSystem solar_system_create_sun_mercury(void)
{
    SolarSystem system = {
        .bodies = {
            create_sun(),
            solar_system_create_mercury_at_perihelion(),
        },
        .body_count = 2,
        .elapsed_seconds = 0.0,
    };

    return system;
}

SolarSystem solar_system_create_sun_mercury_venus(void)
{
    SolarSystem system = {
        .bodies = {
            create_sun(),
            solar_system_create_mercury_at_perihelion(),
            solar_system_create_venus_at_perihelion(),
        },
        .body_count = 3,
        .elapsed_seconds = 0.0,
    };

    return system;
}

SolarSystem solar_system_create_sun_mercury_venus_earth(void)
{
    SolarSystem system = {
        .bodies = {
            create_sun(),
            solar_system_create_mercury_at_perihelion(),
            solar_system_create_venus_at_perihelion(),
            solar_system_create_earth_at_perihelion(),
        },
        .body_count = 4,
        .elapsed_seconds = 0.0,
    };

    return system;
}

void solar_system_step(SolarSystem *system, double dt_seconds)
{
    physics_step(system->bodies, system->body_count, dt_seconds);
    system->elapsed_seconds += dt_seconds;
}

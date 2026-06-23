#include "solar_system.h"

#include "constants.h"
#include "physics.h"

static Body create_sun(void)
{
    return body_create_identified(
        "Sun",
        BODY_KIND_STAR,
        BODY_ID_SUN,
        BODY_ID_NONE,
        SOLAR_SUN_MASS_KG,
        SOLAR_SUN_RADIUS_M,
        vec3d_zero(),
        vec3d_zero(),
        true
    );
}

Body solar_system_create_mercury_at_perihelion(void)
{
    return body_create_identified(
        "Mercury",
        BODY_KIND_PLANET,
        BODY_ID_MERCURY,
        BODY_ID_SUN,
        SOLAR_MERCURY_MASS_KG,
        SOLAR_MERCURY_RADIUS_M,
        (Vec3d){SOLAR_MERCURY_PERIHELION_M, 0.0, 0.0},
        (Vec3d){0.0, 0.0, SOLAR_MERCURY_PERIHELION_SPEED_MPS},
        false
    );
}

Body solar_system_create_venus_at_perihelion(void)
{
    return body_create_identified(
        "Venus",
        BODY_KIND_PLANET,
        BODY_ID_VENUS,
        BODY_ID_SUN,
        SOLAR_VENUS_MASS_KG,
        SOLAR_VENUS_RADIUS_M,
        (Vec3d){-SOLAR_VENUS_PERIHELION_M, 0.0, 0.0},
        (Vec3d){0.0, 0.0, -SOLAR_VENUS_PERIHELION_SPEED_MPS},
        false
    );
}

Body solar_system_create_earth_at_perihelion(void)
{
    return body_create_identified(
        "Earth",
        BODY_KIND_PLANET,
        BODY_ID_EARTH,
        BODY_ID_SUN,
        SOLAR_EARTH_MASS_KG,
        SOLAR_EARTH_RADIUS_M,
        (Vec3d){0.0, 0.0, SOLAR_EARTH_PERIHELION_M},
        (Vec3d){-SOLAR_EARTH_PERIHELION_SPEED_MPS, 0.0, 0.0},
        false
    );
}

Body solar_system_create_moon_at_perigee_near_earth(const Body *earth)
{
    /* Satellites are initialized in the same absolute frame as the rest of the
     * N-body system, but their offset and velocity come from parent-relative
     * orbital elements so tests can verify the intended moon/planet relation. */
    Vec3d moon_offset_m = {SOLAR_MOON_PERIGEE_M, 0.0, 0.0};
    Vec3d moon_relative_velocity_mps = {0.0, 0.0, SOLAR_MOON_PERIGEE_SPEED_MPS};

    return body_create_identified(
        "Moon",
        BODY_KIND_MOON,
        BODY_ID_MOON,
        BODY_ID_EARTH,
        SOLAR_MOON_MASS_KG,
        SOLAR_MOON_RADIUS_M,
        vec3d_add(earth->position_m, moon_offset_m),
        vec3d_add(earth->velocity_mps, moon_relative_velocity_mps),
        false
    );
}

Body solar_system_create_mars_at_perihelion(void)
{
    return body_create_identified(
        "Mars",
        BODY_KIND_PLANET,
        BODY_ID_MARS,
        BODY_ID_SUN,
        SOLAR_MARS_MASS_KG,
        SOLAR_MARS_RADIUS_M,
        (Vec3d){0.0, 0.0, -SOLAR_MARS_PERIHELION_M},
        (Vec3d){SOLAR_MARS_PERIHELION_SPEED_MPS, 0.0, 0.0},
        false
    );
}

Body solar_system_create_phobos_at_periareion_near_mars(const Body *mars)
{
    /* Keep Mars-relative satellite motion in the same X/Z ecliptic plane as
     * the planet orbits. Visual inclinations can be modeled later, but the
     * default no-inclination scene should not draw vertical moon trails. */
    Vec3d offset_m = {SOLAR_PHOBOS_PERIAREION_M, 0.0, 0.0};
    Vec3d relative_velocity_mps = {0.0, 0.0, SOLAR_PHOBOS_PERIAREION_SPEED_MPS};

    return body_create_identified(
        "Phobos",
        BODY_KIND_MOON,
        BODY_ID_PHOBOS,
        BODY_ID_MARS,
        SOLAR_PHOBOS_MASS_KG,
        SOLAR_PHOBOS_RADIUS_M,
        vec3d_add(mars->position_m, offset_m),
        vec3d_add(mars->velocity_mps, relative_velocity_mps),
        false
    );
}

Body solar_system_create_deimos_at_periareion_near_mars(const Body *mars)
{
    Vec3d offset_m = {-SOLAR_DEIMOS_PERIAREION_M, 0.0, 0.0};
    Vec3d relative_velocity_mps = {0.0, 0.0, -SOLAR_DEIMOS_PERIAREION_SPEED_MPS};

    return body_create_identified(
        "Deimos",
        BODY_KIND_MOON,
        BODY_ID_DEIMOS,
        BODY_ID_MARS,
        SOLAR_DEIMOS_MASS_KG,
        SOLAR_DEIMOS_RADIUS_M,
        vec3d_add(mars->position_m, offset_m),
        vec3d_add(mars->velocity_mps, relative_velocity_mps),
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

SolarSystem solar_system_create_sun_mercury_venus_earth_moon(void)
{
    Body earth = solar_system_create_earth_at_perihelion();

    SolarSystem system = {
        .bodies = {
            create_sun(),
            solar_system_create_mercury_at_perihelion(),
            solar_system_create_venus_at_perihelion(),
            earth,
            solar_system_create_moon_at_perigee_near_earth(&earth),
        },
        .body_count = 5,
        .elapsed_seconds = 0.0,
    };

    return system;
}

SolarSystem solar_system_create_sun_mercury_venus_earth_moon_mars(void)
{
    Body earth = solar_system_create_earth_at_perihelion();

    SolarSystem system = {
        .bodies = {
            create_sun(),
            solar_system_create_mercury_at_perihelion(),
            solar_system_create_venus_at_perihelion(),
            earth,
            solar_system_create_moon_at_perigee_near_earth(&earth),
            solar_system_create_mars_at_perihelion(),
        },
        .body_count = 6,
        .elapsed_seconds = 0.0,
    };

    return system;
}

SolarSystem solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos(void)
{
    Body earth = solar_system_create_earth_at_perihelion();
    Body mars = solar_system_create_mars_at_perihelion();

    SolarSystem system = {
        .bodies = {
            create_sun(),
            solar_system_create_mercury_at_perihelion(),
            solar_system_create_venus_at_perihelion(),
            earth,
            solar_system_create_moon_at_perigee_near_earth(&earth),
            mars,
            solar_system_create_phobos_at_periareion_near_mars(&mars),
            solar_system_create_deimos_at_periareion_near_mars(&mars),
        },
        .body_count = 8,
        .elapsed_seconds = 0.0,
    };

    return system;
}

void solar_system_step(SolarSystem *system, double dt_seconds)
{
    physics_step(system->bodies, system->body_count, dt_seconds);
    system->elapsed_seconds += dt_seconds;
}

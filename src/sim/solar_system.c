#include "solar_system.h"

#include "constants.h"
#include "orbit.h"
#include "physics.h"

_Static_assert(SOLAR_CORE_SCENE_BODY_COUNT == 128, "V5: the core scene has 128 bodies");
_Static_assert(SOLAR_CORE_SCENE_BODY_COUNT <= SOLAR_SYSTEM_BODY_CAPACITY, "core scene must fit the body array");

/* Body is ~136 bytes, so it is passed by const pointer and copied once into
 * the array instead of being copied again into the parameter. */
bool solar_system_append(SolarSystem *system, const Body *body)
{
    if (system->body_count >= SOLAR_SYSTEM_BODY_CAPACITY) return false;
    system->bodies[system->body_count++] = *body;
    return true;
}

/* Legacy demonstration states are written as in-plane J2000 ecliptic
 * coordinates (X, Y; Z = north is zero) and mapped once into simulation axes.
 * Every legacy velocity is counterclockwise seen from ecliptic north, i.e.
 * prograde: r x v points to +Z ecliptic, which is simulation +Y. */
static Vec3d ecliptic_plane(double x_m, double y_m)
{
    return orbit_ecliptic_to_simulation((Vec3d){x_m, y_m, 0.0});
}

/* Moons are created around their parent's intended heliocentric state. Left
 * there, the parent would carry that state while its moons add momentum, so
 * the family's center of mass would drift off the intended orbit (about
 * 12 m/s for Earth-Moon). Instead the intended state belongs to the family
 * barycenter: shift the parent and its direct moons together by minus the
 * mass-weighted mean offset. Parent-relative states are unchanged, and
 * massless test particles carry zero weight. Call once, right after the
 * family's moons are appended, while the parent still has its intended state. */
static void place_family_barycenter(SolarSystem *system, size_t parent_index)
{
    const Body *parent = &system->bodies[parent_index];
    const BodyId parent_id = parent->id;
    const Vec3d intended_position = parent->position_m, intended_velocity = parent->velocity_mps;
    double family_mass = 0.0;
    Vec3d weighted_offset = vec3d_zero(), weighted_velocity = vec3d_zero();
    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        if (i != parent_index && body->parent_id != parent_id) continue;
        /* Offsets from the intended state keep the sum small and precise. */
        family_mass += body->mass_kg;
        weighted_offset = vec3d_add(weighted_offset, vec3d_scale(vec3d_sub(body->position_m, intended_position), body->mass_kg));
        weighted_velocity = vec3d_add(weighted_velocity, vec3d_scale(vec3d_sub(body->velocity_mps, intended_velocity), body->mass_kg));
    }
    if (family_mass <= 0.0) return;
    Vec3d shift = vec3d_scale(weighted_offset, 1.0 / family_mass);
    Vec3d drift = vec3d_scale(weighted_velocity, 1.0 / family_mass);
    for (size_t i = 0; i < system->body_count; ++i) {
        Body *body = &system->bodies[i];
        if (i != parent_index && body->parent_id != parent_id) continue;
        body->position_m = vec3d_sub(body->position_m, shift);
        body->velocity_mps = vec3d_sub(body->velocity_mps, drift);
    }
}

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
        ecliptic_plane(SOLAR_MERCURY_PERIHELION_M, 0.0),
        ecliptic_plane(0.0, SOLAR_MERCURY_PERIHELION_SPEED_MPS),
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
        ecliptic_plane(-SOLAR_VENUS_PERIHELION_M, 0.0),
        ecliptic_plane(0.0, -SOLAR_VENUS_PERIHELION_SPEED_MPS),
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
        ecliptic_plane(0.0, SOLAR_EARTH_PERIHELION_M),
        ecliptic_plane(-SOLAR_EARTH_PERIHELION_SPEED_MPS, 0.0),
        false
    );
}

Body solar_system_create_moon_at_perigee_near_earth(const Body *earth)
{
    /* Satellites are initialized in the same absolute frame as the rest of the
     * N-body system, but their offset and velocity come from parent-relative
     * orbital elements so tests can verify the intended moon/planet relation. */
    Vec3d moon_offset_m = ecliptic_plane(SOLAR_MOON_PERIGEE_M, 0.0);
    Vec3d moon_relative_velocity_mps = ecliptic_plane(0.0, SOLAR_MOON_PERIGEE_SPEED_MPS);

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
        ecliptic_plane(0.0, -SOLAR_MARS_PERIHELION_M),
        ecliptic_plane(SOLAR_MARS_PERIHELION_SPEED_MPS, 0.0),
        false
    );
}

Body solar_system_create_phobos_at_periareion_near_mars(const Body *mars)
{
    /* Keep Mars-relative satellite motion in the same ecliptic plane (the
     * simulation X/Z plane) as the planet orbits. Visual inclinations can be
     * modeled later, but the no-inclination scene draws no vertical trails. */
    Vec3d offset_m = ecliptic_plane(SOLAR_PHOBOS_PERIAREION_M, 0.0);
    Vec3d relative_velocity_mps = ecliptic_plane(0.0, SOLAR_PHOBOS_PERIAREION_SPEED_MPS);

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
    Vec3d offset_m = ecliptic_plane(-SOLAR_DEIMOS_PERIAREION_M, 0.0);
    Vec3d relative_velocity_mps = ecliptic_plane(0.0, -SOLAR_DEIMOS_PERIAREION_SPEED_MPS);

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

Body solar_system_create_vesta_at_perihelion(void)
{
    /* Vesta's measured inclination is intentionally omitted until a dedicated
     * orbital-geometry milestone. It stays in the ecliptic (simulation X/Z) plane. */
    return body_create_identified(
        "Vesta",
        BODY_KIND_ASTEROID,
        BODY_ID_VESTA,
        BODY_ID_SUN,
        SOLAR_VESTA_MASS_KG,
        SOLAR_VESTA_RADIUS_M,
        ecliptic_plane(SOLAR_VESTA_PERIHELION_M, 0.0),
        ecliptic_plane(0.0, SOLAR_VESTA_PERIHELION_SPEED_MPS),
        false
    );
}

Body solar_system_create_jupiter_at_perihelion(void)
{
    return body_create_identified(
        "Jupiter",
        BODY_KIND_PLANET,
        BODY_ID_JUPITER,
        BODY_ID_SUN,
        SOLAR_JUPITER_MASS_KG,
        SOLAR_JUPITER_RADIUS_M,
        ecliptic_plane(-SOLAR_JUPITER_PERIHELION_M, 0.0),
        ecliptic_plane(0.0, -SOLAR_JUPITER_PERIHELION_SPEED_MPS),
        false
    );
}

Body solar_system_create_saturn_at_perihelion(void)
{
    return body_create_identified(
        "Saturn",
        BODY_KIND_PLANET,
        BODY_ID_SATURN,
        BODY_ID_SUN,
        SOLAR_SATURN_MASS_KG,
        SOLAR_SATURN_RADIUS_M,
        ecliptic_plane(0.0, SOLAR_SATURN_PERIHELION_M),
        ecliptic_plane(-SOLAR_SATURN_PERIHELION_SPEED_MPS, 0.0),
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

SolarSystem solar_system_create_sun_vesta(void)
{
    SolarSystem system = {
        .bodies = {
            create_sun(),
            solar_system_create_vesta_at_perihelion(),
        },
        .body_count = 2,
        .elapsed_seconds = 0.0,
    };

    return system;
}

SolarSystem solar_system_create_sun_jupiter(void)
{
    SolarSystem system = {
        .bodies = {
            create_sun(),
            solar_system_create_jupiter_at_perihelion(),
        },
        .body_count = 2,
        .elapsed_seconds = 0.0,
    };

    return system;
}

SolarSystem solar_system_create_sun_saturn(void)
{
    SolarSystem system = {
        .bodies = {
            create_sun(),
            solar_system_create_saturn_at_perihelion(),
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

    place_family_barycenter(&system, 3);
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

    place_family_barycenter(&system, 3);
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

    place_family_barycenter(&system, 3);
    place_family_barycenter(&system, 5);
    return system;
}

SolarSystem solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos_vesta(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    /* Nine of 128 slots are used here, so the bounded append cannot fail. */
    Body vesta = solar_system_create_vesta_at_perihelion();
    (void)solar_system_append(&system, &vesta);
    return system;
}

SolarSystem solar_system_create_current(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos_vesta_jupiter();
    /* The static assertions above prove the inventory fits; tests also check
     * that body_count reaches SOLAR_CORE_SCENE_BODY_COUNT. */
    for (size_t i = 0; i < SOLAR_JOVIAN_MOON_COUNT; ++i) {
        Body moon = satellite_create(&solar_jovian_moons[i], &system.bodies[9]);
        (void)solar_system_append(&system, &moon);
    }
    place_family_barycenter(&system, 9);
    Body outer_planets[] = {
        solar_system_create_saturn_at_perihelion(),
        solar_system_create_uranus_at_perihelion(),
        solar_system_create_neptune_at_perihelion(),
    };
    for (size_t i = 0; i < sizeof(outer_planets) / sizeof(outer_planets[0]); ++i)
        (void)solar_system_append(&system, &outer_planets[i]);
    return system;
}

Body solar_system_create_uranus_at_perihelion(void)
{
    double q = SOLAR_URANUS_SEMI_MAJOR_AXIS_M * (1-SOLAR_URANUS_ECCENTRICITY);
    double v = sqrt(SOLAR_G*SOLAR_SUN_MASS_KG*(2/q-1/SOLAR_URANUS_SEMI_MAJOR_AXIS_M));
    return body_create_identified("Uranus", BODY_KIND_PLANET, BODY_ID_URANUS, BODY_ID_SUN,
        SOLAR_URANUS_MASS_KG, SOLAR_URANUS_RADIUS_M, ecliptic_plane(q, 0), ecliptic_plane(0, v), false);
}

Body solar_system_create_neptune_at_perihelion(void)
{
    double q = SOLAR_NEPTUNE_SEMI_MAJOR_AXIS_M * (1-SOLAR_NEPTUNE_ECCENTRICITY);
    double v = sqrt(SOLAR_G*SOLAR_SUN_MASS_KG*(2/q-1/SOLAR_NEPTUNE_SEMI_MAJOR_AXIS_M));
    return body_create_identified("Neptune", BODY_KIND_PLANET, BODY_ID_NEPTUNE, BODY_ID_SUN,
        SOLAR_NEPTUNE_MASS_KG, SOLAR_NEPTUNE_RADIUS_M, ecliptic_plane(0, -q), ecliptic_plane(v, 0), false);
}

SolarSystem solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos_vesta_jupiter(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos_vesta();
    Body jupiter = solar_system_create_jupiter_at_perihelion();
    (void)solar_system_append(&system, &jupiter);
    return system;
}

void solar_system_step(SolarSystem *system, double dt_seconds)
{
    physics_step(system->bodies, system->body_count, dt_seconds);
    system->elapsed_seconds += dt_seconds;
}
int solar_system_parent_index(const SolarSystem *system, size_t body_index)
{
    if (body_index >= system->body_count) return -1;
    BodyId parent = system->bodies[body_index].parent_id;
    if (parent == BODY_ID_NONE || parent == BODY_ID_UNKNOWN) return -1;
    for (size_t i = 0; i < system->body_count; ++i) {
        if (system->bodies[i].id == parent) return (int)i;
    }
    return -1;
}

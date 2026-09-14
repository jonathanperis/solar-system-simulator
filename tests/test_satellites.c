#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "sim/constants.h"
#include "sim/jovian_catalog.h"
#include "sim/solar_system.h"

static void test_orbital_elements_preserve_geometry_and_parent_motion(void)
{
    Body parent = solar_system_create_jupiter_at_perihelion();
    /* Cardinal orientations give independent vector oracles for the source
     * frame conversion, including a polar and a retrograde orbit. */
    const double inclinations[] = {0, 90, 180};
    for (size_t k = 0; k < 3; ++k) {
        SatelliteDefinition moon = {.name = "Test", .code = 501,
            .a_km = 1000000, .eccentricity = 0.1, .inclination_deg = inclinations[k]};
        Body body = satellite_create(&moon, &parent);
        Vec3d r = vec3d_sub(body.position_m, parent.position_m);
        Vec3d v = vec3d_sub(body.velocity_mps, parent.velocity_mps);
        double speed = sqrt(SOLAR_G * parent.mass_kg * (2 / 9e8 - 1 / 1e9));
        assert(fabs(r.x - 9e8) < 0.001);
        assert(fabs(r.y) < 0.001 && fabs(r.z) < 0.001);
        assert(fabs(v.x) < 1e-8);
        assert(fabs(v.y - speed * sin(inclinations[k] * acos(-1) / 180)) < 1e-8);
        assert(fabs(v.z - speed * cos(inclinations[k] * acos(-1) / 180)) < 1e-8);
    }
    SatelliteDefinition laplace = {.name = "Plane", .code = 502, .a_km = 1000000,
        .frame = SATELLITE_FRAME_LAPLACE, .pole_ra_deg = 0, .pole_dec_deg = 0};
    Body body = satellite_create(&laplace, &parent);
    Vec3d r = vec3d_sub(body.position_m, parent.position_m);
    /* Pole +ICRF X means reference node +ICRF Y. Obliquity rotates it into
     * ecliptic Y/Z, then simulator Y=ec.Z and Z=ec.Y. */
    const double obliquity = 23.439291111 * acos(-1) / 180;
    assert(fabs(r.x) < 0.001);
    assert(fabs(r.y + 1e9 * sin(obliquity)) < 0.001);
    assert(fabs(r.z - 1e9 * cos(obliquity)) < 0.001);

    /* At M=180 degrees we are at apoapsis. Two quarter-turns about the
     * reference normal put it on +X, independently checking phase and angles. */
    SatelliteDefinition apoapsis = {.name = "Apoapsis", .code = 503, .a_km = 1000000,
        .eccentricity = 0.1, .mean_anomaly_deg = 180, .periapsis_deg = 90, .node_deg = 90};
    body = satellite_create(&apoapsis, &parent);
    r = vec3d_sub(body.position_m, parent.position_m);
    Vec3d v = vec3d_sub(body.velocity_mps, parent.velocity_mps);
    assert(fabs(r.x - 1.1e9) < 0.001 && fabs(r.y) < 0.001 && fabs(r.z) < 0.001);
    assert(fabs(v.z - sqrt(SOLAR_G * parent.mass_kg * (2 / 1.1e9 - 1 / 1e9))) < 1e-8);
}

static void test_complete_jovian_catalog_and_initial_orbits(void)
{
    SolarSystem system = solar_system_create_current();
    assert(SOLAR_JOVIAN_MOON_COUNT == 115 && system.body_count == 128);
    assert(system.bodies[9].id == BODY_ID_JUPITER);
    assert(strcmp(system.bodies[10].name, "Io") == 0);
    assert(strcmp(system.bodies[13].name, "Callisto") == 0);
    size_t unknown = 0;
    for (size_t i = 0; i < SOLAR_JOVIAN_MOON_COUNT; ++i) {
        const SatelliteDefinition *def = &solar_jovian_moons[i];
        Body *body = &system.bodies[10 + i];
        assert((int)body->id == def->code);
        assert(body->parent_id == BODY_ID_JUPITER && !body->fixed);
        assert(solar_system_parent_index(&system, 10 + i) == 9);
        for (size_t j = 0; j < i; ++j) assert(body->id != system.bodies[10 + j].id);
        Vec3d r = vec3d_sub(body->position_m, system.bodies[9].position_m);
        Vec3d v = vec3d_sub(body->velocity_mps, system.bodies[9].velocity_mps);
        double a = def->a_km * 1000;
        double mu = SOLAR_G * (system.bodies[9].mass_kg + body->mass_kg);
        double energy = vec3d_length_squared(v) / 2 - mu / vec3d_length(r);
        assert(fabs(energy / (-mu / (2 * a)) - 1) < 1e-9);
        assert(vec3d_length(r) >= a * (1 - def->eccentricity) - 0.001);
        assert(vec3d_length(r) <= a * (1 + def->eccentricity) + 0.001);
        if (body->mass_quality == PHYSICAL_UNKNOWN) {
            ++unknown;
            assert(body->mass_kg == 0 && body->radius_quality == PHYSICAL_UNKNOWN);
        }
    }
    assert(unknown == 106);
    assert(fabs(system.bodies[10].mass_kg * SOLAR_G / 1e9 - 5959.91547) < 1e-8);
    assert(system.bodies[10].radius_m == 1821490);
}

int main(void)
{
    test_orbital_elements_preserve_geometry_and_parent_motion();
    test_complete_jovian_catalog_and_initial_orbits();
    puts("test_satellites passed");
}

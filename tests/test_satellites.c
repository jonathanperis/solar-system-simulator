#include "require_assert.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "sim/constants.h"
#include "sim/experiment.h"
#include "sim/jovian_catalog.h"
#include "sim/lessons.h"
#include "sim/orbit.h"
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
        /* In-plane ecliptic +Y is simulation -Z; inclination tilts it north (+Y). */
        assert(fabs(v.y - speed * sin(inclinations[k] * acos(-1) / 180)) < 1e-8);
        assert(fabs(v.z + speed * cos(inclinations[k] * acos(-1) / 180)) < 1e-8);
    }
    SatelliteDefinition laplace = {.name = "Plane", .code = 502, .a_km = 1000000,
        .frame = SATELLITE_FRAME_LAPLACE, .pole_ra_deg = 0, .pole_dec_deg = 0};
    Body body = satellite_create(&laplace, &parent);
    Vec3d r = vec3d_sub(body.position_m, parent.position_m);
    /* Pole +ICRF X means reference node +ICRF Y. Obliquity rotates it into
     * ecliptic Y/Z, then simulator (x, y, z) = ecliptic (X, Z, -Y). */
    const double obliquity = 23.439291111 * acos(-1) / 180;
    assert(fabs(r.x) < 0.001);
    assert(fabs(r.y + 1e9 * sin(obliquity)) < 0.001);
    assert(fabs(r.z + 1e9 * cos(obliquity)) < 0.001);

    /* At M=180 degrees we are at apoapsis. Two quarter-turns about the
     * reference normal put it on +X, independently checking phase and angles. */
    SatelliteDefinition apoapsis = {.name = "Apoapsis", .code = 503, .a_km = 1000000,
        .eccentricity = 0.1, .mean_anomaly_deg = 180, .periapsis_deg = 90, .node_deg = 90};
    body = satellite_create(&apoapsis, &parent);
    r = vec3d_sub(body.position_m, parent.position_m);
    Vec3d v = vec3d_sub(body.velocity_mps, parent.velocity_mps);
    assert(fabs(r.x - 1.1e9) < 0.001 && fabs(r.y) < 0.001 && fabs(r.z) < 0.001);
    assert(fabs(v.z + sqrt(SOLAR_G * parent.mass_kg * (2 / 1.1e9 - 1 / 1e9))) < 1e-8);
}

/* Parent-relative states produced by the former fixed-12-iteration
 * eccentric-anomaly solver in satellite.c, captured before orbit.c became the
 * only Kepler solver (A58). Aoede has the catalog's largest eccentricity.
 * They were captured in the former reflected axes (X, Z, Y); the proper
 * rotation (X, Z, -Y) adopted later negates only their z components. */
static void test_shared_conic_solver_reproduces_former_jovian_states(void)
{
    const struct { int code; Vec3d r, v; } former[] = {
    {501, {399575698.28771973, 10358419.917243615, 130027180.29385185}, {-5423.6301040120397, 513.27701459524815, 16516.265081396425}},
    {502, {-561543514.51696777, -17846866.935899183, -356257518.30371094}, {7455.4697554733511, -206.80339833419112, -11681.936870667454}},
    {506, {-4975624582.7926025, 4570634900.7620564, 9129780032.6524658}, {-2849.7185993760531, 1054.3877480410076, -1414.4343158764841}},
    {541, {7518843363.793335, -11354612099.834766, -30691348141.629383}, {-1465.2261142717032, -264.83318574687002, -2.4894121291381452}},
    };
    Body jupiter = solar_system_create_jupiter_at_perihelion();
    double max_e = 0;
    for (size_t i = 0; i < SOLAR_JOVIAN_MOON_COUNT; ++i) max_e = fmax(max_e, solar_jovian_moons[i].eccentricity);
    size_t matched = 0;
    for (size_t k = 0; k < sizeof(former) / sizeof(former[0]); ++k) {
        for (size_t i = 0; i < SOLAR_JOVIAN_MOON_COUNT; ++i) {
            const SatelliteDefinition *d = &solar_jovian_moons[i];
            if (d->code != former[k].code) continue;
            if (d->code == 541) assert(d->eccentricity == max_e);
            Body moon = satellite_create(d, &jupiter);
            Vec3d r = vec3d_sub(moon.position_m, jupiter.position_m);
            Vec3d v = vec3d_sub(moon.velocity_mps, jupiter.velocity_mps);
            r.z = -r.z;
            v.z = -v.z;
            /* The former values were differences of ~7e11 m absolute
             * positions, so they carry ~1e-4 m roundoff of their own. */
            assert(vec3d_length(vec3d_sub(r, former[k].r)) / vec3d_length(former[k].r) < 1e-11);
            assert(vec3d_length(vec3d_sub(v, former[k].v)) / vec3d_length(former[k].v) < 1e-11);
            ++matched;
        }
    }
    assert(matched == 4);
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

/* +1 when an orbit is prograde (counterclockwise seen from ecliptic north),
 * -1 when retrograde. Jovian moons use their source-frame inclination. */
static int expected_direction(const Body *body)
{
    for (size_t i = 0; i < SOLAR_JOVIAN_MOON_COUNT; ++i)
        if ((int)body->id == solar_jovian_moons[i].code) return solar_jovian_moons[i].inclination_deg > 90 ? -1 : 1;
    return 1;
}

/* Simulation Y is ecliptic north. A proper (right-handed) frame makes the
 * parent-relative specific angular momentum r x v point to +Y for prograde
 * motion, which raylib's right-handed camera with up=+Y draws counterclockwise. */
static void assert_orbit_directions(const SolarSystem *system, const int *override)
{
    size_t checked = 0;
    for (size_t i = 0; i < system->body_count; ++i) {
        int parent = solar_system_parent_index(system, i);
        if (parent < 0) continue;
        const Body *body = &system->bodies[i], *center = &system->bodies[parent];
        Vec3d r = vec3d_sub(body->position_m, center->position_m);
        Vec3d v = vec3d_sub(body->velocity_mps, center->velocity_mps);
        Vec3d h = vec3d_cross(r, v);
        int expected = override && override[i] ? override[i] : expected_direction(body);
        assert(h.y * expected > 1e-6 * vec3d_length(h));
        ++checked;
    }
    assert(checked > 0);
}

static void test_ecliptic_frame_is_a_proper_rotation_with_prograde_plus_y(void)
{
    Vec3d x = orbit_ecliptic_to_simulation((Vec3d){1, 0, 0});
    Vec3d y = orbit_ecliptic_to_simulation((Vec3d){0, 1, 0});
    Vec3d z = orbit_ecliptic_to_simulation((Vec3d){0, 0, 1});
    /* Ecliptic north is simulation +Y, and X x Y = Z survives the mapping
     * (determinant +1): a rotation, not a mirror image. */
    assert(z.x == 0 && z.y == 1 && z.z == 0);
    assert(vec3d_length(vec3d_sub(vec3d_cross(x, y), z)) == 0);

    SolarSystem core = solar_system_create_current();
    assert_orbit_directions(&core, NULL);
    size_t retrograde = 0;
    for (size_t i = 0; i < SOLAR_JOVIAN_MOON_COUNT; ++i) retrograde += solar_jovian_moons[i].inclination_deg > 90;
    assert(retrograde > 0 && retrograde < SOLAR_JOVIAN_MOON_COUNT);

    for (int preset = 0; preset < LESSON_COUNT; ++preset) {
        if (preset == LESSON_COLLISION) continue; /* head-on, parentless spheres */
        SolarSystem lesson;
        assert(lesson_create((LessonPreset)preset, 1, &lesson));
        assert_orbit_directions(&lesson, NULL);
    }

    /* Catalog experiment: Horizons planets are prograde; a source inclination
     * above 90 degrees (1I/'Oumuamua, 122.7) must stay retrograde. */
    SolarSystem experiment;
    char names[SOLAR_EXPERIMENT_CAPACITY][SOLAR_EXPERIMENT_NAME_BYTES];
    assert(experiment_parse("SOLAR_EXPERIMENT_V1 2461200.5\n"
        "20000004\tVesta\t2.148\t0.09\t7.14\t103.7\t151.4\t2461000.5\t0\t0\t2\t2\n"
        "50788063\tOumuamua\t0.2559\t1.201\t122.7\t24.6\t241.8\t2458006\t0\t0\t2\t2\n", &experiment, names));
    int directions[SOLAR_SYSTEM_BODY_CAPACITY] = {0};
    directions[10] = -1;
    assert_orbit_directions(&experiment, directions);
}

int main(void)
{
    test_ecliptic_frame_is_a_proper_rotation_with_prograde_plus_y();
    test_orbital_elements_preserve_geometry_and_parent_motion();
    test_shared_conic_solver_reproduces_former_jovian_states();
    test_complete_jovian_catalog_and_initial_orbits();
    puts("test_satellites passed");
}

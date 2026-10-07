#include "require_assert.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "sim/constants.h"
#include "sim/experiment.h"
#include "sim/satellite_catalog.h"
#include "sim/lessons.h"
#include "sim/orbit.h"
#include "sim/solar_system.h"

static Vec3d icrf_direction(double ra_deg, double dec_deg);

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
            /* The former states used Jupiter's earlier literal mass 1.898125e27
             * kg; it is now GM/G (A74). A conic's position at a given mean
             * anomaly does not depend on mu, and its velocity scales as
             * sqrt(mu), so rescale the captured velocity to the current mu. */
            double moon_mass = d->gm_km3_s2 * 1e9 / SOLAR_G;
            Vec3d former_v = vec3d_scale(former[k].v,
                sqrt((jupiter.mass_kg + moon_mass) / (1.898125e27 + moon_mass)));
            /* The former values were differences of ~7e11 m absolute
             * positions, so they carry ~1e-4 m roundoff of their own. */
            assert(vec3d_length(vec3d_sub(r, former[k].r)) / vec3d_length(former[k].r) < 1e-11);
            assert(vec3d_length(vec3d_sub(v, former_v)) / vec3d_length(former_v) < 1e-11);
            ++matched;
        }
    }
    assert(matched == 4);
}

static void test_complete_jovian_catalog_and_initial_orbits(void)
{
    /* The whole catalog lives in Jupiter's family scene (Jupiter at index 5,
     * then the catalog from index 9; the Galilean moons lead both). */
    SolarSystem system;
    assert(solar_system_create_family(BODY_ID_JUPITER, &system));
    assert(SOLAR_JOVIAN_MOON_COUNT == 115 && system.body_count == 124);
    assert(system.bodies[5].id == BODY_ID_JUPITER);
    assert(strcmp(system.bodies[9].name, "Io") == 0);
    assert(strcmp(system.bodies[12].name, "Callisto") == 0);
    size_t unknown = 0;
    for (size_t i = 0; i < SOLAR_JOVIAN_MOON_COUNT; ++i) {
        const SatelliteDefinition *def = &solar_jovian_moons[i];
        Body *body = &system.bodies[9 + i];
        assert((int)body->id == def->code);
        assert(body->parent_id == BODY_ID_JUPITER && !body->fixed);
        assert(solar_system_parent_index(&system, 9 + i) == 5);
        for (size_t j = 0; j < i; ++j) assert(body->id != system.bodies[9 + j].id);
        Vec3d r = vec3d_sub(body->position_m, system.bodies[5].position_m);
        Vec3d v = vec3d_sub(body->velocity_mps, system.bodies[5].velocity_mps);
        double a = def->a_km * 1000;
        double mu = SOLAR_G * (system.bodies[5].mass_kg + body->mass_kg);
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
    assert(fabs(system.bodies[9].mass_kg * SOLAR_G / 1e9 - 5959.91547) < 1e-8);
    assert(system.bodies[9].radius_m == 1821490);
}

/* +1 when an orbit is prograde (counterclockwise seen from ecliptic north),
 * -1 when retrograde. Catalog moons follow their source frame: an ecliptic
 * inclination above 90 degrees is retrograde; in a Laplace or equatorial
 * frame the plane's pole decides, so Uranus's regular moons, which orbit a
 * pole tilted past the ecliptic (obliquity ~98 degrees), count as retrograde. */
static int expected_direction(const Body *body)
{
    const BodyId planets[] = {BODY_ID_JUPITER, BODY_ID_SATURN, BODY_ID_URANUS, BODY_ID_NEPTUNE};
    for (size_t k = 0; k < 4; ++k) {
        const SatelliteCatalog *catalog = satellite_catalog_for(planets[k]);
        for (size_t i = 0; i < catalog->count; ++i) {
            const SatelliteDefinition *d = &catalog->moons[i];
            if ((int)body->id != d->code) continue;
            double up = d->frame == SATELLITE_FRAME_ECLIPTIC ? 1 : icrf_direction(d->pole_ra_deg, d->pole_dec_deg).y;
            return up * cos(d->inclination_deg * acos(-1.0) / 180) > 0 ? 1 : -1;
        }
    }
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
    const BodyId planets[] = {BODY_ID_JUPITER, BODY_ID_SATURN, BODY_ID_URANUS, BODY_ID_NEPTUNE};
    for (size_t k = 0; k < 4; ++k) {
        SolarSystem family;
        assert(solar_system_create_family(planets[k], &family));
        assert_orbit_directions(&family, NULL);
    }
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

static Body parent_planet(BodyId planet)
{
    switch (planet) {
    case BODY_ID_JUPITER: return solar_system_create_jupiter_at_perihelion();
    case BODY_ID_SATURN: return solar_system_create_saturn_at_perihelion();
    case BODY_ID_URANUS: return solar_system_create_uranus_at_perihelion();
    default: return solar_system_create_neptune_at_perihelion();
    }
}

/* Parent-relative orbit normal r x v of a catalog moon, as a unit vector. */
static Vec3d orbit_normal(const SatelliteDefinition *d, const Body *parent)
{
    Body moon = satellite_create(d, parent);
    Vec3d h = vec3d_cross(vec3d_sub(moon.position_m, parent->position_m), vec3d_sub(moon.velocity_mps, parent->velocity_mps));
    return vec3d_scale(h, 1 / vec3d_length(h));
}

static const SatelliteDefinition *find_moon(const SatelliteCatalog *catalog, int code)
{
    for (size_t i = 0; i < catalog->count; ++i)
        if (catalog->moons[i].code == code) return &catalog->moons[i];
    assert(!"moon code missing from its catalog");
    return NULL;
}

static double angle_degrees(Vec3d a, Vec3d b)
{
    /* atan2 stays precise for nearly parallel vectors, where acos does not. */
    return atan2(vec3d_length(vec3d_cross(a, b)), vec3d_dot(a, b)) * 180 / acos(-1.0);
}

/* Unit vector toward ICRF (RA, Dec), in simulation axes. */
static Vec3d icrf_direction(double ra_deg, double dec_deg)
{
    double ra = ra_deg * acos(-1.0) / 180, dec = dec_deg * acos(-1.0) / 180;
    Vec3d equatorial = {cos(dec) * cos(ra), cos(dec) * sin(ra), sin(dec)};
    return orbit_ecliptic_to_simulation(orbit_rotate_to_reference(equatorial, -23.439291111, 0, 0));
}

static void test_giant_planet_catalogs_inventory_and_major_moons(void)
{
    const struct { BodyId planet; size_t count; int majors[8]; size_t major_count; } expected[] = {
        {BODY_ID_JUPITER, 115, {501, 502, 503, 504}, 4},
        {BODY_ID_SATURN, 291, {601, 602, 603, 604, 605, 606, 608}, 7},
        {BODY_ID_URANUS, 29, {701, 702, 703, 704, 705}, 5},
        {BODY_ID_NEPTUNE, 16, {801}, 1},
    };
    assert(satellite_catalog_for(BODY_ID_EARTH) == NULL);
    for (size_t k = 0; k < sizeof(expected) / sizeof(expected[0]); ++k) {
        const SatelliteCatalog *catalog = satellite_catalog_for(expected[k].planet);
        assert(catalog && catalog->count == expected[k].count);
        size_t majors = 0;
        for (size_t i = 0; i < catalog->count; ++i) {
            const SatelliteDefinition *d = &catalog->moons[i];
            assert(d->code != (int)expected[k].planet);
            for (size_t j = 0; j < i; ++j) assert(catalog->moons[j].code != d->code);
            if (!d->major) continue;
            ++majors;
            bool listed = false;
            for (size_t j = 0; j < expected[k].major_count; ++j) listed |= expected[k].majors[j] == d->code;
            /* Major moons are massive: they pull on the rest of the scene. */
            assert(listed && d->mass_quality == PHYSICAL_MEASURED && d->gm_km3_s2 > 0);
        }
        assert(majors == expected[k].major_count);
    }
}

/* A80: point-mass periods from JPL mean a and planet-only GM stay within 1%
 * of the JPL mean period. The gap is the omitted oblateness (J2), resonances
 * and solar perturbation: model error, not integrator error. */
static void test_point_mass_periods_stay_within_one_percent_of_jpl(void)
{
    const BodyId planets[] = {BODY_ID_JUPITER, BODY_ID_SATURN, BODY_ID_URANUS, BODY_ID_NEPTUNE};
    double worst = 0;
    for (size_t k = 0; k < 4; ++k) {
        const SatelliteCatalog *catalog = satellite_catalog_for(planets[k]);
        Body planet = parent_planet(planets[k]);
        for (size_t i = 0; i < catalog->count; ++i) {
            const SatelliteDefinition *d = &catalog->moons[i];
            double a = d->a_km * 1000, mu = SOLAR_G * planet.mass_kg + d->gm_km3_s2 * 1e9;
            double period_days = 2 * acos(-1.0) * sqrt(a * a * a / mu) / SOLAR_DAY_SECONDS;
            assert(d->period_days > 0);
            worst = fmax(worst, fabs(period_days / d->period_days - 1));
        }
    }
    /* Today's worst case is Europa at 0.75%; better data may only shrink it. */
    assert(worst < 0.01);
}

/* A79: every reference plane converts into the right orientation. */
static void test_reference_planes_orient_regular_moons(void)
{
    /* JPL's pole-less Uranian "equatorial" rows use the spin pole, which is
     * the antipode of the IAU north pole kept in constants.h. */
    const SatelliteCatalog *uranian = satellite_catalog_for(BODY_ID_URANUS);
    size_t equatorial = 0;
    for (size_t i = 0; i < uranian->count; ++i) {
        const SatelliteDefinition *d = &uranian->moons[i];
        if (d->frame != SATELLITE_FRAME_EQUATORIAL) continue;
        ++equatorial;
        assert(d->major);
        assert(fabs(d->pole_ra_deg - (SOLAR_URANUS_IAU_POLE_RA_DEG + 180 - 360)) < 1e-9);
        assert(fabs(d->pole_dec_deg + SOLAR_URANUS_IAU_POLE_DEC_DEG) < 1e-9);
    }
    assert(equatorial == 5);

    /* Each converted orbit normal sits exactly its source inclination away
     * from its source plane's pole, whatever the frame. */
    const BodyId planets[] = {BODY_ID_JUPITER, BODY_ID_SATURN, BODY_ID_URANUS, BODY_ID_NEPTUNE};
    for (size_t k = 0; k < 4; ++k) {
        const SatelliteCatalog *catalog = satellite_catalog_for(planets[k]);
        Body planet = parent_planet(planets[k]);
        for (size_t i = 0; i < catalog->count; ++i) {
            const SatelliteDefinition *d = &catalog->moons[i];
            Vec3d pole = d->frame == SATELLITE_FRAME_ECLIPTIC ? (Vec3d){0, 1, 0} : icrf_direction(d->pole_ra_deg, d->pole_dec_deg);
            assert(fabs(angle_degrees(orbit_normal(d, &planet), pole) - d->inclination_deg) < 1e-6);
        }
    }

    /* Different ephemeris solutions of one planet describe one equatorial
     * system: Uranus's major moons (URA182, equatorial frame) and inner moons
     * (URA184, Laplace frame) share a plane, Saturn's inner and major moons
     * do too, and Uranus's regular moons orbit about the IAU pole's antipode
     * because the planet spins retrograde. */
    Body uranus = parent_planet(BODY_ID_URANUS), saturn = parent_planet(BODY_ID_SATURN);
    Vec3d spin = icrf_direction(SOLAR_URANUS_IAU_POLE_RA_DEG + 180, -SOLAR_URANUS_IAU_POLE_DEC_DEG);
    assert(angle_degrees(orbit_normal(find_moon(uranian, 701), &uranus), spin) < 0.5);
    assert(angle_degrees(orbit_normal(find_moon(uranian, 701), &uranus), orbit_normal(find_moon(uranian, 706), &uranus)) < 1);
    assert(angle_degrees(orbit_normal(find_moon(uranian, 715), &uranus), spin) < 1.5);
    const SatelliteCatalog *saturnian = satellite_catalog_for(BODY_ID_SATURN);
    assert(angle_degrees(orbit_normal(find_moon(saturnian, 606), &saturn), orbit_normal(find_moon(saturnian, 602), &saturn)) < 1);
    assert(angle_degrees(orbit_normal(find_moon(saturnian, 618), &saturn), orbit_normal(find_moon(saturnian, 602), &saturn)) < 1);

    /* Triton is the one large retrograde moon: its orbit opposes Neptune's
     * regular inner moons (Proteus here) by more than 150 degrees. */
    const SatelliteCatalog *neptunian = satellite_catalog_for(BODY_ID_NEPTUNE);
    Body neptune = parent_planet(BODY_ID_NEPTUNE);
    assert(angle_degrees(orbit_normal(find_moon(neptunian, 801), &neptune), orbit_normal(find_moon(neptunian, 808), &neptune)) > 150);
}

int main(void)
{
    test_ecliptic_frame_is_a_proper_rotation_with_prograde_plus_y();
    test_orbital_elements_preserve_geometry_and_parent_motion();
    test_shared_conic_solver_reproduces_former_jovian_states();
    test_complete_jovian_catalog_and_initial_orbits();
    test_giant_planet_catalogs_inventory_and_major_moons();
    test_point_mass_periods_stay_within_one_percent_of_jpl();
    test_reference_planes_orient_regular_moons();
    puts("test_satellites passed");
}

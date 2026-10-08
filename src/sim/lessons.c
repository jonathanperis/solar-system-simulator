#include "lessons.h"
#include "constants.h"
#include "physics.h"
#include "diagnostics.h"
#include "orbit.h"
#include "collisions.h"

const char *lesson_name(LessonPreset preset)
{
    const char *names[] = {"core", "circular", "eccentric", "escape", "earth-moon", "inclined", "phobos",
        "barycentric-core", "resonance", "encounter", "collision",
        "jupiter-system", "saturn-system", "uranus-system", "neptune-system", "pluto-system", "didymos-system",
        "pluto-charon", "dart"};
    _Static_assert(sizeof(names) / sizeof(names[0]) == LESSON_COUNT, "every preset needs a name");
    return preset >= 0 && preset < LESSON_COUNT ? names[preset] : "catalog";
}

BodyId lesson_family_planet(LessonPreset preset)
{
    switch (preset) {
        case LESSON_JUPITER_SYSTEM: return BODY_ID_JUPITER;
        case LESSON_SATURN_SYSTEM: return BODY_ID_SATURN;
        case LESSON_URANUS_SYSTEM: return BODY_ID_URANUS;
        case LESSON_NEPTUNE_SYSTEM: return BODY_ID_NEPTUNE;
        case LESSON_PLUTO_SYSTEM: return BODY_ID_PLUTO;
        case LESSON_DIDYMOS_SYSTEM: return BODY_ID_DIDYMOS;
        default: return BODY_ID_NONE;
    }
}

bool lesson_is_scene(LessonPreset preset)
{
    return preset == LESSON_CORE || lesson_family_planet(preset) != BODY_ID_NONE;
}

bool lesson_starts_at_epoch(LessonPreset preset)
{
    return lesson_is_scene(preset) || preset == LESSON_BARYCENTRIC_CORE;
}

size_t lesson_subject_index(LessonPreset preset)
{
    /* A family scene opens on its planet. */
    if (lesson_family_planet(preset) != BODY_ID_NONE) return (size_t)solar_system_family_planet_index(lesson_family_planet(preset));
    if (preset == LESSON_CORE || preset == LESSON_BARYCENTRIC_CORE) return 0;
    return preset == LESSON_RESONANCE || preset == LESSON_ENCOUNTER ? 2 : 1;
}

double lesson_default_step(LessonPreset preset) { return preset == LESSON_COLLISION ? .1 : 15; }

#define LESSON_FACTOR_FLOOR 0.1
#define LESSON_FACTOR_CEILING 2.0
/* Bisection halves [0.1, 2] each pass: 30 passes leave an interval of about
 * 2e-9, far below the 0.01 rounding of the published limit. */
#define LESSON_FACTOR_BISECTIONS 30

bool lesson_monitors_contact(LessonPreset preset)
{
    if (preset == LESSON_CATALOG) return true;
    return preset >= 0 && preset < LESSON_COUNT && !lesson_is_scene(preset) && preset != LESSON_BARYCENTRIC_CORE &&
        preset != LESSON_COLLISION;
}

/* Builds a lesson without range checks; lesson_create() validates first. */
static void build_lesson(LessonPreset preset, double velocity_factor, SolarSystem *result)
{
    SolarSystem system = {0};
    if (lesson_family_planet(preset) != BODY_ID_NONE) {
        (void)solar_system_create_family(lesson_family_planet(preset), &system);
    } else if (preset == LESSON_CORE || preset == LESSON_BARYCENTRIC_CORE) {
        system = solar_system_create_current();
        if (preset == LESSON_BARYCENTRIC_CORE) {
            PhysicsDiagnostics d = physics_diagnostics(&system);
            Vec3d velocity = vec3d_scale(d.momentum_kg_mps, 1 / d.total_mass_kg);
            for (size_t i = 0; i < system.body_count; ++i) {
                system.bodies[i].fixed = false;
                system.bodies[i].position_m = vec3d_sub(system.bodies[i].position_m, d.center_of_mass_m);
                system.bodies[i].velocity_mps = vec3d_sub(system.bodies[i].velocity_mps, velocity);
            }
        }
    } else if (preset == LESSON_COLLISION) {
        /* These are chosen classroom spheres, not astronomical measurements.
         * Their head-on geometry has no unmodeled internal angular momentum. */
        for (size_t i = 0; i < 2; ++i) {
            double sign = i == 0 ? -1 : 1;
            system.bodies[i] = body_create_identified(i == 0 ? "Collider A" : "Collider B", BODY_KIND_ASTEROID,
                (BodyId)(1000001 + i), BODY_ID_NONE, 10, 10, (Vec3d){sign * 100, 0, 0},
                (Vec3d){-sign * 10 * velocity_factor, 0, 0}, false);
            system.bodies[i].mass_quality = system.bodies[i].radius_quality = PHYSICAL_ESTIMATED;
            system.bodies[i].group = "Chosen classroom spheres";
        }
        system.body_count = 2;
    } else if (preset == LESSON_RESONANCE || preset == LESSON_ENCOUNTER) {
        system = solar_system_create_sun_only();
        Body planet = preset == LESSON_RESONANCE ? solar_system_create_jupiter_at_perihelion() : solar_system_create_earth_at_perihelion();
        double radius = preset == LESSON_RESONANCE ? SOLAR_JUPITER_SEMI_MAJOR_AXIS_M : SOLAR_AU_METERS;
        double theta = preset == LESSON_RESONANCE ? acos(-1.0) / 3 : 0;
        double speed = sqrt(SOLAR_G * SOLAR_SUN_MASS_KG / radius);
        /* Lesson states are written in ecliptic axes (prograde = counterclockwise
         * from north) and mapped once, exactly like the core scene. */
        planet.position_m = orbit_ecliptic_to_simulation((Vec3d){radius * cos(theta), radius * sin(theta), 0});
        planet.velocity_mps = orbit_ecliptic_to_simulation((Vec3d){-speed * sin(theta), speed * cos(theta), 0});
        Body probe = body_create_identified(preset == LESSON_RESONANCE ? "3:2 test particle" : "Encounter probe",
            BODY_KIND_ASTEROID, (BodyId)1000001, preset == LESSON_RESONANCE ? BODY_ID_SUN : BODY_ID_EARTH,
            0, 0, vec3d_zero(), vec3d_zero(), false);
        probe.mass_quality = probe.radius_quality = PHYSICAL_UNKNOWN;
        probe.group = "Controlled test particles";
        if (preset == LESSON_RESONANCE) {
            double a = radius * pow(2.0 / 3.0, 2.0 / 3.0), q = a * .9;
            probe.position_m = orbit_ecliptic_to_simulation((Vec3d){q, 0, 0});
            probe.velocity_mps = orbit_ecliptic_to_simulation(
                (Vec3d){0, velocity_factor * sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * (2 / q - 1 / a)), 0});
        } else {
            /* Start 40 Earth radii behind Earth along its motion and 4 radii
             * farther from the Sun (Earth sits on ecliptic +X), then overtake
             * it at 10 km/s times the factor. */
            probe.position_m = vec3d_add(planet.position_m,
                orbit_ecliptic_to_simulation((Vec3d){4 * SOLAR_EARTH_RADIUS_M, -40 * SOLAR_EARTH_RADIUS_M, 0}));
            probe.velocity_mps = vec3d_add(planet.velocity_mps,
                orbit_ecliptic_to_simulation((Vec3d){0, 10000 * velocity_factor, 0}));
        }
        system.bodies[1] = planet; system.bodies[2] = probe; system.body_count = 3;
    } else if (preset == LESSON_PLUTO_CHARON || preset == LESSON_DART) {
        /* An isolated binary at its barycenter, as in the Earth-Moon lesson.
         * Pluto-Charon: Charon's catalog orbit (12% of Pluto's mass puts the
         * barycenter outside Pluto). DART: Dimorphos on its pre-impact orbit,
         * JPL Horizons s547 osculating ecliptic elements about the Didymos
         * primary at 2022-09-01 TDB, started at periapsis; the speed factor
         * is the along-track change, and 0.985 reproduces DART's ~2.6 mm/s
         * slowdown (two-body period 12.34 h -> ~11.8 h, as observed: ~33 min). */
        static const SatelliteDefinition dimorphos_pre_impact = {
            .code = BODY_ID_DIMORPHOS, .name = "Dimorphos", .group = "Didymos system",
            .a_km = 1.209922053993790, .eccentricity = 0.01711717899757169, .periapsis_deg = 73.55398047490951,
            .mean_anomaly_deg = 0.0, .inclination_deg = 170.7245506300443, .node_deg = 39.81034174807041,
            .gm_km3_s2 = 3.0268e-10, .radius_km = 0.0746, .period_days = 44418.72204856695 / 86400.0,
            .frame = SATELLITE_FRAME_ECLIPTIC, .mass_quality = PHYSICAL_ESTIMATED, .radius_quality = PHYSICAL_ESTIMATED,
        };
        Body parent = preset == LESSON_PLUTO_CHARON ? solar_system_create_pluto_at_perihelion() : solar_system_create_didymos_at_perihelion();
        parent.position_m = parent.velocity_mps = vec3d_zero();
        parent.parent_id = BODY_ID_NONE;
        Body moon = satellite_create(preset == LESSON_PLUTO_CHARON ? &satellite_catalog_for(BODY_ID_PLUTO)->moons[0]
            : &dimorphos_pre_impact, &parent);
        moon.velocity_mps = vec3d_scale(moon.velocity_mps, velocity_factor);
        double fraction = moon.mass_kg / (parent.mass_kg + moon.mass_kg);
        Vec3d center = vec3d_scale(moon.position_m, fraction);
        Vec3d velocity = vec3d_scale(moon.velocity_mps, fraction);
        parent.position_m = vec3d_scale(center, -1);
        parent.velocity_mps = vec3d_scale(velocity, -1);
        moon.position_m = vec3d_sub(moon.position_m, center);
        moon.velocity_mps = vec3d_sub(moon.velocity_mps, velocity);
        system.bodies[0] = parent;
        system.bodies[1] = moon;
        system.body_count = 2;
    } else if (preset == LESSON_EARTH_MOON || preset == LESSON_PHOBOS) {
        Body parent = preset == LESSON_EARTH_MOON ? solar_system_create_earth_at_perihelion() : solar_system_create_mars_at_perihelion();
        parent.position_m = parent.velocity_mps = vec3d_zero();
        parent.parent_id = BODY_ID_NONE;
        Body moon = preset == LESSON_EARTH_MOON ? solar_system_create_moon_at_perigee_near_earth(&parent)
                                               : solar_system_create_phobos_at_periareion_near_mars(&parent);
        moon.velocity_mps = vec3d_scale(moon.velocity_mps, velocity_factor);
        /* Move the free pair to its barycenter while preserving relative state.
         * Both bodies move, so linear momentum has no fixed-origin constraint. */
        double fraction = moon.mass_kg / (parent.mass_kg + moon.mass_kg);
        Vec3d center = vec3d_scale(moon.position_m, fraction);
        Vec3d velocity = vec3d_scale(moon.velocity_mps, fraction);
        parent.position_m = vec3d_scale(center, -1);
        parent.velocity_mps = vec3d_scale(velocity, -1);
        moon.position_m = vec3d_sub(moon.position_m, center);
        moon.velocity_mps = vec3d_sub(moon.velocity_mps, velocity);
        system.bodies[0] = parent;
        system.bodies[1] = moon;
        system.body_count = 2;
    } else {
        system = solar_system_create_sun_only();
        Body planet = solar_system_create_earth_at_perihelion();
        double eccentricity = preset == LESSON_ECCENTRIC ? 0.5 : 0.0;
        double distance = SOLAR_AU_METERS * (1.0 - eccentricity);
        double speed = sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * (2.0 / distance - 1.0 / SOLAR_AU_METERS));
        if (preset == LESSON_ESCAPE) speed = sqrt(2.0 * SOLAR_G * SOLAR_SUN_MASS_KG / distance);
        double inclination = preset == LESSON_INCLINED ? acos(-1.0) / 6.0 : 0.0;
        /* Ecliptic in-plane velocity tilted toward north by the inclination. */
        planet.position_m = orbit_ecliptic_to_simulation((Vec3d){distance, 0, 0});
        planet.velocity_mps = orbit_ecliptic_to_simulation((Vec3d){0, speed * velocity_factor * cos(inclination),
            speed * velocity_factor * sin(inclination)});
        system.bodies[1] = planet;
        system.body_count = 2;
    }
    physics_compute_accelerations(system.bodies, system.body_count);
    *result = system;
}

/* True if the subject's two-body conic around its parent stays outside contact. */
static bool lesson_orbit_clears_parent(LessonPreset preset, double velocity_factor)
{
    SolarSystem system;
    build_lesson(preset, velocity_factor, &system);
    size_t subject = lesson_subject_index(preset);
    int parent = solar_system_parent_index(&system, subject);
    if (parent < 0) return true;
    const Body *body = &system.bodies[subject], *center = &system.bodies[parent];
    /* A fixed parent never accelerates, so only its own mass pulls; a free
     * pair orbits with the combined gravitational parameter G (M + m). */
    double mu = SOLAR_G * (center->mass_kg + (center->fixed ? 0 : body->mass_kg));
    double closest = orbit_closest_approach_m(vec3d_sub(body->position_m, center->position_m),
        vec3d_sub(body->velocity_mps, center->velocity_mps), mu);
    return closest >= center->radius_m + body->radius_m;
}

static double compute_minimum_velocity_factor(LessonPreset preset)
{
    if (lesson_is_scene(preset) || preset == LESSON_BARYCENTRIC_CORE) return 1.0;
    if (!lesson_monitors_contact(preset) || lesson_orbit_clears_parent(preset, LESSON_FACTOR_FLOOR)) return LESSON_FACTOR_FLOOR;
    /* No allowed factor clears: the lesson would be unusable. Say so with NaN
     * (every validator rejects it) instead of quietly publishing the ceiling. */
    if (!lesson_orbit_clears_parent(preset, LESSON_FACTOR_CEILING)) return NAN;
    /* Slower starts lower the periapsis monotonically, so bisect between a
     * failing and a clearing factor, then round up to whole hundredths so the
     * published limit is a readable slider value that still clears. */
    double low = LESSON_FACTOR_FLOOR, high = LESSON_FACTOR_CEILING;
    for (int i = 0; i < LESSON_FACTOR_BISECTIONS; ++i) {
        double middle = 0.5 * (low + high);
        if (lesson_orbit_clears_parent(preset, middle)) high = middle; else low = middle;
    }
    double rounded = fmin(LESSON_FACTOR_CEILING, ceil(high * 100.0 - 1e-9) / 100.0);
    return lesson_orbit_clears_parent(preset, rounded) ? rounded : high;
}

double lesson_minimum_velocity_factor(LessonPreset preset)
{
    if (preset < 0 || preset >= LESSON_COUNT) return 1.0;
    /* Lesson initial states are fixed, so each limit is computed once and
     * reused (the web runtime asks every frame). Zero marks "not yet computed". */
    static double cached[LESSON_COUNT];
    if (cached[preset] == 0.0) cached[preset] = compute_minimum_velocity_factor(preset);
    return cached[preset];
}

bool lesson_create(LessonPreset preset, double velocity_factor, SolarSystem *result)
{
    /* Written as !(x >= minimum) so a NaN minimum (no clearing factor) rejects. */
    if (preset < 0 || preset >= LESSON_COUNT || !isfinite(velocity_factor) ||
        !(velocity_factor >= lesson_minimum_velocity_factor(preset)) || velocity_factor > LESSON_FACTOR_CEILING)
        return false;
    if ((lesson_is_scene(preset) || preset == LESSON_BARYCENTRIC_CORE) && velocity_factor != 1.0) return false;
    build_lesson(preset, velocity_factor, result);
    return true;
}

/* Longitude in the ecliptic plane, counterclockwise from +X seen from north.
 * Simulation -Z is ecliptic +Y (see orbit_ecliptic_to_simulation). */
static double ecliptic_longitude(Vec3d v)
{
    return atan2(-v.z, v.x);
}

static double mean_longitude(Vec3d r, Vec3d v, double mu, double *periapsis)
{
    double distance = vec3d_length(r);
    Vec3d evec = vec3d_sub(vec3d_scale(vec3d_cross(v, vec3d_cross(r, v)), 1 / mu), vec3d_scale(r, 1 / distance));
    double e = vec3d_length(evec);
    *periapsis = e > 1e-10 ? ecliptic_longitude(evec) : 0;
    if (e >= 1) return NAN;
    double anomaly = ecliptic_longitude(r) - *periapsis;
    double eccentric_anomaly = atan2(sqrt(1 - e * e) * sin(anomaly), e + cos(anomaly));
    return eccentric_anomaly - e * sin(eccentric_anomaly) + *periapsis;
}

double lesson_resonant_angle_degrees(const SolarSystem *system)
{
    if (system->body_count != 3 || system->bodies[1].id != BODY_ID_JUPITER) return NAN;
    Vec3d origin = system->bodies[0].position_m, velocity = system->bodies[0].velocity_mps;
    double mu = SOLAR_G * system->bodies[0].mass_kg, periapsis, unused;
    double jupiter = mean_longitude(vec3d_sub(system->bodies[1].position_m, origin), vec3d_sub(system->bodies[1].velocity_mps, velocity), mu, &unused);
    double particle = mean_longitude(vec3d_sub(system->bodies[2].position_m, origin), vec3d_sub(system->bodies[2].velocity_mps, velocity), mu, &periapsis);
    double angle = 3 * jupiter - 2 * particle - periapsis;
    return atan2(sin(angle), cos(angle)) * 180 / acos(-1.0);
}

bool lesson_reference_position(const SolarSystem *initial, size_t subject, double seconds, Vec3d *relative)
{
    if (initial->body_count != 2 || subject != 1 || initial->bodies[1].parent_id != initial->bodies[0].id) return false;
    const Body *parent = &initial->bodies[0], *body = &initial->bodies[1];
    Vec3d r = vec3d_sub(body->position_m, parent->position_m), v = vec3d_sub(body->velocity_mps, parent->velocity_mps);
    /* Every two-body lesson starts at an apsis (velocity perpendicular to the
     * radius), so the start is periapsis or apoapsis and the apsis geometry
     * below applies. Refuse any other start rather than publish a wrong reference. */
    if (fabs(vec3d_dot(r, v)) > 1e-9 * vec3d_length(r) * vec3d_length(v)) return false;
    double distance = vec3d_length(r), mu = SOLAR_G * (parent->mass_kg + (parent->fixed ? 0 : body->mass_kg));
    double ratio = vec3d_length_squared(v) * distance / mu;
    double e = fabs(ratio - 1), q = distance, offset = 0;
    Vec3d p = vec3d_scale(r, 1 / distance), h = vec3d_cross(r, v);
    if (e < 1e-12) e = 0;
    if (ratio < 1 - 1e-12) {
        double a = distance / (1 + e);
        q = a * (1 - e); offset = acos(-1.0) * sqrt(a * a * a / mu);
        p = vec3d_scale(p, -1);
    }
    Vec3d tangent = vec3d_cross(vec3d_scale(h, 1 / vec3d_length(h)), p);
    OrbitState state;
    if (!orbit_state(q, e, mu, seconds + offset, &state)) return false;
    *relative = vec3d_add(vec3d_scale(p, state.position_m.x), vec3d_scale(tangent, state.position_m.y));
    return true;
}

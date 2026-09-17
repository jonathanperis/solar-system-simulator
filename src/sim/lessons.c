#include "lessons.h"
#include "constants.h"
#include "physics.h"
#include "diagnostics.h"
#include "orbit.h"

const char *lesson_name(LessonPreset preset)
{
    const char *names[] = {"core", "circular", "eccentric", "escape", "earth-moon", "inclined", "phobos",
        "barycentric-core", "resonance", "encounter", "collision"};
    return preset >= 0 && preset < LESSON_COUNT ? names[preset] : "catalog";
}

size_t lesson_subject_index(LessonPreset preset)
{
    if (preset == LESSON_CORE || preset == LESSON_BARYCENTRIC_CORE) return 0;
    return preset == LESSON_RESONANCE || preset == LESSON_ENCOUNTER ? 2 : 1;
}

double lesson_default_step(LessonPreset preset) { return preset == LESSON_COLLISION ? .1 : 15; }

bool lesson_create(LessonPreset preset, double velocity_factor, SolarSystem *result)
{
    if (preset < 0 || preset >= LESSON_COUNT || !isfinite(velocity_factor) || velocity_factor < 0.1 || velocity_factor > 2.0)
        return false;
    if ((preset == LESSON_CORE || preset == LESSON_BARYCENTRIC_CORE) && velocity_factor != 1.0) return false;
    SolarSystem system = {0};
    if (preset == LESSON_CORE || preset == LESSON_BARYCENTRIC_CORE) {
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
        planet.position_m = (Vec3d){radius * cos(theta), 0, radius * sin(theta)};
        planet.velocity_mps = (Vec3d){-speed * sin(theta), 0, speed * cos(theta)};
        Body probe = body_create_identified(preset == LESSON_RESONANCE ? "3:2 test particle" : "Encounter probe",
            BODY_KIND_ASTEROID, (BodyId)1000001, preset == LESSON_RESONANCE ? BODY_ID_SUN : BODY_ID_EARTH,
            0, 0, vec3d_zero(), vec3d_zero(), false);
        probe.mass_quality = probe.radius_quality = PHYSICAL_UNKNOWN;
        probe.group = "Controlled test particles";
        if (preset == LESSON_RESONANCE) {
            double a = radius * pow(2.0 / 3.0, 2.0 / 3.0), q = a * .9;
            probe.position_m = (Vec3d){q, 0, 0};
            probe.velocity_mps = (Vec3d){0, 0, velocity_factor * sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * (2 / q - 1 / a))};
        } else {
            probe.position_m = vec3d_add(planet.position_m, (Vec3d){4 * SOLAR_EARTH_RADIUS_M, 0, -40 * SOLAR_EARTH_RADIUS_M});
            probe.velocity_mps = vec3d_add(planet.velocity_mps, (Vec3d){0, 0, 10000 * velocity_factor});
        }
        system.bodies[1] = planet; system.bodies[2] = probe; system.body_count = 3;
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
        planet.position_m = (Vec3d){distance, 0, 0};
        planet.velocity_mps = (Vec3d){0, speed * velocity_factor * sin(inclination), speed * velocity_factor * cos(inclination)};
        system.bodies[1] = planet;
        system.body_count = 2;
    }
    physics_compute_accelerations(system.bodies, system.body_count);
    *result = system;
    return true;
}

static double mean_longitude(Vec3d r, Vec3d v, double mu, double *periapsis)
{
    double distance = vec3d_length(r);
    Vec3d evec = vec3d_sub(vec3d_scale(vec3d_cross(v, vec3d_cross(r, v)), 1 / mu), vec3d_scale(r, 1 / distance));
    double e = vec3d_length(evec);
    *periapsis = e > 1e-10 ? atan2(evec.z, evec.x) : 0;
    if (e >= 1) return NAN;
    double anomaly = atan2(r.z, r.x) - *periapsis;
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
    *relative = vec3d_add(vec3d_scale(p, state.position_m.x), vec3d_scale(tangent, state.position_m.z));
    return true;
}

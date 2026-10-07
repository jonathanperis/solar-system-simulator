#include "satellite.h"

#include <math.h>

#include "constants.h"
#include "orbit.h"

/* J2000 mean obliquity of the ecliptic, 23 deg 26' 21.448". */
#define SOLAR_J2000_OBLIQUITY_DEG 23.439291111

static Vec3d orbital_to_simulation(Vec3d perifocal, const SatelliteDefinition *d)
{
    /* Angles are measured in the satellite's reference frame: the J2000
     * ecliptic, or a plane given by its pole: a planet's Laplace plane for
     * regular moons, or Uranus's equator for its major moons. Uranus spins
     * retrograde, so that equatorial pole is the antipode of its IAU north
     * pole; its regular moons then have small, prograde inclinations. */
    Vec3d v = orbit_rotate_to_reference(perifocal, d->inclination_deg, d->node_deg, d->periapsis_deg);
    if (d->frame != SATELLITE_FRAME_ECLIPTIC) {
        /* JPL measures node from the plane's ascending node on the
         * ICRF equator. A plane with pole (RA, Dec) is inclined 90 - Dec to
         * the equator with ascending node at RA + 90 degrees, so the same 3-1-3
         * rotation takes Laplace coordinates to ICRF. Rotating about the shared
         * X axis by minus the obliquity then gives J2000 ecliptic coordinates. */
        v = orbit_rotate_to_reference(v, 90.0 - d->pole_dec_deg, d->pole_ra_deg + 90.0, 0.0);
        v = orbit_rotate_to_reference(v, -SOLAR_J2000_OBLIQUITY_DEG, 0.0, 0.0);
    }
    return orbit_ecliptic_to_simulation(v);
}

Body satellite_create(const SatelliteDefinition *d, const Body *parent)
{
    const double a = d->a_km * 1000.0;
    const double e = d->eccentricity;
    double mass = d->gm_km3_s2 * 1e9 / SOLAR_G;
    double mu = SOLAR_G * (parent->mass_kg + mass);
    /* Mean anomaly grows at the mean motion n = sqrt(mu / a^3), so M / n is the
     * time since periapsis. orbit.c then solves Kepler's equation for any
     * eccentricity; this file only orients the result. */
    double mean_motion = sqrt(mu / (a * a * a));
    double seconds_from_periapsis = d->mean_anomaly_deg * acos(-1.0) / 180.0 / mean_motion;
    OrbitState state;
    if (!orbit_state(a * (1.0 - e), e, mu, seconds_from_periapsis, &state)) {
        /* Generated catalog checks reject invalid elements; keep a finite
         * parent-coincident state rather than propagating NaN if one slips by. */
        state = (OrbitState){vec3d_zero(), vec3d_zero()};
    }
    Body body = body_create_identified(d->name, BODY_KIND_MOON, (BodyId)d->code, parent->id,
        mass, d->radius_km * 1000.0,
        vec3d_add(parent->position_m, orbital_to_simulation(state.position_m, d)),
        vec3d_add(parent->velocity_mps, orbital_to_simulation(state.velocity_mps, d)), false);
    body.mass_quality = d->mass_quality;
    body.radius_quality = d->radius_quality;
    body.group = d->group;
    return body;
}

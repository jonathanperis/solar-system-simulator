#include "satellite.h"

#include <math.h>

#include "constants.h"

static Vec3d rotate_x(Vec3d v, double a)
{
    return (Vec3d){v.x, cos(a) * v.y - sin(a) * v.z, sin(a) * v.y + cos(a) * v.z};
}

static Vec3d rotate_z(Vec3d v, double a)
{
    return (Vec3d){cos(a) * v.x - sin(a) * v.y, sin(a) * v.x + cos(a) * v.y, v.z};
}

static Vec3d orbital_to_simulation(Vec3d v, const SatelliteDefinition *d)
{
    const double radians = acos(-1.0) / 180.0;
    v = rotate_z(v, d->periapsis_deg * radians);
    v = rotate_x(v, d->inclination_deg * radians);
    v = rotate_z(v, d->node_deg * radians);
    if (d->frame == SATELLITE_FRAME_LAPLACE) {
        /* JPL measures node from the Laplace plane's ascending node on the
         * ICRF equator. Its longitude is pole RA + 90 degrees. Convert to ICRF,
         * then to J2000 ecliptic with the standard mean obliquity. */
        v = rotate_x(v, (90.0 - d->pole_dec_deg) * radians);
        v = rotate_z(v, (d->pole_ra_deg + 90.0) * radians);
        v = rotate_x(v, -23.439291111 * radians);
    }
    /* Existing prograde orbits move +X toward +Z; Y is ecliptic north. This
     * axis permutation changes handedness, not the physical orbital direction. */
    return (Vec3d){v.x, v.z, v.y};
}

Body satellite_create(const SatelliteDefinition *d, const Body *parent)
{
    const double a = d->a_km * 1000.0;
    const double e = d->eccentricity;
    const double m = d->mean_anomaly_deg * acos(-1.0) / 180.0;
    double eccentric_anomaly = m;
    /* Solve E - e*sin(E) = M. All snapshot eccentricities are below 0.5;
     * twelve Newton iterations comfortably converge at double precision. */
    for (int i = 0; i < 12; ++i) {
        eccentric_anomaly -= (eccentric_anomaly - e * sin(eccentric_anomaly) - m)
            / (1.0 - e * cos(eccentric_anomaly));
    }
    double mass = d->gm_km3_s2 * 1e9 / SOLAR_G;
    double mu = SOLAR_G * (parent->mass_kg + mass);
    double s = sin(eccentric_anomaly), c = cos(eccentric_anomaly);
    double minor = sqrt(1.0 - e * e);
    Vec3d r = {a * (c - e), a * minor * s, 0};
    double velocity_scale = sqrt(mu / a) / (1.0 - e * c);
    Vec3d v = {-velocity_scale * s, velocity_scale * minor * c, 0};
    Body body = body_create_identified(d->name, BODY_KIND_MOON, (BodyId)d->code, parent->id,
        mass, d->radius_km * 1000.0,
        vec3d_add(parent->position_m, orbital_to_simulation(r, d)),
        vec3d_add(parent->velocity_mps, orbital_to_simulation(v, d)), false);
    body.mass_quality = d->mass_quality;
    body.radius_quality = d->radius_quality;
    body.group = d->group;
    return body;
}

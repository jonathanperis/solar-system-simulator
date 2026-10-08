#include "orbit.h"
#include "constants.h"
#include <math.h>

static void stumpff(double z, double *c, double *s)
{
    if (fabs(z) < 1e-4) {
        *c = .5 - z/24 + z*z/720 - z*z*z/40320;
        *s = 1.0/6 - z/120 + z*z/5040 - z*z*z/362880;
    } else if (z > 0) {
        double x = sqrt(z);
        *c = 2*sin(x/2)*sin(x/2)/z;
        *s = (x-sin(x))/(x*x*x);
    } else {
        double x = sqrt(-z);
        *c = (cosh(x)-1)/(-z);
        *s = (sinh(x)-x)/(x*x*x);
    }
}

bool orbit_state(double q, double e, double mu, double dt, OrbitState *out)
{
    if (!isfinite(q) || q <= 0 || !isfinite(e) || e < 0 || !isfinite(mu) || mu <= 0 || !isfinite(dt)) return false;
    double alpha = (1-e)/q, root_mu = sqrt(mu);
    if (e < 1) {
        double period = 2*acos(-1.0)/sqrt(mu*alpha*alpha*alpha);
        dt = remainder(dt, period);
    }
    double sign = dt < 0 ? -1 : 1, target = root_mu*fabs(dt);
    double lo = 0, hi = sqrt(q), c, s;
    /* Universal Kepler equation at periapsis: e*chi^3*S + q*chi = sqrt(mu)*t.
     * Its positive derivative is radius. Bracketing makes high-e and e=1
     * cases converge without switching to fragile eccentric-anomaly guesses. */
    for (int i = 0; i < 128; ++i) {
        stumpff(alpha*hi*hi, &c, &s);
        if (e*hi*hi*hi*s + q*hi >= target) break;
        hi *= 2;
    }
    double chi = hi/2;
    for (int i = 0; i < 128; ++i) {
        stumpff(alpha*chi*chi, &c, &s);
        double f = e*chi*chi*chi*s + q*chi - target;
        if (f > 0) hi = chi; else lo = chi;
        double next = chi - f/(e*chi*chi*c + q);
        if (!isfinite(next) || next <= lo || next >= hi) next = (lo+hi)/2;
        if (fabs(f) <= 2e-15*fmax(target, q*sqrt(q))) break;
        chi = next;
    }
    chi *= sign;
    stumpff(alpha*chi*chi, &c, &s);
    /* Lagrange f/g form at periapsis, written in the perifocal frame: x along
     * the periapsis direction, y along the periapsis velocity (prograde). */
    double speed = sqrt(mu*(1+e)/q);
    Vec3d r = {q-chi*chi*c, (dt-chi*chi*chi*s/root_mu)*speed, 0};
    double distance = vec3d_length(r);
    Vec3d v = {root_mu/distance*(alpha*chi*chi*chi*s-chi), (1-chi*chi*c/distance)*speed, 0};
    if (!isfinite(distance) || distance <= 0 || !isfinite(v.x) || !isfinite(v.y)) return false;
    *out = (OrbitState){r, v};
    return true;
}

double orbit_closest_approach_m(Vec3d r, Vec3d v, double mu)
{
    /* Conic geometry: specific angular momentum h = r x v fixes the
     * semi-latus rectum p = |h|^2 / mu, and periapsis is q = p / (1 + e). The
     * eccentricity comes from the eccentricity vector e = (v x h) / mu - r/|r|
     * rather than sqrt(1 + 2 E h^2 / mu^2), whose 1 - 1 cancellation would
     * leave e ~ 1e-8 for a circle. A radial orbit (h = 0) reaches q = 0. */
    double distance = vec3d_length(r);
    if (distance <= 0) return 0.0;
    double energy = 0.5 * vec3d_length_squared(v) - mu / distance;
    if (energy >= 0 && vec3d_dot(r, v) >= 0) return distance; /* open and receding */
    Vec3d h = vec3d_cross(r, v);
    Vec3d e = vec3d_sub(vec3d_scale(vec3d_cross(v, h), 1.0 / mu), vec3d_scale(r, 1.0 / distance));
    return vec3d_length_squared(h) / (mu * (1.0 + vec3d_length(e)));
}

Vec3d orbit_rotate_to_reference(Vec3d v, double inclination, double node, double periapsis)
{
    /* R = Rz(node) * Rx(inclination) * Rz(periapsis), angles in degrees. */
    double k = acos(-1.0)/180, w = periapsis*k, i = inclination*k, n = node*k;
    double x = cos(w)*v.x - sin(w)*v.y, y = sin(w)*v.x + cos(w)*v.y;
    double y_tilted = cos(i)*y - sin(i)*v.z, z = sin(i)*y + cos(i)*v.z;
    return (Vec3d){cos(n)*x - sin(n)*y_tilted, sin(n)*x + cos(n)*y_tilted, z};
}

Vec3d orbit_ecliptic_to_simulation(Vec3d ecliptic)
{
    /* A -90 degree rotation about X: ecliptic north (+Z) becomes simulation
     * +Y, the "up" axis of the camera, and ecliptic +Y becomes simulation -Z.
     * It is a proper rotation (determinant +1), so prograde motion keeps
     * angular momentum toward north (+Y) and is drawn counterclockwise when
     * viewed from above. Swapping Y and Z instead would mirror the scene. */
    return (Vec3d){ecliptic.x, ecliptic.z, -ecliptic.y};
}

Vec3d orbit_orient(Vec3d v, double inclination, double node, double periapsis)
{
    return orbit_ecliptic_to_simulation(orbit_rotate_to_reference(v, inclination, node, periapsis));
}

double catalog_coordinate(double q, double e, double i, double n, double w, double tp, double epoch, int component)
{
    OrbitState state;
    if (!isfinite(i) || !isfinite(n) || !isfinite(w) || component < 0 || component > 5 ||
        !orbit_state(q*SOLAR_AU_METERS, e, SOLAR_G*SOLAR_SUN_MASS_KG, (epoch-tp)*SOLAR_DAY_SECONDS, &state)) return NAN;
    Vec3d v = orbit_orient(component < 3 ? state.position_m : state.velocity_mps, i, n, w);
    return component%3 == 0 ? v.x : component%3 == 1 ? v.y : v.z;
}

double catalog_period_days(double q, double e)
{
    if (!isfinite(q) || q <= 0 || !isfinite(e) || e < 0) return NAN;
    if (e >= 1) return 0;
    double a = q*SOLAR_AU_METERS/(1-e);
    return 2*acos(-1.0)*sqrt(a*a*a/(SOLAR_G*SOLAR_SUN_MASS_KG))/SOLAR_DAY_SECONDS;
}

Vec3d orbit_icrf_direction(double ra_deg, double dec_deg)
{
    const double degree = acos(-1.0) / 180.0;
    Vec3d equatorial = {cos(dec_deg * degree) * cos(ra_deg * degree), cos(dec_deg * degree) * sin(ra_deg * degree),
        sin(dec_deg * degree)};
    /* Rotating about the shared X axis by minus the J2000 obliquity
     * (23 deg 26' 21.448") takes equatorial coordinates to the ecliptic. */
    return orbit_ecliptic_to_simulation(orbit_rotate_to_reference(equatorial, -23.439291111, 0.0, 0.0));
}

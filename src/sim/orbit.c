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
    double speed = sqrt(mu*(1+e)/q);
    Vec3d r = {q-chi*chi*c, 0, (dt-chi*chi*chi*s/root_mu)*speed};
    double distance = vec3d_length(r);
    Vec3d v = {root_mu/distance*(alpha*chi*chi*chi*s-chi), 0, (1-chi*chi*c/distance)*speed};
    if (!isfinite(distance) || distance <= 0 || !isfinite(v.x) || !isfinite(v.z)) return false;
    *out = (OrbitState){r, v};
    return true;
}

Vec3d orbit_orient(Vec3d v, double inclination, double node, double periapsis)
{
    double k = acos(-1.0)/180, w = periapsis*k, i = inclination*k, n = node*k;
    double x = cos(w)*v.x-sin(w)*v.z, y = sin(w)*v.x+cos(w)*v.z;
    return (Vec3d){cos(n)*x-sin(n)*cos(i)*y, sin(i)*y, sin(n)*x+cos(n)*cos(i)*y};
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

#include "require_assert.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "sim/orbit.h"
#include "sim/constants.h"

int main(void)
{
    const double mu = SOLAR_G * SOLAR_SUN_MASS_KG;
    const double q = SOLAR_AU_METERS;
    const double eccentricities[] = {0, 0.6, 0.999999999, 1, 1.000000001, 1.2};
    for (size_t i = 0; i < sizeof(eccentricities) / sizeof(*eccentricities); ++i) {
        double e = eccentricities[i];
        OrbitState s;
        assert(orbit_state(q, e, mu, 0, &s));
        assert(fabs(s.position_m.x / q - 1) < 1e-13);
        assert(fabs(s.velocity_mps.y / sqrt(mu * (1 + e) / q) - 1) < 1e-13);
        assert(s.position_m.z == 0 && s.velocity_mps.z == 0);
        assert(orbit_state(q, e, mu, 100 * SOLAR_DAY_SECONDS, &s));
        double r = vec3d_length(s.position_m);
        double energy = vec3d_length_squared(s.velocity_mps) / 2 - mu / r;
        assert(fabs((energy - mu * (e - 1) / (2*q)) / (mu/q)) < 1e-10);
        double angular = s.position_m.x*s.velocity_mps.y - s.position_m.y*s.velocity_mps.x;
        assert(fabs(angular / sqrt(mu*q*(1+e)) - 1) < 1e-10);
        OrbitState backwards;
        assert(orbit_state(q, e, mu, -100 * SOLAR_DAY_SECONDS, &backwards));
        assert(fabs((s.position_m.x - backwards.position_m.x)/q) < 1e-11);
        assert(fabs((s.position_m.y + backwards.position_m.y)/q) < 1e-11);
    }
    /* orbit_state returns the perifocal frame: periapsis +x, prograde motion +y. */
    /* Independent circular and Barker solutions, not a second copy of the
     * universal-variable implementation. A parabolic D=1 reaches (0,2q). */
    OrbitState s;
    double quarter = acos(-1.0)/2 * sqrt(q*q*q/mu);
    assert(orbit_state(q, 0, mu, quarter, &s));
    assert(fabs(s.position_m.x/q) < 1e-12);
    assert(fabs(s.position_m.y/q - 1) < 1e-12);
    assert(orbit_state(q, 1, mu, sqrt(2*q*q*q/mu)*4/3, &s));
    assert(fabs(s.position_m.x/q) < 1e-12);
    assert(fabs(s.position_m.y/q - 2) < 1e-12);
    /* Independent eccentric-anomaly and hyperbolic-anomaly reference points. */
    double e=.6, a=q/(1-e), E=acos(e);
    assert(orbit_state(q,e,mu,(E-e*sin(E))*sqrt(a*a*a/mu),&s));
    assert(fabs(s.position_m.x/q) < 1e-12);
    assert(fabs(s.position_m.y/q-(1+e)) < 1e-12);
    Vec3d inclined=orbit_orient(s.position_m,90,0,0);
    assert(fabs(inclined.y/q-1.6)<1e-12 && fabs(inclined.z/q)<1e-12);
    e=1.2; a=q/(e-1); double H=1;
    assert(orbit_state(q,e,mu,(e*sinh(H)-H)*sqrt(a*a*a/mu),&s));
    assert(fabs((s.position_m.x-a*(e-cosh(H)))/q)<1e-12);
    assert(fabs((s.position_m.y-a*sqrt(e*e-1)*sinh(H))/q)<1e-12);
    /* The 3-1-3 rotation: periapsis about the normal, inclination about the
     * line of nodes, node about the reference pole. Cardinal angles give exact
     * oracles: i=90 lifts perifocal +y onto the reference pole (+z). */
    Vec3d r = orbit_rotate_to_reference((Vec3d){1, 0, 0}, 0, 90, 0);
    assert(fabs(r.x) < 1e-15 && fabs(r.y - 1) < 1e-15 && fabs(r.z) < 1e-15);
    r = orbit_rotate_to_reference((Vec3d){0, 1, 0}, 90, 0, 0);
    assert(fabs(r.x) < 1e-15 && fabs(r.y) < 1e-15 && fabs(r.z - 1) < 1e-15);
    r = orbit_rotate_to_reference((Vec3d){1, 0, 0}, 0, 0, 90);
    assert(fabs(r.x) < 1e-15 && fabs(r.y - 1) < 1e-15 && fabs(r.z) < 1e-15);
    assert(!orbit_state(0, .5, mu, 0, &s));
    assert(!orbit_state(q, NAN, mu, 0, &s));
    assert(!orbit_state(q, .5, mu, INFINITY, &s));
    puts("test_orbit passed");
}

#ifndef SOLAR_ORBIT_H
#define SOLAR_ORBIT_H
#include <stdbool.h>
#include "vec3d.h"
typedef struct OrbitState { Vec3d position_m, velocity_mps; } OrbitState;
/* The single conic (Kepler) propagator for every orbit in the simulator.
 * Output is in the perifocal frame: periapsis on +x, prograde velocity +y,
 * orbit normal +z (so z is always zero). SI units throughout; mu = G*M. */
bool orbit_state(double q_m, double e, double mu, double seconds_from_perihelion, OrbitState *out);
/* Classical 3-1-3 Euler rotation from the perifocal frame into the frame the
 * angles were measured in (degrees): periapsis argument about the orbit
 * normal, inclination about the line of nodes, node longitude about the
 * reference pole. The same rotation turns a reference plane given by its pole
 * into its parent frame (satellite.c uses it for Laplace planes). */
Vec3d orbit_rotate_to_reference(Vec3d v, double inclination, double node, double periapsis);
/* Smallest future distance from the focus on the two-body conic through the
 * relative state (r, v) with mu = G*(M + m): the periapsis, unless an open
 * (parabolic/hyperbolic) orbit is already receding. Meters. */
double orbit_closest_approach_m(Vec3d relative_position_m, Vec3d relative_velocity_mps, double mu);
/* J2000 ecliptic (X toward the equinox, Z toward ecliptic north) to simulation
 * axes: (x, y, z)_sim = (X, Z, -Y). Simulation +Y is ecliptic north. */
Vec3d orbit_ecliptic_to_simulation(Vec3d ecliptic);
/* Unit vector toward ICRF right ascension/declination (degrees), rotated by
 * the J2000 obliquity into the ecliptic and then into simulation axes. */
Vec3d orbit_icrf_direction(double ra_deg, double dec_deg);
/* Perifocal vector with J2000 ecliptic angles (degrees) to simulation axes. */
Vec3d orbit_orient(Vec3d v, double inclination, double node, double periapsis);
/* Compact FFI for Python snapshot generation and the WASM atlas. Coordinates
 * 0..2 are position meters, 3..5 velocity m/s. Source time is Julian day TDB. */
double catalog_coordinate(double q_au, double e, double inclination, double node,
    double periapsis, double tp_jd, double epoch_jd, int component);
double catalog_period_days(double q_au, double eccentricity);
#endif

#ifndef SOLAR_ORBIT_H
#define SOLAR_ORBIT_H
#include <stdbool.h>
#include "vec3d.h"
typedef struct OrbitState { Vec3d position_m, velocity_mps; } OrbitState;
/* Perihelion on +X, prograde velocity +Z; SI units throughout. */
bool orbit_state(double q_m, double e, double mu, double seconds_from_perihelion, OrbitState *out);
/* Source angles are degrees in J2000 ecliptic; output uses Y-north simulation axes. */
Vec3d orbit_orient(Vec3d v, double inclination, double node, double periapsis);
/* Compact FFI for Python snapshot generation and the WASM atlas. Coordinates
 * 0..2 are position meters, 3..5 velocity m/s. Source time is Julian day TDB. */
double catalog_coordinate(double q_au, double e, double inclination, double node,
    double periapsis, double tp_jd, double epoch_jd, int component);
double catalog_period_days(double q_au, double eccentricity);
#endif

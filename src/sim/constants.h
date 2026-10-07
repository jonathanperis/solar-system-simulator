#ifndef SOLAR_CONSTANTS_H
#define SOLAR_CONSTANTS_H

#include <math.h>

/* Baseline physical values: JPL Solar System Dynamics planetary/satellite
 * parameters and the NASA Planetary Fact Sheet. They seed an educational
 * model, not dated ephemerides; periapsis distances and vis-viva speeds below
 * are derived.
 *
 * Masses come from GM: orbit fits measure the product G*M (m^3/s^2) far more
 * precisely than G itself, so M = GM / G keeps every acceleration G*M/r^2
 * equal to the measured GM. GM values are DE440 numbers from
 * https://ssd.jpl.nasa.gov/astro_par.html and
 * https://ssd.jpl.nasa.gov/planets/phys_par.html (checked 2026-10-06).
 * Jupiter-Neptune use planet-only GMs from JPL Horizons physical data
 * (bodies 599/699/799/899, checked 2026-10-07): astro_par lists *system* GMs
 * that include the moons, and the scene carries Jupiter's moons explicitly. */

#define SOLAR_G 6.67430e-11 /* CODATA 2018, m^3 kg^-1 s^-2 */
#define SOLAR_AU_METERS 149597870700.0
#define SOLAR_DAY_SECONDS 86400.0

#define SOLAR_SUN_GM_M3PS2 1.32712440041279419e20
#define SOLAR_SUN_MASS_KG (SOLAR_SUN_GM_M3PS2 / SOLAR_G)
#define SOLAR_SUN_RADIUS_M 695700000.0

#define SOLAR_MERCURY_GM_M3PS2 2.2031868551e13
#define SOLAR_MERCURY_MASS_KG (SOLAR_MERCURY_GM_M3PS2 / SOLAR_G)
#define SOLAR_MERCURY_RADIUS_M 2439700.0
#define SOLAR_MERCURY_SEMI_MAJOR_AXIS_M 57909050000.0
#define SOLAR_MERCURY_ECCENTRICITY 0.205630
#define SOLAR_MERCURY_PERIHELION_M \
    (SOLAR_MERCURY_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_MERCURY_ECCENTRICITY))
#define SOLAR_MERCURY_PERIHELION_SPEED_MPS \
    (sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * \
        ((2.0 / SOLAR_MERCURY_PERIHELION_M) - (1.0 / SOLAR_MERCURY_SEMI_MAJOR_AXIS_M))))

#define SOLAR_VENUS_GM_M3PS2 3.24858592e14
#define SOLAR_VENUS_MASS_KG (SOLAR_VENUS_GM_M3PS2 / SOLAR_G)
#define SOLAR_VENUS_RADIUS_M 6051800.0
#define SOLAR_VENUS_SEMI_MAJOR_AXIS_M 108208000000.0
#define SOLAR_VENUS_ECCENTRICITY 0.006772
#define SOLAR_VENUS_PERIHELION_M \
    (SOLAR_VENUS_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_VENUS_ECCENTRICITY))
#define SOLAR_VENUS_PERIHELION_SPEED_MPS \
    (sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * \
        ((2.0 / SOLAR_VENUS_PERIHELION_M) - (1.0 / SOLAR_VENUS_SEMI_MAJOR_AXIS_M))))

#define SOLAR_EARTH_GM_M3PS2 3.98600435436e14
#define SOLAR_EARTH_MASS_KG (SOLAR_EARTH_GM_M3PS2 / SOLAR_G)
#define SOLAR_EARTH_RADIUS_M 6371000.0
#define SOLAR_EARTH_SEMI_MAJOR_AXIS_M 149597887155.76578
#define SOLAR_EARTH_ECCENTRICITY 0.01671022
#define SOLAR_EARTH_PERIHELION_M \
    (SOLAR_EARTH_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_EARTH_ECCENTRICITY))
#define SOLAR_EARTH_PERIHELION_SPEED_MPS \
    (sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * \
        ((2.0 / SOLAR_EARTH_PERIHELION_M) - (1.0 / SOLAR_EARTH_SEMI_MAJOR_AXIS_M))))

#define SOLAR_MOON_GM_M3PS2 4.902800118e12
#define SOLAR_MOON_MASS_KG (SOLAR_MOON_GM_M3PS2 / SOLAR_G)
#define SOLAR_MOON_RADIUS_M 1737400.0
#define SOLAR_MOON_SEMI_MAJOR_AXIS_M 384400000.0
#define SOLAR_MOON_ECCENTRICITY 0.0549
#define SOLAR_MOON_PERIGEE_M \
    (SOLAR_MOON_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_MOON_ECCENTRICITY))
#define SOLAR_MOON_PERIGEE_SPEED_MPS \
    (sqrt(SOLAR_G * (SOLAR_EARTH_MASS_KG + SOLAR_MOON_MASS_KG) * \
        ((2.0 / SOLAR_MOON_PERIGEE_M) - (1.0 / SOLAR_MOON_SEMI_MAJOR_AXIS_M))))

#define SOLAR_MARS_GM_M3PS2 4.2828375214e13
#define SOLAR_MARS_MASS_KG (SOLAR_MARS_GM_M3PS2 / SOLAR_G)
#define SOLAR_MARS_RADIUS_M 3389500.0 /* JPL mean radius */
#define SOLAR_MARS_SEMI_MAJOR_AXIS_M 227900000000.0
#define SOLAR_MARS_ECCENTRICITY 0.0934
#define SOLAR_MARS_PERIHELION_M \
    (SOLAR_MARS_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_MARS_ECCENTRICITY))
#define SOLAR_MARS_APHELION_M \
    (SOLAR_MARS_SEMI_MAJOR_AXIS_M * (1.0 + SOLAR_MARS_ECCENTRICITY))
#define SOLAR_MARS_PERIHELION_SPEED_MPS \
    (sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * \
        ((2.0 / SOLAR_MARS_PERIHELION_M) - (1.0 / SOLAR_MARS_SEMI_MAJOR_AXIS_M))))

#define SOLAR_PHOBOS_MASS_KG 1.061834199841182e16
#define SOLAR_PHOBOS_RADIUS_M 11080.0
#define SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M 9377000.0
#define SOLAR_PHOBOS_ECCENTRICITY 0.0151
#define SOLAR_PHOBOS_PERIAREION_M \
    (SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_PHOBOS_ECCENTRICITY))
#define SOLAR_PHOBOS_APOAREION_M \
    (SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M * (1.0 + SOLAR_PHOBOS_ECCENTRICITY))
#define SOLAR_PHOBOS_PERIAREION_SPEED_MPS \
    (sqrt(SOLAR_G * (SOLAR_MARS_MASS_KG + SOLAR_PHOBOS_MASS_KG) * \
        ((2.0 / SOLAR_PHOBOS_PERIAREION_M) - (1.0 / SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M))))

#define SOLAR_DEIMOS_MASS_KG 1.441349654645431e15
#define SOLAR_DEIMOS_RADIUS_M 6200.0
#define SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M 23460000.0
#define SOLAR_DEIMOS_ECCENTRICITY 0.00033
#define SOLAR_DEIMOS_PERIAREION_M \
    (SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_DEIMOS_ECCENTRICITY))
#define SOLAR_DEIMOS_APOAREION_M \
    (SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M * (1.0 + SOLAR_DEIMOS_ECCENTRICITY))
#define SOLAR_DEIMOS_PERIAREION_SPEED_MPS \
    (sqrt(SOLAR_G * (SOLAR_MARS_MASS_KG + SOLAR_DEIMOS_MASS_KG) * \
        ((2.0 / SOLAR_DEIMOS_PERIAREION_M) - (1.0 / SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M))))

/* JPL SBDB solution 36 supplies Vesta's osculating orbit and GM. The effective
 * diameter becomes a spherical radius for this point-mass educational model;
 * it does not claim Vesta's irregular shape or its measured inclination. */
#define SOLAR_VESTA_GM_M3PS2 1.72882844e10
#define SOLAR_VESTA_MASS_KG (SOLAR_VESTA_GM_M3PS2 / SOLAR_G)
#define SOLAR_VESTA_RADIUS_M 261385.0
#define SOLAR_VESTA_SEMI_MAJOR_AXIS_M (2.361365965127599 * SOLAR_AU_METERS)
#define SOLAR_VESTA_ECCENTRICITY 0.09020374382834395
#define SOLAR_VESTA_PERIHELION_M \
    (SOLAR_VESTA_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_VESTA_ECCENTRICITY))
#define SOLAR_VESTA_PERIHELION_SPEED_MPS \
    (sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * \
        ((2.0 / SOLAR_VESTA_PERIHELION_M) - (1.0 / SOLAR_VESTA_SEMI_MAJOR_AXIS_M))))

/* JPL supplies Jupiter's planet-only GM, mean radius, and J2000
 * orbit. Inclination is deferred, so this milestone uses the same planar
 * perihelion model as the existing planets. */
#define SOLAR_JUPITER_GM_M3PS2 1.266865319e17
#define SOLAR_JUPITER_MASS_KG (SOLAR_JUPITER_GM_M3PS2 / SOLAR_G)
#define SOLAR_JUPITER_RADIUS_M 69911000.0
#define SOLAR_JUPITER_SEMI_MAJOR_AXIS_M (5.20288700 * SOLAR_AU_METERS)
#define SOLAR_JUPITER_ECCENTRICITY 0.04838624
#define SOLAR_JUPITER_PERIHELION_M \
    (SOLAR_JUPITER_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_JUPITER_ECCENTRICITY))
#define SOLAR_JUPITER_PERIHELION_SPEED_MPS \
    (sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * \
        ((2.0 / SOLAR_JUPITER_PERIHELION_M) - (1.0 / SOLAR_JUPITER_SEMI_MAJOR_AXIS_M))))

/* Saturn uses the same JPL planetary tables and planar perihelion policy as
 * Jupiter. Ring dimensions and tilt below are NASA values used only by the renderer;
 * Saturn's physical radius and gravity never include the rings. */
#define SOLAR_SATURN_GM_M3PS2 3.7931206234e16
#define SOLAR_SATURN_MASS_KG (SOLAR_SATURN_GM_M3PS2 / SOLAR_G)
#define SOLAR_SATURN_RADIUS_M 58232000.0
#define SOLAR_SATURN_SEMI_MAJOR_AXIS_M (9.53667594 * SOLAR_AU_METERS)
#define SOLAR_SATURN_ECCENTRICITY 0.05386179
#define SOLAR_SATURN_PERIHELION_M \
    (SOLAR_SATURN_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_SATURN_ECCENTRICITY))
#define SOLAR_SATURN_PERIHELION_SPEED_MPS \
    (sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * \
        ((2.0 / SOLAR_SATURN_PERIHELION_M) - (1.0 / SOLAR_SATURN_SEMI_MAJOR_AXIS_M))))
#define SOLAR_SATURN_RING_SYSTEM_DIAMETER_M 282000000.0
#define SOLAR_SATURN_RING_OUTER_RADIUS_M (SOLAR_SATURN_RING_SYSTEM_DIAMETER_M / 2.0)
#define SOLAR_SATURN_AXIAL_TILT_DEGREES 26.73

/* JPL physical parameters and approximate-position Table 1, checked 2026-09-14.
 * Like the earlier demo planets these start at perihelion in the ecliptic
 * (simulation X/Z) plane. */
#define SOLAR_URANUS_GM_M3PS2 5.7939506103e15
#define SOLAR_URANUS_MASS_KG (SOLAR_URANUS_GM_M3PS2 / SOLAR_G)
#define SOLAR_URANUS_RADIUS_M 25362000.0
#define SOLAR_URANUS_SEMI_MAJOR_AXIS_M (19.18916464 * SOLAR_AU_METERS)
#define SOLAR_URANUS_ECCENTRICITY .04725744
/* IAU WGCCRE 2015 north pole of Uranus (ICRF, J2000; Archinal et al. 2018).
 * Uranus spins retrograde about it, so its regular moons orbit about the
 * antipode; tools/satellite_catalog.py fills JPL's pole-less "equatorial"
 * frame with that antipode, and tests check the two copies agree. */
#define SOLAR_URANUS_IAU_POLE_RA_DEG 257.311
#define SOLAR_URANUS_IAU_POLE_DEC_DEG -15.175
#define SOLAR_NEPTUNE_GM_M3PS2 6.83509997e15
#define SOLAR_NEPTUNE_MASS_KG (SOLAR_NEPTUNE_GM_M3PS2 / SOLAR_G)
#define SOLAR_NEPTUNE_RADIUS_M 24622000.0
#define SOLAR_NEPTUNE_SEMI_MAJOR_AXIS_M (30.06992276 * SOLAR_AU_METERS)
#define SOLAR_NEPTUNE_ECCENTRICITY .00859048

/* Small-body satellite systems (SPEC T78). Like the planets, both primaries
 * start at a planar heliocentric perihelion with vis-viva speed; their real
 * inclinations (Pluto 17.1 deg, Didymos 3.4 deg) are not modeled here.
 * Pluto: planet-only GM from JPL Horizons body 999 (Brozovic & Jacobson 2024,
 * checked 2026-10-07), IAU volume mean radius, and the JPL SBDB 134340
 * osculating orbit (solution 1, epoch JD 2457588.5). */
#define SOLAR_PLUTO_GM_M3PS2 8.69326e11
#define SOLAR_PLUTO_MASS_KG (SOLAR_PLUTO_GM_M3PS2 / SOLAR_G)
#define SOLAR_PLUTO_RADIUS_M 1188300.0
#define SOLAR_PLUTO_SEMI_MAJOR_AXIS_M (39.58862938517124 * SOLAR_AU_METERS)
#define SOLAR_PLUTO_ECCENTRICITY 0.2518378778576892
/* Didymos primary: approximate GM and triaxial radii from the Horizons DART
 * s547 reconstruction (body 920065803; volume-equivalent diameter 710.3 m)
 * and the JPL SBDB 65803 orbit of the system (solution 240, epoch
 * JD 2461200.5). The GM has no published uncertainty: an estimate (V25). */
#define SOLAR_DIDYMOS_GM_M3PS2 35.1278
#define SOLAR_DIDYMOS_MASS_KG (SOLAR_DIDYMOS_GM_M3PS2 / SOLAR_G)
#define SOLAR_DIDYMOS_RADIUS_M 355.15
#define SOLAR_DIDYMOS_SEMI_MAJOR_AXIS_M (1.642709608529702 * SOLAR_AU_METERS)
#define SOLAR_DIDYMOS_ECCENTRICITY 0.3831233242624545

#endif

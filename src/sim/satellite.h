#ifndef SOLAR_SATELLITE_H
#define SOLAR_SATELLITE_H

#include "body.h"

/* Reference plane of a moon's mean elements. Laplace and planet-equatorial
 * planes carry their pole (RA/Dec, ICRF) in the definition; the generator
 * fills Uranus's equatorial pole from the IAU model (SPEC A79). */
typedef enum SatelliteFrame {
    SATELLITE_FRAME_ECLIPTIC,
    SATELLITE_FRAME_LAPLACE,
    SATELLITE_FRAME_EQUATORIAL
} SatelliteFrame;

/* Source-table units stay explicit here; Body always receives SI values. */
typedef struct SatelliteDefinition {
    int code;
    const char *name;
    const char *group;
    double a_km;
    double eccentricity;
    double periapsis_deg;
    double mean_anomaly_deg;
    double inclination_deg;
    double node_deg;
    double pole_ra_deg;
    double pole_dec_deg;
    double gm_km3_s2;
    double radius_km;
    double period_days; /* JPL mean period; the point-mass model differs by < 1% (A80) */
    SatelliteFrame frame;
    PhysicalQuality mass_quality;
    PhysicalQuality radius_quality;
    bool major; /* rounded moon in the main scene, not only its family scene */
} SatelliteDefinition;

Body satellite_create(const SatelliteDefinition *definition, const Body *parent);

#endif

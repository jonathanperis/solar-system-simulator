#ifndef SOLAR_SATELLITE_H
#define SOLAR_SATELLITE_H

#include "body.h"

typedef enum SatelliteFrame {
    SATELLITE_FRAME_ECLIPTIC,
    SATELLITE_FRAME_LAPLACE
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
    SatelliteFrame frame;
    PhysicalQuality mass_quality;
    PhysicalQuality radius_quality;
} SatelliteDefinition;

Body satellite_create(const SatelliteDefinition *definition, const Body *parent);

#endif

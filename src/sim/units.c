#include "units.h"

#include "constants.h"

float meters_to_render_units(double meters)
{
    return (float)((meters / SOLAR_AU_METERS) * SOLAR_RENDER_UNITS_PER_AU);
}

Vec3d meters_vec_to_render_vec3d(Vec3d meters)
{
    /* Keep double precision until a camera-relative displacement is formed. */
    return vec3d_scale(meters, SOLAR_RENDER_UNITS_PER_AU / SOLAR_AU_METERS);
}

double seconds_to_days(double seconds)
{
    return seconds / SOLAR_DAY_SECONDS;
}

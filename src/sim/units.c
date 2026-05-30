#include "units.h"

#include "constants.h"

float meters_to_render_units(double meters)
{
    return (float)((meters / SOLAR_AU_METERS) * SOLAR_RENDER_UNITS_PER_AU);
}

Vec3d meters_vec_to_render_vec3d(Vec3d meters)
{
    return (Vec3d){
        (double)meters_to_render_units(meters.x),
        (double)meters_to_render_units(meters.y),
        (double)meters_to_render_units(meters.z),
    };
}

double seconds_to_days(double seconds)
{
    return seconds / SOLAR_DAY_SECONDS;
}

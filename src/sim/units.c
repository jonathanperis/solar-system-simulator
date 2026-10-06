#include "units.h"

#include "constants.h"

double seconds_to_days(double seconds)
{
    return seconds / SOLAR_DAY_SECONDS;
}

#include "scene_epoch.h"

#include <math.h>
#include <stdio.h>

/* Horizons heliocentric states of the planetary-system barycenters (Mercury
 * and Venus have no moons, so their centres are their barycenters). */
static const double planet_states[8][6] = {
#include "planet_epoch.inc"
};

typedef struct EpochState {
    int code;
    double state[6];
} EpochState;

static const EpochState states[] = {
#include "scene_epoch.inc"
};

void scene_epoch_planet_state(size_t planet_index, Vec3d *position_m, Vec3d *velocity_mps)
{
    const double *s = planet_states[planet_index < 8 ? planet_index : 0];
    *position_m = (Vec3d){s[0], s[1], s[2]};
    *velocity_mps = (Vec3d){s[3], s[4], s[5]};
}

bool scene_epoch_state(int code, Vec3d *position_m, Vec3d *velocity_mps)
{
    for (size_t i = 0; i < sizeof(states) / sizeof(states[0]); ++i) {
        if (states[i].code != code) continue;
        const double *s = states[i].state;
        *position_m = (Vec3d){s[0], s[1], s[2]};
        *velocity_mps = (Vec3d){s[3], s[4], s[5]};
        return true;
    }
    return false;
}

bool scene_epoch_format_date(double elapsed_seconds, char *out, size_t size)
{
    /* Days since 1970-01-01 00:00 (JD 2440587.5), split into a civil date
     * with Howard Hinnant's days-to-civil algorithm and a time of day. */
    double days = SOLAR_SCENE_EPOCH_JD - 2440587.5 + elapsed_seconds / 86400.0;
    if (!isfinite(days)) return false;
    double whole = floor(days);
    long minutes = (long)floor((days - whole) * 1440.0 + 1e-6);
    if (minutes >= 1440) { whole += 1; minutes -= 1440; }
    long z = (long)whole + 719468;
    long era = (z >= 0 ? z : z - 146096) / 146097;
    long doe = z - era * 146097;
    long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long day_of_year = doe - (365 * yoe + yoe / 4 - yoe / 100);
    long mp = (5 * day_of_year + 2) / 153;
    long day = day_of_year - (153 * mp + 2) / 5 + 1;
    long month = mp < 10 ? mp + 3 : mp - 9;
    long year = yoe + era * 400 + (month <= 2);
    int written = snprintf(out, size, "%04ld-%02ld-%02ld %02ld:%02ld TDB", year, month, day, minutes / 60, minutes % 60);
    return written > 0 && (size_t)written < size;
}

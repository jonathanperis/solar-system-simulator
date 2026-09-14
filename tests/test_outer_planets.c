#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "sim/solar_system.h"
#include "sim/constants.h"
int main(void)
{
    Body bodies[] = {solar_system_create_uranus_at_perihelion(), solar_system_create_neptune_at_perihelion()};
    const double masses[] = {86.8099e24, 102.4092e24}, radii[] = {25362000, 24622000};
    const double axes[] = {19.18916464, 30.06992276}, eccentricities[] = {.04725744, .00859048};
    SolarSystem current = solar_system_create_current();
    assert(current.body_count == 128 && current.bodies[125].id == BODY_ID_SATURN);
    for (size_t i = 0; i < 2; ++i) {
        Body b = bodies[i];
        assert(current.bodies[126+i].id == b.id);
        assert(b.id == (i ? BODY_ID_NEPTUNE : BODY_ID_URANUS));
        assert(b.mass_kg == masses[i] && b.radius_m == radii[i] && b.parent_id == BODY_ID_SUN && !b.fixed);
        double r = vec3d_length(b.position_m), v = vec3d_length(b.velocity_mps);
        assert(fabs(r/(axes[i]*SOLAR_AU_METERS*(1-eccentricities[i]))-1) < 1e-14);
        double expected = sqrt(SOLAR_G*SOLAR_SUN_MASS_KG*(2/r-1/(axes[i]*SOLAR_AU_METERS)));
        assert(fabs(v/expected-1) < 1e-14);
        SolarSystem isolated = {.bodies={current.bodies[0],b}, .body_count=2};
        solar_system_step(&isolated, SOLAR_DAY_SECONDS);
        assert(vec3d_length(vec3d_sub(isolated.bodies[1].position_m, b.position_m)) > 1e8);
    }
    puts("test_outer_planets passed");
}

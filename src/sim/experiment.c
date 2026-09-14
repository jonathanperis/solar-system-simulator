#include "experiment.h"
#include "orbit.h"
#include "constants.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

static const double planet_states[8][6] = {
#include "planet_epoch.inc"
};

bool experiment_parse(const char *text, SolarSystem *out,
    char names[SOLAR_EXPERIMENT_CAPACITY][SOLAR_EXPERIMENT_NAME_BYTES])
{
    int consumed = 0;
    double epoch;
    if (strlen(text) >= SOLAR_EXPERIMENT_TEXT_BYTES ||
        sscanf(text, "SOLAR_EXPERIMENT_V1 %lf%n", &epoch, &consumed) != 1 ||
        epoch != SOLAR_CATALOG_EPOCH_JD || text[consumed] != '\n') return false;
    text += consumed+1;
    SolarSystem system = solar_system_create_sun_only();
    Body (*factories[])(void) = {solar_system_create_mercury_at_perihelion, solar_system_create_venus_at_perihelion,
        solar_system_create_earth_at_perihelion, solar_system_create_mars_at_perihelion,
        solar_system_create_jupiter_at_perihelion, solar_system_create_saturn_at_perihelion,
        solar_system_create_uranus_at_perihelion, solar_system_create_neptune_at_perihelion};
    for (size_t i = 0; i < 8; ++i) {
        Body b = factories[i]();
        const double *s = planet_states[i];
        b.position_m = (Vec3d){s[0],s[1],s[2]}; b.velocity_mps = (Vec3d){s[3],s[4],s[5]};
        system.bodies[system.body_count++] = b;
    }
    size_t count = 0;
    while (*text) {
        if (count == SOLAR_EXPERIMENT_CAPACITY) return false;
        char *end;
        errno=0;
        long identity=strtol(text,&end,10);
        if(errno || end==text || *end!='\t' || identity<1000000 || identity>INT32_MAX) return false;
        int id=(int)identity;
        text=end+1;
        double q,e,i,n,w,tp,mass,radius,mq,rq;
        consumed = 0;
        if (sscanf(text, "%95[^\t]\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf%n",
            names[count],&q,&e,&i,&n,&w,&tp,&mass,&radius,&mq,&rq,&consumed) != 11 ||
            consumed <= 0 || (text[consumed] && text[consumed] != '\n')) return false;
        if (id < 1000000 || !isfinite(mass) || mass < 0 || !isfinite(radius) || radius < 0 ||
            !isfinite(mq) || !isfinite(rq) || mq < 0 || mq > PHYSICAL_PUBLISHED || rq < 0 || rq > PHYSICAL_PUBLISHED ||
            mq != floor(mq) || rq != floor(rq) ||
            ((mass == 0) != (mq == PHYSICAL_UNKNOWN)) || ((radius == 0) != (rq == PHYSICAL_UNKNOWN)) ||
            !isfinite(i) || i < 0 || i > 180 || !isfinite(n) || !isfinite(w) || !isfinite(tp)) return false;
        BodyId body_id = id == 20000004 ? BODY_ID_VESTA : (BodyId)id;
        for (size_t k = 0; k < system.body_count; ++k) if (system.bodies[k].id == body_id) return false;
        OrbitState state;
        if (!orbit_state(q*SOLAR_AU_METERS,e,SOLAR_G*SOLAR_SUN_MASS_KG,
            (SOLAR_CATALOG_EPOCH_JD-tp)*SOLAR_DAY_SECONDS,&state)) return false;
        Body b = body_create_identified(names[count], BODY_KIND_ASTEROID, body_id, BODY_ID_SUN, mass,radius,
            orbit_orient(state.position_m,i,n,w),orbit_orient(state.velocity_mps,i,n,w),false);
        b.mass_quality = (PhysicalQuality)mq; b.radius_quality = (PhysicalQuality)rq;
        b.group = "Catalog experiment";
        system.bodies[system.body_count++] = b;
        ++count;
        text += consumed;
        if (*text == '\n') ++text;
    }
    if (!count) return false;
    *out = system;
    return true;
}

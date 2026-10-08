#include "satellite_catalog.h"

const SatelliteDefinition solar_jovian_moons[SOLAR_JOVIAN_MOON_COUNT] = {
#include "jovian_moons.inc"
};
const SatelliteDefinition solar_saturnian_moons[SOLAR_SATURNIAN_MOON_COUNT] = {
#include "saturnian_moons.inc"
};
const SatelliteDefinition solar_uranian_moons[SOLAR_URANIAN_MOON_COUNT] = {
#include "uranian_moons.inc"
};
const SatelliteDefinition solar_neptunian_moons[SOLAR_NEPTUNIAN_MOON_COUNT] = {
#include "neptunian_moons.inc"
};
const SatelliteDefinition solar_plutonian_moons[SOLAR_PLUTONIAN_MOON_COUNT] = {
#include "plutonian_moons.inc"
};
const SatelliteDefinition solar_didymos_moons[SOLAR_DIDYMOS_MOON_COUNT] = {
#include "didymos_moons.inc"
};
const SatelliteDefinition solar_patroclus_moons[SOLAR_PATROCLUS_MOON_COUNT] = {
#include "patroclus_moons.inc"
};

static const SatelliteCatalog catalogs[] = {
    {BODY_ID_JUPITER, solar_jovian_moons, SOLAR_JOVIAN_MOON_COUNT},
    {BODY_ID_SATURN, solar_saturnian_moons, SOLAR_SATURNIAN_MOON_COUNT},
    {BODY_ID_URANUS, solar_uranian_moons, SOLAR_URANIAN_MOON_COUNT},
    {BODY_ID_NEPTUNE, solar_neptunian_moons, SOLAR_NEPTUNIAN_MOON_COUNT},
    {BODY_ID_PLUTO, solar_plutonian_moons, SOLAR_PLUTONIAN_MOON_COUNT},
    {BODY_ID_DIDYMOS, solar_didymos_moons, SOLAR_DIDYMOS_MOON_COUNT},
    {BODY_ID_PATROCLUS, solar_patroclus_moons, SOLAR_PATROCLUS_MOON_COUNT},
};

const SatelliteCatalog *satellite_catalog_for(BodyId planet)
{
    for (size_t i = 0; i < sizeof(catalogs) / sizeof(catalogs[0]); ++i)
        if (catalogs[i].planet == planet) return &catalogs[i];
    return NULL;
}

#include "experiment.h"
#include "orbit.h"
#include "constants.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

/* Sun + eight planets + the selected small bodies must fit the shared array. */
#define SOLAR_EXPERIMENT_PLANET_COUNT 8
_Static_assert(1 + SOLAR_EXPERIMENT_PLANET_COUNT + SOLAR_EXPERIMENT_CAPACITY <= SOLAR_SYSTEM_BODY_CAPACITY,
    "catalog experiments must fit the scene body array");

static const double planet_states[SOLAR_EXPERIMENT_PLANET_COUNT][6] = {
#include "planet_epoch.inc"
};

#define EXPERIMENT_HEADER "SOLAR_EXPERIMENT_V1 " SOLAR_CATALOG_EPOCH_TEXT "\n"
#define EXPERIMENT_FIELD_COUNT 12
#define EXPERIMENT_NUMBER_BYTES 64

/* One TSV field is the byte range [text, text + length). */
typedef struct Field { const char *text; size_t length; } Field;

/* Numbers are plain decimals: optional sign, digits, optional fraction and
 * exponent. strtod alone would also skip leading whitespace and accept hex
 * floats, "inf" and "nan"; restricting the alphabet first rejects those, and
 * the end-pointer check rejects trailing junk such as "2x". */
static bool parse_decimal(Field field, double *value)
{
    char buffer[EXPERIMENT_NUMBER_BYTES];
    if (field.length == 0 || field.length >= sizeof(buffer) ||
        strspn(field.text, "0123456789+-.eE") < field.length) return false;
    memcpy(buffer, field.text, field.length);
    buffer[field.length] = '\0';
    char *end;
    errno = 0;
    *value = strtod(buffer, &end);
    return errno == 0 && end == buffer + field.length && isfinite(*value);
}

/* Identities and quality codes are unsigned decimal integers; strtoll's
 * whitespace/sign skipping is refused by checking the alphabet first. */
static bool parse_unsigned(Field field, long long *value)
{
    char buffer[16];
    if (field.length == 0 || field.length > 10 || strspn(field.text, "0123456789") < field.length) return false;
    memcpy(buffer, field.text, field.length);
    buffer[field.length] = '\0';
    errno = 0;
    *value = strtoll(buffer, NULL, 10);
    return errno == 0;
}

/* Names are copied verbatim (UTF-8 allowed) but never contain ASCII control
 * bytes: a newline or escape sequence would corrupt CSV rows and HUD text. */
static bool copy_name(Field field, char name[SOLAR_EXPERIMENT_NAME_BYTES])
{
    if (field.length == 0 || field.length >= SOLAR_EXPERIMENT_NAME_BYTES) return false;
    for (size_t i = 0; i < field.length; ++i) {
        unsigned char c = (unsigned char)field.text[i];
        if (c < 0x20 || c == 0x7f) return false;
    }
    memcpy(name, field.text, field.length);
    name[field.length] = '\0';
    return true;
}

/* Splits one row into exactly twelve fields separated by single tab bytes.
 * The row ends at '\n' or at the end of the document. Returns the start of
 * the next row, or NULL if the row has too few or too many fields. */
static const char *split_row(const char *text, Field fields[EXPERIMENT_FIELD_COUNT])
{
    for (size_t i = 0; i < EXPERIMENT_FIELD_COUNT; ++i) {
        size_t length = strcspn(text, "\t\n");
        char terminator = text[length];
        bool last = i + 1 == EXPERIMENT_FIELD_COUNT;
        if (last ? terminator == '\t' : terminator != '\t') return NULL;
        fields[i] = (Field){text, length};
        text += length + (terminator ? 1 : 0);
    }
    return text;
}

bool experiment_parse(const char *text, SolarSystem *out,
    char names[SOLAR_EXPERIMENT_CAPACITY][SOLAR_EXPERIMENT_NAME_BYTES])
{
    /* The version line names the one supported source epoch byte for byte. */
    const size_t prefix = strlen(EXPERIMENT_HEADER);
    if (strlen(text) >= SOLAR_EXPERIMENT_TEXT_BYTES || strncmp(text, EXPERIMENT_HEADER, prefix) != 0) return false;
    text += prefix;

    SolarSystem system = solar_system_create_sun_only();
    Body (*factories[SOLAR_EXPERIMENT_PLANET_COUNT])(void) = {solar_system_create_mercury_at_perihelion,
        solar_system_create_venus_at_perihelion, solar_system_create_earth_at_perihelion,
        solar_system_create_mars_at_perihelion, solar_system_create_jupiter_at_perihelion,
        solar_system_create_saturn_at_perihelion, solar_system_create_uranus_at_perihelion,
        solar_system_create_neptune_at_perihelion};
    for (size_t i = 0; i < SOLAR_EXPERIMENT_PLANET_COUNT; ++i) {
        /* Factories supply identity and physical data; the Horizons snapshot
         * supplies the source-epoch heliocentric state in simulation axes. */
        Body b = factories[i]();
        const double *s = planet_states[i];
        b.position_m = (Vec3d){s[0], s[1], s[2]};
        b.velocity_mps = (Vec3d){s[3], s[4], s[5]};
        if (!solar_system_append(&system, b)) return false;
    }
    size_t count = 0;
    while (*text) {
        /* Fields: id, name, q (AU), e, i, node, periapsis (deg), tp (JD TDB),
         * mass (kg), radius (m), mass quality, radius quality. */
        Field f[EXPERIMENT_FIELD_COUNT];
        if (count == SOLAR_EXPERIMENT_CAPACITY || !(text = split_row(text, f))) return false;
        long long identity, mq, rq;
        double v[8];
        if (!parse_unsigned(f[0], &identity) || identity < 1000000 || identity > INT32_MAX ||
            !copy_name(f[1], names[count])) return false;
        for (size_t k = 0; k < 8; ++k) if (!parse_decimal(f[k + 2], &v[k])) return false;
        if (!parse_unsigned(f[10], &mq) || !parse_unsigned(f[11], &rq) ||
            mq > PHYSICAL_PUBLISHED || rq > PHYSICAL_PUBLISHED) return false;
        double q = v[0], e = v[1], i = v[2], n = v[3], w = v[4], tp = v[5], mass = v[6], radius = v[7];
        /* Unknown physical values are carried as zero (test particle, marker
         * only) and only then: a positive value must have a known quality. */
        bool mass_known = mq != PHYSICAL_UNKNOWN, radius_known = rq != PHYSICAL_UNKNOWN;
        if (mass < 0 || radius < 0 || (mass > 0) != mass_known || (radius > 0) != radius_known ||
            i < 0 || i > 180) return false;
        BodyId body_id = identity == 20000004 ? BODY_ID_VESTA : (BodyId)identity;
        for (size_t k = 0; k < system.body_count; ++k) if (system.bodies[k].id == body_id) return false;
        OrbitState state;
        if (!orbit_state(q * SOLAR_AU_METERS, e, SOLAR_G * SOLAR_SUN_MASS_KG,
            (SOLAR_CATALOG_EPOCH_JD - tp) * SOLAR_DAY_SECONDS, &state)) return false;
        Body b = body_create_identified(names[count], BODY_KIND_ASTEROID, body_id, BODY_ID_SUN, mass, radius,
            orbit_orient(state.position_m, i, n, w), orbit_orient(state.velocity_mps, i, n, w), false);
        b.mass_quality = (PhysicalQuality)mq;
        b.radius_quality = (PhysicalQuality)rq;
        b.group = "Catalog experiment";
        if (!solar_system_append(&system, b)) return false;
        ++count;
    }
    if (!count) return false;
    *out = system;
    return true;
}

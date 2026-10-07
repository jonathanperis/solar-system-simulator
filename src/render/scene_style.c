#include "scene_style.h"

#include <math.h>

#include "../sim/orbit.h"

#define PI 3.14159265358979323846
#define DEG (PI / 180.0)
/* Mean obliquity of the ecliptic at J2000 (IAU 2006), the same value the
 * Jovian satellite conversion uses in src/sim/satellite.c. */
#define J2000_OBLIQUITY_DEGREES 23.439291111

static const char *const texture_files[RENDER_TEXTURE_COUNT] = {
    [RENDER_TEXTURE_SUN] = "sun.jpg",
    [RENDER_TEXTURE_MERCURY] = "mercury.jpg",
    [RENDER_TEXTURE_VENUS] = "venus_atmosphere.jpg",
    [RENDER_TEXTURE_EARTH_DAY] = "earth_day.jpg",
    [RENDER_TEXTURE_EARTH_NIGHT] = "earth_night.jpg",
    [RENDER_TEXTURE_EARTH_CLOUDS] = "earth_clouds.jpg",
    [RENDER_TEXTURE_MOON] = "moon.jpg",
    [RENDER_TEXTURE_MARS] = "mars.jpg",
    [RENDER_TEXTURE_JUPITER] = "jupiter.jpg",
    [RENDER_TEXTURE_SATURN] = "saturn.jpg",
    [RENDER_TEXTURE_SATURN_RING] = "saturn_ring.png",
    [RENDER_TEXTURE_URANUS] = "uranus.jpg",
    [RENDER_TEXTURE_NEPTUNE] = "neptune.jpg",
    [RENDER_TEXTURE_STARS] = "stars_milky_way.jpg",
};

const char *render_texture_file(RenderTextureSlot slot)
{
    if (slot < 0 || slot >= RENDER_TEXTURE_COUNT) return NULL;
    return texture_files[slot];
}

RenderTextureSlot render_texture_for_body(BodyId id)
{
    switch (id) {
        case BODY_ID_SUN: return RENDER_TEXTURE_SUN;
        case BODY_ID_MERCURY: return RENDER_TEXTURE_MERCURY;
        /* Venus's surface is hidden under cloud; the visible map is its
         * atmosphere, which is what an observer would actually see. */
        case BODY_ID_VENUS: return RENDER_TEXTURE_VENUS;
        case BODY_ID_EARTH: return RENDER_TEXTURE_EARTH_DAY;
        case BODY_ID_MOON: return RENDER_TEXTURE_MOON;
        case BODY_ID_MARS: return RENDER_TEXTURE_MARS;
        case BODY_ID_JUPITER: return RENDER_TEXTURE_JUPITER;
        case BODY_ID_SATURN: return RENDER_TEXTURE_SATURN;
        case BODY_ID_URANUS: return RENDER_TEXTURE_URANUS;
        case BODY_ID_NEPTUNE: return RENDER_TEXTURE_NEPTUNE;
        default: return RENDER_TEXTURE_NONE;
    }
}

RenderAtmosphere render_atmosphere_for_body(BodyId id)
{
    /* Rim tints are artistic approximations of each atmosphere's scattered
     * colour; strength 0 means airless (no rim). */
    switch (id) {
        case BODY_ID_EARTH: return (RenderAtmosphere){0.35f, 0.60f, 1.00f, 0.65f};
        case BODY_ID_VENUS: return (RenderAtmosphere){1.00f, 0.88f, 0.65f, 0.45f};
        case BODY_ID_MARS: return (RenderAtmosphere){0.95f, 0.62f, 0.45f, 0.18f};
        case BODY_ID_JUPITER: return (RenderAtmosphere){0.95f, 0.85f, 0.72f, 0.22f};
        case BODY_ID_SATURN: return (RenderAtmosphere){0.95f, 0.87f, 0.62f, 0.22f};
        case BODY_ID_URANUS: return (RenderAtmosphere){0.62f, 0.90f, 1.00f, 0.38f};
        case BODY_ID_NEPTUNE: return (RenderAtmosphere){0.40f, 0.62f, 1.00f, 0.38f};
        default: return (RenderAtmosphere){0, 0, 0, 0};
    }
}

/* IAU WGCCRE 2015 rotation elements (Archinal et al. 2018, Celest Mech Dyn
 * Astr 130:22). The north pole has ICRF right ascension
 * alpha0 = alpha + alpha_rate*T and declination delta0 = delta + delta_rate*T;
 * the prime meridian sits W = w0 + w_rate*d degrees east of the node where the
 * body's equator crosses the ICRF equator. T is Julian centuries and d is days
 * since J2000 TDB. Small periodic terms are omitted; they move these axes by
 * well under a degree, invisible at simulator scales. */
typedef struct RotationModel {
    BodyId id;
    double alpha, alpha_rate, delta, delta_rate, w0, w_rate;
} RotationModel;

static const RotationModel rotation_models[] = {
    {BODY_ID_SUN, 286.13, 0, 63.87, 0, 84.176, 14.1844000},
    {BODY_ID_MERCURY, 281.0103, -0.0328, 61.4155, -0.0049, 329.5988, 6.1385108},
    /* Retrograde rotators keep the pole on the invariable plane's north side
     * and spin with negative W rate, so their angular velocity points south. */
    {BODY_ID_VENUS, 272.76, 0, 67.16, 0, 160.20, -1.4813688},
    {BODY_ID_EARTH, 0.00, -0.641, 90.00, -0.557, 190.147, 360.9856235},
    {BODY_ID_MOON, 269.9949, 0.0031, 66.5392, 0.0130, 38.3213, 13.17635815},
    {BODY_ID_MARS, 317.269202, -0.10927547, 54.432516, -0.05827105, 176.049863, 350.891982443297},
    {BODY_ID_JUPITER, 268.056595, -0.006499, 64.495303, 0.002413, 284.95, 870.5360000},
    {BODY_ID_SATURN, 40.589, -0.036, 83.537, -0.004, 38.90, 810.7939024},
    {BODY_ID_URANUS, 257.311, 0, -15.175, 0, 203.81, -501.1600928},
    {BODY_ID_NEPTUNE, 299.36, 0, 43.46, 0, 249.978, 541.1397757},
};

/* ICRF (equatorial) vector -> J2000 ecliptic -> simulation axes. Rotating by
 * the obliquity about the shared X axis (the vernal equinox) turns the
 * equator into the ecliptic; orbit_ecliptic_to_simulation then applies the
 * project's proper rotation (x, y, z) = (X, Z, -Y). */
static Vec3d equatorial_to_simulation(Vec3d q)
{
    double e = J2000_OBLIQUITY_DEGREES * DEG;
    Vec3d ecliptic = {q.x, cos(e) * q.y + sin(e) * q.z, -sin(e) * q.y + cos(e) * q.z};
    return orbit_ecliptic_to_simulation(ecliptic);
}

RenderOrientation render_body_orientation(BodyId id, double days_since_j2000)
{
    const RotationModel *model = NULL;
    for (size_t i = 0; i < sizeof(rotation_models) / sizeof(rotation_models[0]); ++i)
        if (rotation_models[i].id == id) model = &rotation_models[i];
    if (!model) return (RenderOrientation){{0, 1, 0}, {1, 0, 0}, 0, false};

    double d = days_since_j2000, T = d / 36525.0;
    double alpha = model->alpha + model->alpha_rate * T;
    double delta = model->delta + model->delta_rate * T;
    double w = model->w0 + model->w_rate * d;
    if (id == BODY_ID_NEPTUNE) {
        /* Neptune's pole precesses noticeably; keep its leading N term. */
        double n = (357.85 + 52.316 * T) * DEG;
        alpha += 0.70 * sin(n);
        delta -= 0.51 * cos(n);
        w -= 0.48 * sin(n);
    }
    alpha *= DEG;
    delta *= DEG;
    w = fmod(w, 360.0) * DEG;

    /* Pole P from (alpha0, delta0). The body equator's ascending node Q on the
     * ICRF equator lies at right ascension alpha0 + 90 deg. Rotating Q east by
     * W about P gives the prime meridian: M = cos W Q + sin W (P x Q). */
    Vec3d pole = {cos(delta) * cos(alpha), cos(delta) * sin(alpha), sin(delta)};
    Vec3d node = {-sin(alpha), cos(alpha), 0};
    Vec3d east = vec3d_cross(pole, node);
    Vec3d meridian = vec3d_add(vec3d_scale(node, cos(w)), vec3d_scale(east, sin(w)));
    return (RenderOrientation){equatorial_to_simulation(pole), equatorial_to_simulation(meridian), model->w_rate, true};
}

float render_trail_alpha(size_t index, size_t count)
{
    if (count <= 1 || index + 1 >= count) return 1.0f;
    /* t = 0 oldest, 1 newest. A gentle t^1.5 curve keeps most of the orbit
     * readable while the floor stops ancient history from vanishing. It is
     * written as t * sqrt(t): this runs for every trail vertex every frame
     * (up to ~130k), and pow() is several times slower in WebAssembly. */
    float t = (float)index / (float)(count - 1);
    return 0.12f + 0.88f * t * sqrtf(t);
}

static double smoothstep(double edge0, double edge1, double x)
{
    double t = (x - edge0) / (edge1 - edge0);
    t = t < 0 ? 0 : t > 1 ? 1 : t;
    return t * t * (3 - 2 * t);
}

float render_grid_alpha(double distance, double half_width)
{
    if (!(half_width > 0) || distance >= half_width) return 0.0f;
    /* Full (but subdued) near the centre, fading out before the edge, so the
     * dense far lines that alias into moire are simply not drawn. */
    return (float)(0.5 * (1.0 - smoothstep(0.2 * half_width, half_width, distance)));
}

RenderGridLevels render_grid_levels(double camera_distance)
{
    double distance = camera_distance > 1e-12 ? camera_distance : 1e-12;
    /* level 0 = a camera 10 units away (1 AU at 10 units per AU). Each whole
     * level is a decade; the fractional part drives the cross-fade. */
    double level = log10(distance) - 1.0, decade = floor(level), fraction = level - decade;
    double minor = pow(10.0, decade);
    return (RenderGridLevels){minor, minor * 10.0, 1.0 - fraction, distance * 3.5};
}

double render_projected_radius_pixels(double radius, double distance, double fovy_degrees, double viewport_height)
{
    if (!(radius > 0) || !(distance > 0) || !(fovy_degrees > 0) || !(viewport_height > 0)) return 0;
    /* Pinhole projection: the viewport's half height spans tan(fovy / 2) of
     * distance, so one world unit at `distance` covers
     * (viewport_height / 2) / (distance * tan(fovy / 2)) pixels. */
    double half_height_units = distance * tan(fovy_degrees * DEG / 2);
    return radius * (viewport_height / 2) / half_height_units;
}

float render_glow_intensity(double r)
{
    if (r <= 0) return 1.0f;
    if (r >= 1) return 0.0f;
    /* A tight bright core plus a wide soft halo; (1-r)^2 forces exactly zero
     * at the billboard edge so no square outline shows. */
    return (float)((1 - r) * (1 - r) * (0.3 + 0.7 * exp(-14.0 * r * r)));
}

size_t render_sphere_vertex_count(int slices, int rings)
{
    return slices < 3 || rings < 2 ? 0 : (size_t)(slices + 1) * (size_t)(rings + 1);
}

size_t render_sphere_index_count(int slices, int rings)
{
    return slices < 3 || rings < 2 ? 0 : (size_t)slices * (size_t)rings * 6;
}

bool render_build_sphere(int slices, int rings, float *positions, float *normals, float *texcoords,
    unsigned short *indices)
{
    size_t vertices = render_sphere_vertex_count(slices, rings);
    if (vertices == 0 || vertices > 65535) return false;
    size_t v = 0;
    /* Each row is one latitude (north pole first, v = 0 at the image top);
     * the seam column is duplicated (u = 0 and u = 1) so the map can wrap. */
    for (int j = 0; j <= rings; ++j) {
        double latitude = PI / 2 - PI * (double)j / rings;
        for (int i = 0; i <= slices; ++i, ++v) {
            double u = (double)i / slices, longitude = 2 * PI * u - PI;
            /* East-positive longitude turns counterclockwise about +Y, the
             * same sense as prograde orbits in the simulation frame. */
            float p[3] = {(float)(cos(longitude) * cos(latitude)), (float)sin(latitude),
                (float)(-sin(longitude) * cos(latitude))};
            for (int k = 0; k < 3; ++k) positions[v * 3 + k] = normals[v * 3 + k] = p[k];
            texcoords[v * 2] = (float)u;
            texcoords[v * 2 + 1] = (float)j / (float)rings;
        }
    }
    size_t n = 0;
    for (int j = 0; j < rings; ++j) {
        for (int i = 0; i < slices; ++i) {
            unsigned short a = (unsigned short)(j * (slices + 1) + i), b = (unsigned short)(a + slices + 1);
            /* Counterclockwise seen from outside: OpenGL's default front face. */
            unsigned short quad[6] = {a, b, (unsigned short)(a + 1), (unsigned short)(a + 1), b, (unsigned short)(b + 1)};
            for (int k = 0; k < 6; ++k) indices[n++] = quad[k];
        }
    }
    return true;
}

size_t render_ring_vertex_count(int segments)
{
    return segments < 3 ? 0 : (size_t)(segments + 1) * 2;
}

size_t render_ring_index_count(int segments)
{
    return segments < 3 ? 0 : (size_t)segments * 6;
}

bool render_build_ring(int segments, float inner_radius, float outer_radius, float *positions,
    float *normals, float *texcoords, unsigned short *indices)
{
    if (segments < 3 || !(inner_radius > 0) || !(outer_radius > inner_radius)) return false;
    if (render_ring_vertex_count(segments) > 65535) return false;
    for (int k = 0; k <= segments; ++k) {
        double angle = 2 * PI * (double)k / segments;
        for (int edge = 0; edge < 2; ++edge) {
            size_t v = (size_t)k * 2 + (size_t)edge;
            float radius = edge ? outer_radius : inner_radius;
            positions[v * 3] = radius * (float)cos(angle);
            positions[v * 3 + 1] = 0;
            positions[v * 3 + 2] = radius * (float)sin(angle);
            normals[v * 3] = 0;
            normals[v * 3 + 1] = 1;
            normals[v * 3 + 2] = 0;
            /* The opacity strip runs inner -> outer along u. */
            texcoords[v * 2] = (float)edge;
            texcoords[v * 2 + 1] = (float)k / (float)segments;
        }
    }
    size_t n = 0;
    for (int k = 0; k < segments; ++k) {
        unsigned short i0 = (unsigned short)(k * 2), o0 = (unsigned short)(i0 + 1);
        unsigned short i1 = (unsigned short)(i0 + 2), o1 = (unsigned short)(i0 + 3);
        unsigned short quad[6] = {i0, i1, o0, o0, i1, o1};
        for (int j = 0; j < 6; ++j) indices[n++] = quad[j];
    }
    return true;
}

#include "scene_style.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../sim/constants.h"
#include "../sim/orbit.h"
#include "constellation_lines.inc"

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
    [RENDER_TEXTURE_PHOBOS] = "phobos.jpg",
    [RENDER_TEXTURE_VESTA] = "vesta.jpg",
    [RENDER_TEXTURE_IO] = "io.jpg",
    [RENDER_TEXTURE_EUROPA] = "europa.jpg",
    [RENDER_TEXTURE_GANYMEDE] = "ganymede.jpg",
    [RENDER_TEXTURE_CALLISTO] = "callisto.jpg",
    [RENDER_TEXTURE_ENCELADUS] = "enceladus.jpg",
    [RENDER_TEXTURE_TETHYS] = "tethys.jpg",
    [RENDER_TEXTURE_DIONE] = "dione.jpg",
    [RENDER_TEXTURE_RHEA] = "rhea.jpg",
    [RENDER_TEXTURE_IAPETUS] = "iapetus.jpg",
    [RENDER_TEXTURE_TRITON] = "triton.jpg",
    [RENDER_TEXTURE_PLUTO] = "pluto.jpg",
    [RENDER_TEXTURE_CHARON] = "charon.jpg",
};

const char *render_texture_file(RenderTextureSlot slot)
{
    if (slot < 0 || slot >= RENDER_TEXTURE_COUNT) return NULL;
    return texture_files[slot];
}

RenderTextureSlot render_texture_for_body(BodyId id)
{
    /* An int switch: moons without a named BodyId use their NAIF number. */
    switch ((int)id) {
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
        /* USGS global mosaics (A109). Titan keeps its flat haze colour: its
         * surface maps are infrared and radar views no eye would see. */
        case BODY_ID_PHOBOS: return RENDER_TEXTURE_PHOBOS;
        case BODY_ID_VESTA: return RENDER_TEXTURE_VESTA;
        case BODY_ID_IO: return RENDER_TEXTURE_IO;
        case BODY_ID_EUROPA: return RENDER_TEXTURE_EUROPA;
        case BODY_ID_GANYMEDE: return RENDER_TEXTURE_GANYMEDE;
        case BODY_ID_CALLISTO: return RENDER_TEXTURE_CALLISTO;
        case BODY_ID_ENCELADUS: return RENDER_TEXTURE_ENCELADUS;
        case 603: return RENDER_TEXTURE_TETHYS;
        case 604: return RENDER_TEXTURE_DIONE;
        case 605: return RENDER_TEXTURE_RHEA;
        case 608: return RENDER_TEXTURE_IAPETUS;
        case BODY_ID_TRITON: return RENDER_TEXTURE_TRITON;
        case BODY_ID_PLUTO: return RENDER_TEXTURE_PLUTO;
        case BODY_ID_CHARON: return RENDER_TEXTURE_CHARON;
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
        /* Titan: a 1.5-bar nitrogen atmosphere under thick orange haze, the
         * only moon whose limb glows. */
        case BODY_ID_TITAN: return (RenderAtmosphere){1.00f, 0.72f, 0.38f, 0.55f};
        default: return (RenderAtmosphere){0, 0, 0, 0};
    }
}

/* IAU WGCCRE 2015 rotation elements (Archinal et al. 2018, Celest Mech Dyn
 * Astr 130:22). The north pole has ICRF right ascension
 * alpha0 = alpha + alpha_rate*T and declination delta0 = delta + delta_rate*T;
 * the prime meridian sits W = w0 + w_rate*d degrees east of the node where the
 * body's equator crosses the ICRF equator. T is Julian centuries and d is days
 * since J2000 TDB. Periodic terms are omitted unless noted below; for the planets they move these axes by well
 * under a degree. The Moon's are larger (pole ~1.5 deg, prime meridian ~3.6
 * deg), acceptable for an illustrative texture orientation. */
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
    /* Moons with maps (SPEC A109). Synchronous rotators: W advances once per
     * orbit and longitude 0 is near the sub-planet point. NAIF ids. */
    {BODY_ID_PHOBOS, 317.67071657, -0.10844326, 52.88627266, -0.06134706, 35.18774440, 1128.84475928},
    {BODY_ID_VESTA, 309.031, -0.008, 42.235, 0, 285.39, 1617.3329428},
    {BODY_ID_IO, 268.05, -0.009, 64.50, 0.003, 200.39, 203.4889538},
    {BODY_ID_EUROPA, 268.08, -0.009, 64.51, 0.003, 36.022, 101.3747235},
    {BODY_ID_GANYMEDE, 268.20, -0.009, 64.57, 0.003, 44.064, 50.3176081},
    {BODY_ID_CALLISTO, 268.72, -0.009, 64.83, 0.003, 259.51, 21.5710715},
    {BODY_ID_ENCELADUS, 40.66, -0.036, 83.52, -0.004, 6.32, 262.7318996},
    {(BodyId)603, 40.66, -0.036, 83.52, -0.004, 8.95, 190.6979085},   /* Tethys */
    {(BodyId)604, 40.66, -0.036, 83.52, -0.004, 357.6, 131.5349316},  /* Dione */
    {(BodyId)605, 40.38, -0.036, 83.55, -0.004, 235.16, 79.6900478},  /* Rhea */
    {(BodyId)608, 318.16, -3.949, 75.03, -1.143, 355.2, 4.5379572},   /* Iapetus */
    /* Triton orbits retrograde, so it spins with negative W rate. */
    {BODY_ID_TRITON, 299.36, 0, 41.17, 0, 296.53, -61.2572637},
    /* Pluto and Charon are locked to each other: one pole, meridians 180 deg apart. */
    {BODY_ID_PLUTO, 132.993, 0, -6.163, 0, 302.695, 56.3625225},
    {BODY_ID_CHARON, 132.993, 0, -6.163, 0, 122.695, 56.3625225},
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
    } else if (id == BODY_ID_TRITON) {
        /* Triton's pole circles Neptune's every ~688 years by tens of
         * degrees; keep the series through 4 N7 (the rest is < 0.3 deg). */
        double n = (177.85 + 52.316 * T) * DEG;
        alpha += -32.35 * sin(n) - 6.28 * sin(2 * n) - 2.08 * sin(3 * n) - 0.74 * sin(4 * n);
        delta += 22.55 * cos(n) + 2.10 * cos(2 * n) + 0.55 * cos(3 * n) + 0.16 * cos(4 * n);
        w += 22.25 * sin(n) + 6.73 * sin(2 * n) + 2.05 * sin(3 * n) + 0.74 * sin(4 * n);
    } else if (id == BODY_ID_PHOBOS) {
        /* Phobos's forced libration (M1) and its secular tidal speed-up. */
        double m1 = (190.72646643 + 15917.10818695 * T) * DEG;
        alpha -= 1.78428399 * sin(m1);
        delta -= 1.07516537 * cos(m1);
        w += 1.42421769 * sin(m1) + 12.72192797 * T * T;
    } else if ((int)id == 603) {
        /* Tethys: the Voyager-era S4/S5 librations, ~10 degrees in W. */
        double s4 = (300.00 - 7225.9 * T) * DEG, s5 = (316.45 + 506.2 * T) * DEG;
        alpha += 9.66 * sin(s4);
        delta -= 1.09 * cos(s4);
        w += -9.60 * sin(s4) + 2.23 * sin(s5);
    } else if ((int)id == 605) {
        double s6 = (345.20 - 1016.3 * T) * DEG; /* Rhea */
        alpha += 3.10 * sin(s6);
        delta -= 0.35 * cos(s6);
        w -= 3.08 * sin(s6);
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

size_t render_trail_stride_for_extent(size_t point_count, size_t base_stride, double extent_pixels)
{
    if (base_stride == 0) base_stride = 1;
    if (point_count < 2 || !(extent_pixels > 0)) return base_stride;
    double wanted = extent_pixels / RENDER_TRAIL_PIXELS_PER_POINT;
    if (wanted < RENDER_TRAIL_MIN_POINTS) wanted = RENDER_TRAIL_MIN_POINTS;
    if (wanted >= (double)point_count) return base_stride;
    size_t stride = (size_t)ceil((double)(point_count - 1) / wanted);
    return stride > base_stride ? stride : base_stride;
}

bool render_marker_draws_wire(double radius_pixels)
{
    return radius_pixels >= RENDER_WIRE_MARKER_MIN_RADIUS_PIXELS;
}

double render_marker_dot_pixels(double radius_pixels)
{
    /* A wireframe this small fills roughly 1.5 times its radius. */
    double blob = 1.5 * radius_pixels;
    return blob > RENDER_DOT_PIXELS ? blob : RENDER_DOT_PIXELS;
}

RenderFrustum render_frustum(Vec3d apex, Vec3d forward, Vec3d up, double fovy_degrees, double aspect, double margin)
{
    Vec3d f = vec3d_scale(forward, 1.0 / vec3d_length(forward));
    Vec3d side = vec3d_cross(f, up);
    Vec3d right = vec3d_scale(side, 1.0 / vec3d_length(side));
    Vec3d true_up = vec3d_cross(right, f);
    /* Half-angles of the (slightly widened) view: vertical from fovy,
     * horizontal from the aspect ratio of the same image plane. */
    double tan_y = tan(fovy_degrees * acos(-1.0) / 360.0) * (1.0 + margin);
    double tan_x = tan_y * aspect;
    double cy = 1.0 / sqrt(1.0 + tan_y * tan_y), sy = tan_y * cy;
    double cx = 1.0 / sqrt(1.0 + tan_x * tan_x), sx = tan_x * cx;
    /* Outward normals: a point p (relative to the apex) is outside a plane
     * when dot(p, normal) > 0, e.g. right of the view when its rightward
     * component exceeds tan_x times its forward component. */
    RenderFrustum frustum = {apex, {
        vec3d_sub(vec3d_scale(right, cx), vec3d_scale(f, sx)),
        vec3d_sub(vec3d_scale(right, -cx), vec3d_scale(f, sx)),
        vec3d_sub(vec3d_scale(true_up, cy), vec3d_scale(f, sy)),
        vec3d_sub(vec3d_scale(true_up, -cy), vec3d_scale(f, sy)),
    }};
    return frustum;
}

unsigned render_frustum_outcode(const RenderFrustum *frustum, Vec3d point)
{
    /* Inline arithmetic: this runs for every trail sample drawn. */
    double x = point.x - frustum->apex.x, y = point.y - frustum->apex.y, z = point.z - frustum->apex.z;
    unsigned code = 0;
    for (unsigned k = 0; k < 4; ++k) {
        const Vec3d *n = &frustum->normal[k];
        if (x * n->x + y * n->y + z * n->z > 0) code |= 1u << k;
    }
    return code;
}

static double distance_squared(Vec3d a, Vec3d b)
{
    double x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
    return x * x + y * y + z * z;
}

bool render_clip_segment_outside_sphere(Vec3d *a, Vec3d *b, Vec3d center, double radius)
{
    /* Runs for every drawn trail segment, so the common "both outside" case
     * uses plain arithmetic (no calls across translation units). */
    double r2 = radius * radius;
    bool a_inside = distance_squared(*a, center) < r2;
    bool b_inside = distance_squared(*b, center) < r2;
    if (a_inside && b_inside) return false;
    if (!a_inside && !b_inside) return true;
    /* Walk from the outside point toward the inside one: P(s) = out + s*(in - out).
     * |P(s) - c|^2 = r^2 is a quadratic in s; its smaller root is where the
     * segment first meets the sphere. */
    Vec3d *outside = a_inside ? b : a, *inside = a_inside ? a : b;
    Vec3d d = vec3d_sub(*inside, *outside), f = vec3d_sub(*outside, center);
    double qa = vec3d_dot(d, d), qb = 2 * vec3d_dot(f, d), qc = vec3d_dot(f, f) - r2;
    double disc = qb * qb - 4 * qa * qc;
    double s = qa > 0 && disc >= 0 ? (-qb - sqrt(disc)) / (2 * qa) : 1.0;
    s = s < 0 ? 0 : s > 1 ? 1 : s;
    *inside = vec3d_add(*outside, vec3d_scale(d, s));
    return true;
}

bool render_body_wants_label(BodyKind kind, bool selected, bool known_radius, double radius_pixels,
    double parent_separation_pixels)
{
    if (selected || kind == BODY_KIND_STAR || kind == BODY_KIND_PLANET || kind == BODY_KIND_DWARF_PLANET) return true;
    return known_radius && radius_pixels >= RENDER_LABEL_MIN_RADIUS_PIXELS && parent_separation_pixels >= RENDER_LABEL_MIN_SEPARATION_PIXELS;
}

static bool boxes_overlap(const RenderLabelBox *a, const RenderLabelBox *b)
{
    const float margin = 4.0f; /* keep a little breathing room between labels */
    return a->x < b->x + b->width + margin && b->x < a->x + a->width + margin &&
        a->y < b->y + b->height + margin && b->y < a->y + a->height + margin;
}

void render_declutter_labels(RenderLabelBox *boxes, size_t count)
{
    /* Greedy placement: visit labels from most to least important (ties in
     * their original order) and keep each one that overlaps nothing kept so
     * far. n is at most the scene size, so the O(n^2) passes are cheap. */
    for (size_t i = 0; i < count; ++i) boxes[i].visible = false;
    for (size_t placed = 0; placed < count; ++placed) {
        size_t best = count;
        for (size_t i = 0; i < count; ++i) {
            if (boxes[i].visible || boxes[i].priority == INT_MIN) continue;
            if (best == count || boxes[i].priority > boxes[best].priority) best = i;
        }
        if (best == count) break;
        bool clear = true;
        for (size_t j = 0; j < count && clear; ++j) if (boxes[j].visible && boxes_overlap(&boxes[best], &boxes[j])) clear = false;
        if (clear) boxes[best].visible = true;
        else boxes[best].priority = INT_MIN; /* rejected: never revisit */
    }
}

double render_world_units_per_pixel(double distance, double fovy_degrees, double viewport_height)
{
    if (!(distance > 0) || !(fovy_degrees > 0) || !(viewport_height > 0)) return 0;
    return distance * 2 * tan(fovy_degrees * DEG / 2) / viewport_height;
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

bool render_orbit_guide(Vec3d relative_position_m, Vec3d relative_velocity_mps, double mu, RenderOrbitGuide *out)
{
    double r = vec3d_length(relative_position_m), speed = vec3d_length(relative_velocity_mps);
    if (!(mu > 0) || !(r > 0) || !(speed > 0)) return false;
    /* Specific angular momentum h = r x v fixes the orbit plane; a radial
     * velocity leaves no plane to draw in. */
    Vec3d h = vec3d_cross(relative_position_m, relative_velocity_mps);
    double h_length = vec3d_length(h);
    if (!(h_length > 1e-9 * r * speed)) return false;
    /* Vis-viva energy v^2/2 - mu/r is negative only for a closed orbit, whose
     * semi-major axis is then a = -mu / (2 energy). */
    double energy = 0.5 * speed * speed - mu / r;
    if (!(energy < 0)) return false;
    double a = -mu / (2 * energy);
    /* Eccentricity vector (v x h)/mu - r/|r| points at periapsis; its length
     * is e. A near-circular orbit has no defined periapsis, so it starts at
     * the body's current position instead. */
    Vec3d e_vector = vec3d_sub(vec3d_scale(vec3d_cross(relative_velocity_mps, h), 1.0 / mu),
        vec3d_scale(relative_position_m, 1.0 / r));
    double e = vec3d_length(e_vector);
    if (!(e < 1)) return false;
    Vec3d periapsis = e > 1e-10 ? vec3d_scale(e_vector, 1.0 / e) : vec3d_scale(relative_position_m, 1.0 / r);
    Vec3d ahead = vec3d_cross(vec3d_scale(h, 1.0 / h_length), periapsis);
    *out = (RenderOrbitGuide){a, a * sqrt(1 - e * e), e, periapsis, ahead};
    return true;
}

Vec3d render_orbit_guide_point(const RenderOrbitGuide *guide, double cos_e, double sin_e)
{
    return vec3d_add(vec3d_scale(guide->periapsis, guide->semi_major_m * (cos_e - guide->eccentricity)),
        vec3d_scale(guide->ahead, guide->semi_minor_m * sin_e));
}

int render_orbit_guide_segments(double semi_major_pixels)
{
    if (!(semi_major_pixels >= RENDER_ORBIT_GUIDE_MIN_PIXELS)) return 0;
    double segments = ceil(2 * 3.14159265358979323846 * semi_major_pixels / RENDER_ORBIT_GUIDE_PIXELS_PER_SEGMENT);
    if (segments < RENDER_ORBIT_GUIDE_MIN_SEGMENTS) return RENDER_ORBIT_GUIDE_MIN_SEGMENTS;
    return segments > RENDER_ORBIT_GUIDE_MAX_SEGMENTS ? RENDER_ORBIT_GUIDE_MAX_SEGMENTS : (int)segments;
}

bool render_body_wants_ring_marker(BodyKind kind, double radius_pixels)
{
    return (kind == BODY_KIND_PLANET || kind == BODY_KIND_DWARF_PLANET) && radius_pixels < RENDER_RING_MARKER_MAX_RADIUS_PIXELS;
}

double render_selection_bracket_half_size(double radius_pixels)
{
    double half = radius_pixels * 1.15 + 7.0;
    return half > RENDER_SELECTION_MIN_HALF_PIXELS ? half : RENDER_SELECTION_MIN_HALF_PIXELS;
}

bool render_selection_wants_brackets(double radius_pixels)
{
    return radius_pixels < RENDER_SELECTION_MAX_RADIUS_PIXELS;
}

double render_eclipse_fraction(double separation, double occluder_radius, double sun_radius)
{
    if (!(occluder_radius > 0)) return 0;
    if (!(sun_radius > 0)) return separation < occluder_radius ? 1 : 0;
    /* 1 - smoothstep(|rho - sigma|, rho + sigma, theta): full overlap while
     * one disc lies inside the other, none once their edges part. */
    double inner = fabs(occluder_radius - sun_radius), outer = occluder_radius + sun_radius;
    double t = (separation - inner) / (outer - inner);
    t = t < 0 ? 0 : t > 1 ? 1 : t;
    double covered = 1 - t * t * (3 - 2 * t);
    double area = occluder_radius * occluder_radius / (sun_radius * sun_radius);
    return covered * (area < 1 ? area : 1);
}

int render_pick_body(const RenderPickCandidate *candidates, size_t count, float x, float y)
{
    int best = -1;
    float best_score = 1e30f;
    for (size_t i = 0; i < count; ++i) {
        const RenderPickCandidate *c = &candidates[i];
        if (!c->in_front) continue;
        float distance = hypotf(x - c->x, y - c->y);
        float edge = distance - c->radius_pixels;
        if (edge > RENDER_PICK_SLOP_PIXELS) continue;
        /* Inside a disc scores below any near miss; among near misses the
         * closest edge wins, with planets and dwarf planets given 3 px. */
        float score = edge <= 0 ? -1000.0f + distance
            : edge - (c->kind == BODY_KIND_PLANET || c->kind == BODY_KIND_DWARF_PLANET || c->kind == BODY_KIND_STAR ? 3.0f : 0.0f);
        if (score < best_score) {
            best_score = score;
            best = (int)i;
        }
    }
    return best;
}

void render_resolution_governor_init(RenderResolutionGovernor *governor, float max_scale)
{
    *governor = (RenderResolutionGovernor){max_scale > 1.0f ? max_scale : 1.0f, 0.0f, 0.0f};
}

float render_resolution_governor_update(RenderResolutionGovernor *governor, float frame_seconds)
{
    if (!(frame_seconds > 0.0f) || frame_seconds > RENDER_GOVERNOR_STALL_SECONDS) return governor->scale;
    governor->elapsed_seconds += frame_seconds;
    if (governor->elapsed_seconds < RENDER_GOVERNOR_WARMUP_SECONDS || governor->scale <= 1.0f) return governor->scale;
    if (frame_seconds > RENDER_GOVERNOR_SLOW_FRAME_SECONDS) governor->debt_seconds += frame_seconds;
    else governor->debt_seconds = fmaxf(0.0f, governor->debt_seconds - 2.0f * frame_seconds);
    if (governor->debt_seconds >= RENDER_GOVERNOR_DEBT_SECONDS) {
        governor->scale = fmaxf(1.0f, governor->scale - 0.5f);
        governor->debt_seconds = 0.0f;
    }
    return governor->scale;
}

bool render_eclipse_caster(Vec3d receiver_m, double receiver_radius_m, Vec3d caster_m, double caster_radius_m,
    Vec3d sun_m, double sun_radius_m, double scale, RenderEclipseCaster *out)
{
    Vec3d to_sun = vec3d_sub(sun_m, receiver_m);
    double sun_distance = vec3d_length(to_sun);
    if (!(sun_distance > 0) || !(caster_radius_m > 0)) return false;
    Vec3d light = vec3d_scale(to_sun, 1.0 / sun_distance);
    Vec3d d = vec3d_sub(caster_m, receiver_m);
    double along = vec3d_dot(d, light);
    if (!(along > 0)) return false;
    double miss = sqrt(fmax(vec3d_dot(d, d) - along * along, 0));
    /* The penumbra widens by the Sun's angular radius per metre travelled. */
    double gap = miss - caster_radius_m - receiver_radius_m - sun_radius_m / sun_distance * along;
    if (gap > 0) return false;
    *out = (RenderEclipseCaster){vec3d_scale(d, scale), caster_radius_m * scale, gap * scale};
    return true;
}

RenderOrientation render_sky_orientation(void)
{
    /* IAU galactic frame in ICRF (Hipparcos definition): Galactic Centre at
     * RA 266.40499, Dec -28.93617; north galactic pole at RA 192.85948,
     * Dec 27.12825, so the south pole is its antipode. */
    Vec3d centre = orbit_icrf_direction(266.40499, -28.93617);
    Vec3d south = orbit_icrf_direction(192.85948 - 180.0, -27.12825);
    /* Remove the tiny rounding overlap so the frame is exactly orthonormal. */
    south = vec3d_sub(south, vec3d_scale(centre, vec3d_dot(south, centre)));
    south = vec3d_scale(south, 1.0 / vec3d_length(south));
    return (RenderOrientation){south, centre, 0, true};
}

double render_hover_ring_radius(double radius_pixels)
{
    double ring = radius_pixels * 1.1 + 5.0;
    return ring > RENDER_RING_MARKER_RADIUS_PIXELS + 2.0 ? ring : RENDER_RING_MARKER_RADIUS_PIXELS + 2.0;
}

void render_edge_anchor(double dx, double dy, double half_width, double half_height, double inset, double *x, double *y)
{
    double w = fmax(half_width - inset, 1.0), h = fmax(half_height - inset, 1.0);
    if (fabs(dx) < 1e-12 && fabs(dy) < 1e-12) dy = 1.0;
    /* Scale the direction until it touches the first edge it meets. */
    double t = fmin(fabs(dx) > 1e-12 ? w / fabs(dx) : 1e300, fabs(dy) > 1e-12 ? h / fabs(dy) : 1e300);
    *x = dx * t;
    *y = dy * t;
}

double render_slide_past(double x, double y, double w, double h, const double blocker[4], bool vertical,
    double lo, double hi)
{
    const double gap = 6.0;
    bool overlaps = x < blocker[0] + blocker[2] && x + w > blocker[0] && y < blocker[1] + blocker[3] && y + h > blocker[1];
    if (!overlaps) return 0;
    double start = vertical ? y : x, size = vertical ? h : w;
    double before = (vertical ? blocker[1] : blocker[0]) - gap - size;            /* new start above/left */
    double after = (vertical ? blocker[1] + blocker[3] : blocker[0] + blocker[2]) + gap; /* below/right */
    bool before_fits = before >= lo && before + size <= hi, after_fits = after >= lo && after + size <= hi;
    if (before_fits && (!after_fits || start - before <= after - start)) return before - start;
    return after_fits ? after - start : 0;
}

static unsigned read_u16(const unsigned char *p) { return (unsigned)p[0] | (unsigned)p[1] << 8; }

size_t render_belt_decode(const unsigned char *bytes, size_t length, RenderBeltBody *out, size_t capacity)
{
    if (!bytes || length < 16 || memcmp(bytes, "SOLBELT1", 8) != 0) return 0;
    size_t count = (size_t)bytes[8] | (size_t)bytes[9] << 8 | (size_t)bytes[10] << 16 | (size_t)bytes[11] << 24;
    if (count > capacity || length != 16 + count * 16) return 0;
    const double turn = 2 * PI / 65535.0;
    for (size_t k = 0; k < count; ++k) {
        const unsigned char *r = bytes + 16 + k * 16;
        float a;
        memcpy(&a, r, sizeof(a)); /* little-endian IEEE single, as written */
        out[k] = (RenderBeltBody){a, (float)(read_u16(r + 4) / 65535.0), (float)(read_u16(r + 6) * PI / 65535.0),
            (float)(read_u16(r + 8) * turn), (float)(read_u16(r + 10) * turn), (float)(read_u16(r + 12) * turn), r[14]};
        if (!(a > 0) || !isfinite(a) || out[k].e >= 1.0f) return 0;
    }
    return count;
}

double render_belt_mean_motion(double a_au)
{
    double a = a_au * SOLAR_AU_METERS;
    return sqrt(SOLAR_SUN_GM_M3PS2 / (a * a * a)) * SOLAR_DAY_SECONDS;
}

Vec3d render_belt_position(const RenderBeltBody *body, double days)
{
    double e = body->e, a = body->a_au;
    double m = fmod(body->mean_anomaly0 + render_belt_mean_motion(a) * days, 2 * PI);
    /* Kepler's equation M = E - e sin E by Newton's method from E = M + e sin M:
     * six steps converge to below float precision for e < 0.97. */
    double E = m + e * sin(m);
    for (int k = 0; k < 6; ++k) E -= (E - e * sin(E) - m) / (1 - e * cos(E));
    double x = a * (cos(E) - e), y = a * sqrt(1 - e * e) * sin(E);
    /* Perifocal to ecliptic (3-1-3: periapsis, inclination, node), then to
     * simulation axes (x, y, z)_sim = (X, Z, -Y) as orbit_ecliptic_to_simulation. */
    double cw = cos(body->periapsis), sw = sin(body->periapsis), ci = cos(body->inclination), si = sin(body->inclination);
    double cn = cos(body->node), sn = sin(body->node);
    double X = x * (cw * cn - sw * sn * ci) - y * (sw * cn + cw * sn * ci);
    double Y = x * (cw * sn + sw * cn * ci) + y * (cw * cn * ci - sw * sn);
    double Z = x * (sw * si) + y * (cw * si);
    return (Vec3d){X, Z, -Y};
}

static const RenderRing uranus_rings[] = {
    {"6", 41837e3, 1.5e3, 0.3}, {"5", 42234e3, 2.0e3, 0.5}, {"4", 42571e3, 2.0e3, 0.3},
    {"alpha", 44718e3, 7.15e3, 0.4}, {"beta", 45661e3, 8.15e3, 0.3}, {"eta", 47176e3, 1.55e3, 0.4},
    {"gamma", 47627e3, 3.85e3, 1.5}, {"delta", 48300e3, 6.0e3, 0.5}, {"lambda", 50023e3, 2.0e3, 0.1},
    {"epsilon", 51149e3, 58.1e3, 1.5},
};
/* Le Verrier's width is given as "< 100 km"; 100 km is drawn. */
static const RenderRing neptune_rings[] = {
    {"Galle", 41900e3, 2000e3, 1e-4}, {"Le Verrier", 53200e3, 100e3, 0.01},
    {"Lassell", 55200e3, 4000e3, 1e-4}, {"Adams", 62933e3, 15e3, 0.01},
};
/* Edges from the fact sheet's radius ranges (centre and full width here). */
static const RenderRing jupiter_rings[] = {
    {"halo", 106200e3, 33600e3, 3e-6}, {"main", 125970e3, 5940e3, 5e-6},
    {"Amalthea gossamer", 155145e3, 52410e3, 1e-7}, {"Thebe gossamer", 201625e3, 40550e3, 1e-7},
};
static const RenderRingSystem ring_systems[RENDER_RING_SYSTEM_COUNT] = {
    {BODY_ID_URANUS, SOLAR_URANUS_RADIUS_M, 41500e3, 51400e3, 51400e3, 0.05, uranus_rings, sizeof(uranus_rings) / sizeof(uranus_rings[0]), {150, 148, 145}},
    {BODY_ID_NEPTUNE, SOLAR_NEPTUNE_RADIUS_M, 40800e3, 63100e3, 63100e3, 0.5, neptune_rings, sizeof(neptune_rings) / sizeof(neptune_rings[0]), {150, 138, 132}},
    {BODY_ID_JUPITER, SOLAR_JUPITER_RADIUS_M, 89400e3, 221900e3, 128940e3, 0.95, jupiter_rings, sizeof(jupiter_rings) / sizeof(jupiter_rings[0]), {175, 140, 110}},
};

const RenderRingSystem *render_ring_system_at(size_t index)
{
    return index < RENDER_RING_SYSTEM_COUNT ? &ring_systems[index] : NULL;
}

const RenderRingSystem *render_ring_system(BodyId planet)
{
    for (size_t k = 0; k < RENDER_RING_SYSTEM_COUNT; ++k) if (ring_systems[k].planet == planet) return &ring_systems[k];
    return NULL;
}

static void strip_pixel(const RenderRingSystem *system, unsigned char *rgba, double alpha)
{
    rgba[0] = system->rgb[0]; rgba[1] = system->rgb[1]; rgba[2] = system->rgb[2];
    rgba[3] = (unsigned char)lround((alpha > 1 ? 1 : alpha) * 255);
}

void render_ring_strip(const RenderRingSystem *system, unsigned char *rgba)
{
    const double step = (system->outer_m - system->inner_m) / RENDER_RING_STRIP_WIDTH;
    for (int x = 0; x < RENDER_RING_STRIP_WIDTH; ++x) {
        double lo = system->inner_m + x * step, hi = lo + step, alpha = 0;
        for (size_t k = 0; k < system->count; ++k) {
            const RenderRing *r = &system->rings[k];
            double a = fmax(lo, r->radius_m - r->width_m / 2), b = fmin(hi, r->radius_m + r->width_m / 2);
            /* Area coverage: mipmapping then thins narrow rings honestly
             * instead of widening them. */
            if (b > a) alpha += (b - a) / step * (1 - exp(-r->optical_depth));
        }
        strip_pixel(system, rgba + x * 4, alpha);
    }
}

void render_ring_strip_illustrative(const RenderRingSystem *system, unsigned char *rgba)
{
    const double step = (system->outer_m - system->inner_m) / RENDER_RING_ILLUSTRATIVE_WIDTH;
    for (int x = 0; x < RENDER_RING_ILLUSTRATIVE_WIDTH; ++x) {
        double lo = system->inner_m + x * step, hi = lo + step, alpha = 0;
        for (size_t k = 0; k < system->count; ++k) {
            const RenderRing *r = &system->rings[k];
            if (!(r->radius_m + r->width_m / 2 > lo && r->radius_m - r->width_m / 2 < hi)) continue;
            double level = 0.12 * (log10(r->optical_depth) + 7.5);
            if (r->width_m > 2 * step) level *= 0.35;
            alpha = fmax(alpha, fmin(0.85, fmax(0.03, level)));
        }
        strip_pixel(system, rgba + x * 4, alpha);
    }
}

bool render_scale_bar(double meters_per_pixel, double max_pixels, RenderScaleBar *out)
{
    if (!(meters_per_pixel > 0) || !(max_pixels > 0) || !isfinite(meters_per_pixel)) return false;
    double widest = meters_per_pixel * max_pixels;
    bool au = widest >= 0.1 * SOLAR_AU_METERS * 2; /* AU once a 0.1 AU bar fits comfortably */
    double unit = au ? SOLAR_AU_METERS : 1000.0;
    double span = widest / unit;
    double decade = pow(10.0, floor(log10(span)));
    double step = span >= 5 * decade ? 5 : span >= 2 * decade ? 2 : 1;
    double length = step * decade;
    out->length_m = length * unit;
    out->pixels = out->length_m / meters_per_pixel;
    if (au) {
        if (length >= 1) snprintf(out->label, sizeof(out->label), "%.0f AU", length);
        else snprintf(out->label, sizeof(out->label), "%.1f AU", length);
        return true;
    }
    if (length < 1) { snprintf(out->label, sizeof(out->label), "%.0f m", length * 1000); return true; }
    /* Group thousands with a space (the label font is ASCII). */
    char digits[24];
    snprintf(digits, sizeof(digits), "%.0f", length);
    size_t n = strlen(digits), o = 0;
    for (size_t k = 0; k < n && o + 2 < sizeof(out->label); ++k) {
        if (k > 0 && (n - k) % 3 == 0) out->label[o++] = ' ';
        out->label[o++] = digits[k];
    }
    snprintf(out->label + o, sizeof(out->label) - o, " km");
    return true;
}

size_t render_constellation_count(void)
{
    return RENDER_CONSTELLATION_SEGMENTS;
}

bool render_constellation_segment(size_t index, Vec3d *a, Vec3d *b)
{
    if (index >= RENDER_CONSTELLATION_SEGMENTS) return false;
    const float *s = constellation_segments[index];
    *a = orbit_icrf_direction(s[0], s[1]);
    *b = orbit_icrf_direction(s[2], s[3]);
    return true;
}

double render_clear_above(const double (*rects)[4], int count, double x0, double x1, double bottom)
{
    /* A panel blocks when it crosses the span and reaches down to the
     * current limit; lifting the limit can bring another panel into play, so
     * repeat until nothing moves (at most once per panel). */
    double limit = bottom;
    for (int pass = 0; pass <= count; ++pass) {
        bool moved = false;
        for (int k = 0; k < count; ++k) {
            const double *r = rects[k];
            bool crosses = r[0] < x1 && r[0] + r[2] > x0;
            if (crosses && r[1] < limit && r[1] + r[3] >= limit) { limit = r[1]; moved = true; }
        }
        if (!moved) break;
    }
    return limit;
}

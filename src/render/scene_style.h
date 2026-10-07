#ifndef SOLAR_SCENE_STYLE_H
#define SOLAR_SCENE_STYLE_H

/* Presentation policy for the cinematic renderer (SPEC A64-A68).
 *
 * Everything here decides how SI state *looks*: which map a body wears, how
 * its spin axis is drawn, how trails and the grid fade. Nothing here may feed
 * back into src/sim (V1, V10). Like render_scale.h, this header is raylib-free
 * so every rule can be unit-tested without opening a window or a GL context. */

#include <stdbool.h>
#include <stddef.h>

#include "../sim/body.h"
#include "../sim/vec3d.h"

/* Texture slots, in load order. Files live in assets/textures/ (native) and
 * are copied to the site's textures/ folder for the browser (A65). */
typedef enum RenderTextureSlot {
    RENDER_TEXTURE_NONE = -1,
    RENDER_TEXTURE_SUN = 0,
    RENDER_TEXTURE_MERCURY = 1,
    RENDER_TEXTURE_VENUS = 2,
    RENDER_TEXTURE_EARTH_DAY = 3,
    RENDER_TEXTURE_EARTH_NIGHT = 4,
    RENDER_TEXTURE_EARTH_CLOUDS = 5,
    RENDER_TEXTURE_MOON = 6,
    RENDER_TEXTURE_MARS = 7,
    RENDER_TEXTURE_JUPITER = 8,
    RENDER_TEXTURE_SATURN = 9,
    RENDER_TEXTURE_SATURN_RING = 10,
    RENDER_TEXTURE_URANUS = 11,
    RENDER_TEXTURE_NEPTUNE = 12,
    RENDER_TEXTURE_STARS = 13,
    RENDER_TEXTURE_COUNT = 14
} RenderTextureSlot;

/* File name (no directory) of a slot, or NULL for an invalid slot. */
const char *render_texture_file(RenderTextureSlot slot);
/* The surface map a body wears, or RENDER_TEXTURE_NONE for bodies without a
 * source-backed map (they stay honest lit colours rather than borrowed art). */
RenderTextureSlot render_texture_for_body(BodyId id);

/* Atmosphere rim glow: tint (0-1 RGB) and strength (0 = airless). */
typedef struct RenderAtmosphere {
    float r, g, b, strength;
} RenderAtmosphere;
RenderAtmosphere render_atmosphere_for_body(BodyId id);

/* Spin orientation in simulation axes (+Y = ecliptic north, see
 * orbit_ecliptic_to_simulation). `pole` is the body's IAU north pole and
 * `prime_meridian` points from the centre through longitude 0 on its equator.
 * Both are unit vectors and perpendicular. `known` is false for bodies without
 * an IAU model; they get +Y / +X so they still draw upright. */
typedef struct RenderOrientation {
    Vec3d pole;
    Vec3d prime_meridian;
    double spin_rate_degrees_per_day; /* dW/dt; negative = retrograde about the IAU pole */
    bool known;
} RenderOrientation;
/* IAU WGCCRE 2015 rotation model at `days_since_j2000` (TDB days, d in the
 * report). Periodic correction terms are omitted except Neptune's N term. */
RenderOrientation render_body_orientation(BodyId id, double days_since_j2000);

/* Opacity of trail point `index` of `count` (0 = oldest): old history fades
 * toward a floor so recent motion reads first without hiding the past. */
float render_trail_alpha(size_t index, size_t count);
/* Opacity of the reference grid at `distance` render units from its centre,
 * fading to zero at `half_width` so distant lines cannot alias into moiré. */
float render_grid_alpha(double distance, double half_width);
/* Adaptive reference grid for a camera `camera_distance` render units from
 * its target: minor lines every `minor_spacing`, major lines every ten minor
 * lines, both powers of ten so they read as round distances. As the camera
 * zooms out the minor lines fade (`minor_alpha` 1 -> 0) and are replaced by
 * the next decade, so the grid never jumps or crowds into moire. Lines fade
 * out by `radius`. */
typedef struct RenderGridLevels {
    double minor_spacing, major_spacing, minor_alpha, radius;
} RenderGridLevels;
RenderGridLevels render_grid_levels(double camera_distance);

/* Apparent radius in pixels of a sphere `radius` units across seen from
 * `distance` units away through a perspective camera with vertical field of
 * view `fovy_degrees` on a viewport `viewport_height` pixels tall. Bodies
 * smaller than a pixel are drawn as single points instead of full meshes:
 * a mesh draw costs dozens of WebGL calls, and most of the 115 Jovian moons
 * are sub-pixel in the overview. Returns 0 for degenerate input. */
double render_projected_radius_pixels(double radius, double distance, double fovy_degrees, double viewport_height);
#define RENDER_MESH_MIN_RADIUS_PIXELS 0.75

/* Julian date of the J2000.0 epoch (2000 January 1, 12:00 TDB). */
#define RENDER_J2000_JD 2451545.0

/* Normalized glow intensity at fraction `r` (0 = centre, 1 = edge) of the
 * Sun's halo billboard; 0 at and beyond the edge. */
float render_glow_intensity(double r);

/* UV sphere with radius 1, +Y north. Longitude λ (east-positive) maps to
 * position (cos λ cos φ, sin φ, -sin λ cos φ), matching the simulation's
 * proper rotation (ecliptic +Y = simulation -Z), and to u = (λ + π) / 2π so
 * standard equirectangular maps (longitude 0 at the image centre, north at the
 * top) wrap correctly. `slices` meridians by `rings` latitude bands. */
size_t render_sphere_vertex_count(int slices, int rings);
size_t render_sphere_index_count(int slices, int rings);
bool render_build_sphere(int slices, int rings, float *positions, float *normals, float *texcoords,
    unsigned short *indices);

/* Flat annulus in the body's equatorial (XZ) plane with normal +Y, radii in
 * body radii. u runs radially (0 = inner edge, 1 = outer) to match a ring
 * opacity strip; each segment duplicates its seam vertices so u never wraps. */
size_t render_ring_vertex_count(int segments);
size_t render_ring_index_count(int segments);
bool render_build_ring(int segments, float inner_radius, float outer_radius, float *positions,
    float *normals, float *texcoords, unsigned short *indices);

#endif

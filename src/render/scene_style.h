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
    /* Moon and small-world maps: USGS mosaics, public domain (A109). */
    RENDER_TEXTURE_PHOBOS = 14,
    RENDER_TEXTURE_VESTA = 15,
    RENDER_TEXTURE_IO = 16,
    RENDER_TEXTURE_EUROPA = 17,
    RENDER_TEXTURE_GANYMEDE = 18,
    RENDER_TEXTURE_CALLISTO = 19,
    RENDER_TEXTURE_ENCELADUS = 20,
    RENDER_TEXTURE_TETHYS = 21,
    RENDER_TEXTURE_DIONE = 22,
    RENDER_TEXTURE_RHEA = 23,
    RENDER_TEXTURE_IAPETUS = 24,
    RENDER_TEXTURE_TRITON = 25,
    RENDER_TEXTURE_PLUTO = 26,
    RENDER_TEXTURE_CHARON = 27,
    RENDER_TEXTURE_COUNT = 28
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
 * smaller than a pixel are drawn as small dots instead of full meshes:
 * a mesh draw costs dozens of WebGL calls, and most of the 115 Jovian moons
 * are sub-pixel in the overview. Returns 0 for degenerate input. */
double render_projected_radius_pixels(double radius, double distance, double fovy_degrees, double viewport_height);
#define RENDER_MESH_MIN_RADIUS_PIXELS 0.75
/* An unknown-radius wire marker (V25) needs room to read as a wire sphere.
 * Below RENDER_WIRE_MARKER_MIN_RADIUS_PIXELS its 144-vertex wireframe is a
 * blob a few pixels wide (the Saturn overview has 275 of them, more vertices
 * than every trail together), so it becomes a dot render_marker_dot_pixels
 * wide: about the blob's size, never smaller than any other dot. */
#define RENDER_WIRE_MARKER_MIN_RADIUS_PIXELS 3.0
bool render_marker_draws_wire(double radius_pixels);
double render_marker_dot_pixels(double radius_pixels);
/* World size of one screen pixel at `distance` (the inverse of the pinhole
 * projection above). Sub-pixel bodies are drawn as dots RENDER_DOT_PIXELS
 * wide in this many world units, so they stay visible but tiny. */
double render_world_units_per_pixel(double distance, double fovy_degrees, double viewport_height);
#define RENDER_DOT_PIXELS 2.5

/* Sample stride for drawing a trail of `point_count` samples whose on-screen
 * path length is about `extent_pixels`: roughly one drawn point every
 * RENDER_TRAIL_PIXELS_PER_POINT pixels (never fewer than
 * RENDER_TRAIL_MIN_POINTS), and never finer than `base_stride`, the renderer's
 * fixed segment budget. A Jovian moon's trail seen from Saturn spans a few
 * pixels and needs a handful of points, not a thousand. A 4 px chord on a
 * curve of radius 20 px strays 0.1 px from the arc, below what a 1 px line
 * shows; finer budgets (2 px, a 24-point floor) cost the 300-trail Saturn
 * scene a third of its frame rate on a throttled phone (SPEC A97). */
#define RENDER_TRAIL_PIXELS_PER_POINT 4.0
#define RENDER_TRAIL_MIN_POINTS 12
size_t render_trail_stride_for_extent(size_t point_count, size_t base_stride, double extent_pixels);

/* The four side planes of the camera's view pyramid, for skipping trail
 * segments that cannot appear on screen (presentation only). A segment is
 * invisible when both endpoints lie outside the same plane, i.e. when their
 * outcodes share a bit; that test never hides a visible segment, it only
 * misses some invisible ones. The planes pass through the camera, so a point
 * behind it is always outside at least one. `margin` widens the view (0.05
 * is 5%) so a line's width at the screen edge is never cut. */
typedef struct RenderFrustum {
    Vec3d apex;
    Vec3d normal[4];
} RenderFrustum;
RenderFrustum render_frustum(Vec3d apex, Vec3d forward, Vec3d up, double fovy_degrees, double aspect, double margin);
unsigned render_frustum_outcode(const RenderFrustum *frustum, Vec3d point);

/* Trim a trail segment a->b to the part outside a body's drawn sphere, so a
 * trail meets the planet's surface instead of running to its centre and
 * poking out of it. Returns false when the whole segment lies inside. Assumes
 * at most one endpoint is inside, which holds for a body's own trail. */
bool render_clip_segment_outside_sphere(Vec3d *a, Vec3d *b, Vec3d center, double radius);

/* Orbit guides (SPEC A107): the closed two-body conic (Kepler ellipse)
 * through a body's current parent-relative position and velocity, drawn as a
 * faint full orbit so the shape of every path reads at a glance. This is the
 * osculating orbit: it is what the body would follow if only its parent pulled
 * on it, so perturbations make it drift slowly; the trail is the measured
 * path. Presentation only, recomputed from SI state every frame and never
 * written back (V10).
 *
 * `periapsis` and `ahead` are unit vectors in the orbit plane: toward
 * periapsis and 90 degrees further along the motion. A point at eccentric
 * anomaly E sits at a (cos E - e) periapsis + b sin E ahead from the focus
 * (the parent), with b = a sqrt(1 - e^2). Meters. */
typedef struct RenderOrbitGuide {
    double semi_major_m, semi_minor_m, eccentricity;
    Vec3d periapsis, ahead;
} RenderOrbitGuide;
/* Fills `out` and returns true for a bound, non-degenerate orbit; open
 * (parabolic/hyperbolic), radial or massless cases return false and are not
 * drawn. mu = G (M_parent + m_body) in m^3/s^2. */
bool render_orbit_guide(Vec3d relative_position_m, Vec3d relative_velocity_mps, double mu, RenderOrbitGuide *out);
/* Focus-relative point (meters) at eccentric anomaly E, given cos E and
 * sin E: the renderer steps E with a rotation instead of calling cos/sin for
 * every vertex (WebAssembly trig is slow). */
Vec3d render_orbit_guide_point(const RenderOrbitGuide *guide, double cos_e, double sin_e);
/* Segments for a guide whose semi-major axis spans `semi_major_pixels` on
 * screen: about one per RENDER_ORBIT_GUIDE_PIXELS_PER_SEGMENT of
 * circumference within [MIN, MAX]; 0 when the orbit is too small to show a
 * shape (it would be a smudge over the body's dot). */
#define RENDER_ORBIT_GUIDE_MIN_PIXELS 6.0
/* A chord s pixels long on a curve of radius R strays s^2 / 8R from it: 10 px
 * chords stay within a quarter pixel once an orbit is 50 px across. */
#define RENDER_ORBIT_GUIDE_PIXELS_PER_SEGMENT 10.0
#define RENDER_ORBIT_GUIDE_MIN_SEGMENTS 24
#define RENDER_ORBIT_GUIDE_MAX_SEGMENTS 360
int render_orbit_guide_segments(double semi_major_pixels);

/* Eclipse shadows (SPEC A111). Seen from a surface point, an occluding sphere
 * of angular radius `occluder_radius` sits `separation` radians from the
 * Sun's centre, whose disc has angular radius `sun_radius`. Returns the
 * fraction of sunlight it removes: 1 in the umbra, 0 outside the penumbra,
 * a smooth ramp between, and at most (rho / sigma)^2 when the occluder is
 * smaller than the Sun (an annular eclipse). The lighting shader evaluates the
 * same expression; keep the two identical. */
double render_eclipse_fraction(double separation, double occluder_radius, double sun_radius);
#define RENDER_MAX_SHADOW_CASTERS 4
/* A caster for the shader, from true SI geometry (SPEC A111): `offset` is
 * the caster's position relative to the receiver and `radius` its radius,
 * both times `scale` (the receiver's drawn radius per true metre), so the
 * shader sees an exact scaled copy of the real configuration around the
 * drawn receiver. Returns false when the caster is not sunward or its shadow
 * cylinder, widened by the penumbra, misses the receiver; `gap` (scaled) says
 * how clearly it misses (more negative = nearer the shadow line). */
typedef struct RenderEclipseCaster {
    Vec3d offset;
    double radius, gap;
} RenderEclipseCaster;
bool render_eclipse_caster(Vec3d receiver_m, double receiver_radius_m, Vec3d caster_m, double caster_radius_m,
    Vec3d sun_m, double sun_radius_m, double scale, RenderEclipseCaster *out);

/* Screen markers (SPEC A108), drawn after the 3D scene. A planet or dwarf
 * planet smaller than RENDER_RING_MARKER_MAX_RADIUS_PIXELS on screen gets a
 * thin ring around its dot so it can be found among the stars; the selected
 * body gets four corner brackets `render_selection_bracket_half_size` pixels
 * from its centre (always outside its drawn disc). */
#define RENDER_RING_MARKER_MAX_RADIUS_PIXELS 4.0
#define RENDER_RING_MARKER_RADIUS_PIXELS 7.0
#define RENDER_SELECTION_MIN_HALF_PIXELS 13.0
/* Brackets around a body already filling much of the view only add noise;
 * the amber label and the inspector name it. */
#define RENDER_SELECTION_MAX_RADIUS_PIXELS 90.0
bool render_selection_wants_brackets(double radius_pixels);
bool render_body_wants_ring_marker(BodyKind kind, double radius_pixels);
double render_selection_bracket_half_size(double radius_pixels);

/* Click to select (SPEC A112). Each candidate is a body's projected centre,
 * its drawn radius in pixels, its kind and whether it is in front of the
 * camera. A click picks the body whose disc contains the pointer (the one
 * nearest its centre if several do); otherwise the body whose edge is within
 * RENDER_PICK_SLOP_PIXELS, where a planet beats a moon closer than 3 px of it
 * in score. Returns -1 when nothing is near. */
typedef struct RenderPickCandidate {
    float x, y, radius_pixels;
    BodyKind kind;
    bool in_front;
} RenderPickCandidate;
#define RENDER_PICK_SLOP_PIXELS 14.0f
int render_pick_body(const RenderPickCandidate *candidates, size_t count, float x, float y);

/* Resolution governor (SPEC A116). The browser draws at up to 2 backing
 * pixels per CSS pixel; a phone GPU that cannot fill that many pixels would
 * stutter. After a warm-up of RENDER_GOVERNOR_WARMUP_SECONDS, frames slower
 * than RENDER_GOVERNOR_SLOW_FRAME_SECONDS add their time to a debt that fast
 * frames pay back at twice the rate; RENDER_GOVERNOR_DEBT_SECONDS of debt
 * lowers the scale by 0.5 (never below 1). It never climbs back, so it cannot
 * oscillate. Frames over RENDER_GOVERNOR_STALL_SECONDS are stalls (hidden
 * tab, debugger) and are ignored: the app's own stall bound
 * (SOLAR_APP_STALL_FRAME_SECONDS), so a device running at 2 fps still counts.
 * A CPU-bound run (Saturn family at top speed) can also trip it: sharpness is
 * traded for frame rate, never simulation time. */
#define RENDER_GOVERNOR_WARMUP_SECONDS 3.0f
#define RENDER_GOVERNOR_SLOW_FRAME_SECONDS (1.0f / 48.0f)
#define RENDER_GOVERNOR_DEBT_SECONDS 2.0f
#define RENDER_GOVERNOR_STALL_SECONDS 1.0f
typedef struct RenderResolutionGovernor {
    float scale, elapsed_seconds, debt_seconds;
} RenderResolutionGovernor;
void render_resolution_governor_init(RenderResolutionGovernor *governor, float max_scale);
float render_resolution_governor_update(RenderResolutionGovernor *governor, float frame_seconds);

/* Name labels drawn in the canvas (presentation only). The Sun, planets and
 * dwarf planets are always named; the selected body always is; moons and small bodies are
 * named once their family is zoomed in (at least
 * RENDER_LABEL_MIN_SEPARATION_PIXELS from their parent on screen), when they
 * are drawn at least RENDER_LABEL_MIN_RADIUS_PIXELS across and have a known
 * size (unknown-radius wire markers stay unnamed unless selected). */
#define RENDER_LABEL_MIN_SEPARATION_PIXELS 36.0
#define RENDER_LABEL_MIN_RADIUS_PIXELS 2.0
bool render_body_wants_label(BodyKind kind, bool selected, bool known_radius, double radius_pixels,
    double parent_separation_pixels);

/* A label's screen rectangle and importance. render_declutter_labels keeps
 * the most important labels and hides any label that would overlap one
 * already placed, so a crowded inner solar system stays readable. */
typedef struct RenderLabelBox {
    float x, y, width, height;
    int priority;
    bool visible;
} RenderLabelBox;
/* Sets `visible`; a rejected label's priority is overwritten with INT_MIN. */
void render_declutter_labels(RenderLabelBox *boxes, size_t count);

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

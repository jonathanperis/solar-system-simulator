#include "require_assert.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/scene_style.h"
#include "sim/orbit.h"

static const double DEG = 3.14159265358979323846 / 180.0;

static double angle_degrees(Vec3d a, Vec3d b)
{
    double c = vec3d_dot(a, b) / (vec3d_length(a) * vec3d_length(b));
    if (c > 1) c = 1;
    if (c < -1) c = -1;
    return acos(c) / DEG;
}

static void test_texture_inventory_matches_assets(void)
{
    /* Every slot names a distinct file that exists in assets/textures/. The
     * browser fetches exactly these names, so a typo would silently fall back. */
    for (int i = 0; i < RENDER_TEXTURE_COUNT; ++i) {
        const char *name = render_texture_file((RenderTextureSlot)i);
        assert(name && strlen(name) > 4);
        for (int j = 0; j < i; ++j) assert(strcmp(name, render_texture_file((RenderTextureSlot)j)) != 0);
        char path[256];
        snprintf(path, sizeof(path), "assets/textures/%s", name);
        FILE *file = fopen(path, "rb");
        assert(file);
        fclose(file);
    }
    assert(render_texture_file(RENDER_TEXTURE_NONE) == NULL);
    assert(render_texture_file(RENDER_TEXTURE_COUNT) == NULL);

    assert(render_texture_for_body(BODY_ID_EARTH) == RENDER_TEXTURE_EARTH_DAY);
    assert(render_texture_for_body(BODY_ID_SATURN) == RENDER_TEXTURE_SATURN);
    assert(render_texture_for_body(BODY_ID_SUN) == RENDER_TEXTURE_SUN);
    /* Moon maps (A109); every mapped body has an IAU spin model so its map
     * is oriented, not spun at random. */
    assert(render_texture_for_body(BODY_ID_IO) == RENDER_TEXTURE_IO);
    assert(render_texture_for_body(BODY_ID_PHOBOS) == RENDER_TEXTURE_PHOBOS);
    assert(render_texture_for_body((BodyId)605) == RENDER_TEXTURE_RHEA);
    assert(render_texture_for_body(BODY_ID_PLUTO) == RENDER_TEXTURE_PLUTO);
    for (int id = -2; id < 1000; ++id)
        if (render_texture_for_body((BodyId)id) >= 0 && render_texture_for_body((BodyId)id) != RENDER_TEXTURE_STARS)
            assert(render_body_orientation((BodyId)id, 0).known);
    /* No visible-light map: these stay lit colours instead of borrowed art. */
    assert(render_texture_for_body(BODY_ID_TITAN) == RENDER_TEXTURE_NONE);
    assert(render_texture_for_body(BODY_ID_DEIMOS) == RENDER_TEXTURE_NONE);
    assert(render_texture_for_body((BodyId)601) == RENDER_TEXTURE_NONE);
}

static void test_atmospheres_only_where_bodies_have_air(void)
{
    assert(render_atmosphere_for_body(BODY_ID_EARTH).strength > 0.3f);
    assert(render_atmosphere_for_body(BODY_ID_VENUS).strength > 0);
    assert(render_atmosphere_for_body(BODY_ID_NEPTUNE).strength > 0);
    assert(render_atmosphere_for_body(BODY_ID_MOON).strength == 0);
    assert(render_atmosphere_for_body(BODY_ID_TITAN).strength > 0);
    assert(render_atmosphere_for_body(BODY_ID_GANYMEDE).strength == 0);
    assert(render_atmosphere_for_body(BODY_ID_MERCURY).strength == 0);
    RenderAtmosphere earth = render_atmosphere_for_body(BODY_ID_EARTH);
    assert(earth.b > earth.r); /* Rayleigh scattering reads blue */
}

static void test_orientation_vectors_are_orthonormal(void)
{
    const BodyId ids[] = {BODY_ID_SUN, BODY_ID_MERCURY, BODY_ID_VENUS, BODY_ID_EARTH, BODY_ID_MOON,
        BODY_ID_MARS, BODY_ID_JUPITER, BODY_ID_SATURN, BODY_ID_URANUS, BODY_ID_NEPTUNE,
        BODY_ID_IO, BODY_ID_TRITON, BODY_ID_PHOBOS, (BodyId)603, (BodyId)605, BODY_ID_PLUTO};
    for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
        for (double d = -5000; d <= 15000; d += 2500) {
            RenderOrientation o = render_body_orientation(ids[i], d);
            assert(o.known);
            assert(fabs(vec3d_length(o.pole) - 1) < 1e-12);
            assert(fabs(vec3d_length(o.prime_meridian) - 1) < 1e-12);
            assert(fabs(vec3d_dot(o.pole, o.prime_meridian)) < 1e-12);
        }
    }
    RenderOrientation unknown = render_body_orientation(BODY_ID_TITAN, 0);
    assert(!unknown.known);
    assert(unknown.pole.y == 1 && unknown.prime_meridian.x == 1);
}

static void test_spin_axes_reproduce_known_obliquities(void)
{
    /* Angle between each IAU north pole and ecliptic north (+Y). These are
     * ecliptic-referenced, so allow a few degrees against the familiar
     * orbit-referenced obliquities (orbits are inclined up to ~3.4 deg). */
    const Vec3d north = {0, 1, 0};
    struct { BodyId id; double obliquity; double tolerance; } cases[] = {
        {BODY_ID_EARTH, 23.44, 0.05}, {BODY_ID_MARS, 25.19, 2.0}, {BODY_ID_JUPITER, 3.13, 1.5},
        {BODY_ID_SATURN, 26.73, 2.5}, {BODY_ID_NEPTUNE, 28.32, 2.0}, {BODY_ID_MERCURY, 0.03, 7.5},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        double angle = angle_degrees(render_body_orientation(cases[i].id, 0).pole, north);
        assert(fabs(angle - cases[i].obliquity) <= cases[i].tolerance);
    }
    /* Retrograde rotators: the IAU defines "north" on the invariable plane's
     * north side, so the familiar obliquity belongs to the spin vector
     * sign(dW/dt) * pole. Uranus lies on its side (~98 deg), Venus is almost
     * upside down (~177 deg). */
    struct { BodyId id; double low, high; } retrograde[] = {{BODY_ID_URANUS, 95, 101}, {BODY_ID_VENUS, 174, 180}};
    for (size_t i = 0; i < 2; ++i) {
        RenderOrientation o = render_body_orientation(retrograde[i].id, 0);
        Vec3d spin = vec3d_scale(o.pole, o.spin_rate_degrees_per_day < 0 ? -1.0 : 1.0);
        double angle = angle_degrees(spin, north);
        assert(angle > retrograde[i].low && angle < retrograde[i].high);
    }
}

static void test_venus_and_uranus_spin_backwards(void)
{
    /* Spin angular velocity = (dW/dt) * pole. Seen from ecliptic north, Earth
     * turns prograde (+Y component); Venus and Uranus turn retrograde. */
    RenderOrientation earth = render_body_orientation(BODY_ID_EARTH, 0);
    RenderOrientation venus = render_body_orientation(BODY_ID_VENUS, 0);
    RenderOrientation uranus = render_body_orientation(BODY_ID_URANUS, 0);
    assert(earth.spin_rate_degrees_per_day * earth.pole.y > 0);
    assert(venus.spin_rate_degrees_per_day * venus.pole.y < 0);
    assert(uranus.spin_rate_degrees_per_day * uranus.pole.y < 0);
    /* A sidereal day: Earth's prime meridian returns after 0.99727 days. */
    RenderOrientation later = render_body_orientation(BODY_ID_EARTH, 360.0 / 360.9856235);
    /* Tolerance covers the slow pole precession and acos rounding near 1. */
    assert(angle_degrees(earth.prime_meridian, later.prime_meridian) < 1e-3);
    /* ...and has turned prograde (counterclockwise about +Y) after a quarter. */
    RenderOrientation quarter = render_body_orientation(BODY_ID_EARTH, 90.0 / 360.9856235);
    assert(vec3d_dot(vec3d_cross(earth.prime_meridian, quarter.prime_meridian), earth.pole) > 0.99);
}

static void test_greenwich_points_at_its_j2000_sidereal_angle(void)
{
    /* At J2000.0 Greenwich has right ascension ~280.46 deg (GMST). Undo the
     * simulation mapping (x, y, z) = ecliptic (X, Z, -Y), then rotate the
     * ecliptic back to the equator by +23.439 deg about X. */
    Vec3d m = render_body_orientation(BODY_ID_EARTH, 0).prime_meridian;
    double X = m.x, Y = -m.z, Z = m.y, e = 23.439291111 * DEG;
    double eq_y = cos(e) * Y - sin(e) * Z, eq_x = X;
    double ra = atan2(eq_y, eq_x) / DEG;
    if (ra < 0) ra += 360;
    assert(fabs(ra - 280.46) < 0.5);
}

static void test_trail_and_grid_fades(void)
{
    assert(render_trail_alpha(0, 1) == 1.0f);
    assert(render_trail_alpha(99, 100) == 1.0f);
    float previous = 0;
    for (size_t i = 0; i < 100; ++i) {
        float a = render_trail_alpha(i, 100);
        assert(a >= previous && a > 0.05f && a <= 1.0f);
        previous = a;
    }
    assert(render_trail_alpha(0, 100) < 0.3f);

    assert(render_grid_alpha(0, 10) > 0.2f);
    assert(render_grid_alpha(10, 10) == 0.0f);
    assert(render_grid_alpha(20, 10) == 0.0f);
    assert(render_grid_alpha(2, 10) >= render_grid_alpha(6, 10));
    assert(render_grid_alpha(6, 10) >= render_grid_alpha(9, 10));

    assert(render_glow_intensity(0) == 1.0f);
    assert(render_glow_intensity(1) == 0.0f && render_glow_intensity(2) == 0.0f);
    assert(render_glow_intensity(0.3) > render_glow_intensity(0.6));
}

static void test_grid_levels_cross_fade_by_decade(void)
{
    RenderGridLevels near = render_grid_levels(10.0);
    assert(fabs(near.minor_spacing - 1.0) < 1e-12 && fabs(near.major_spacing - 10.0) < 1e-12);
    assert(fabs(near.minor_alpha - 1.0) < 1e-12 && fabs(near.radius - 35.0) < 1e-12);
    /* Just below the next decade the minor lines have almost faded out, and
     * just past it the old major spacing becomes the new, fully opaque minor
     * spacing: the visible grid is continuous across the switch. */
    RenderGridLevels before = render_grid_levels(99.9), after = render_grid_levels(100.1);
    assert(before.minor_alpha < 0.01 && fabs(before.major_spacing - 10.0) < 1e-12);
    assert(fabs(after.minor_spacing - 10.0) < 1e-12 && after.minor_alpha > 0.99);
    /* Line counts stay bounded at every zoom: at most ~1000 minor lines across. */
    for (double d = 1e-4; d < 1e5; d *= 1.37) {
        RenderGridLevels g = render_grid_levels(d);
        assert(2 * g.radius / g.minor_spacing <= 1000.0 + 1e-6);
        assert(g.minor_alpha > 0 && g.minor_alpha <= 1);
    }
    assert(render_grid_levels(0).minor_spacing > 0); /* degenerate camera stays finite */
}

static void test_projected_radius_follows_pinhole_camera(void)
{
    /* A unit sphere filling half the view height at 90 deg: tan(45) = 1, so at
     * distance 1 the half-height spans 1 unit = 500 px on a 1000 px viewport. */
    assert(fabs(render_projected_radius_pixels(1, 1, 90, 1000) - 500) < 1e-9);
    assert(fabs(render_projected_radius_pixels(1, 10, 90, 1000) - 50) < 1e-9);
    /* A 0.012-unit moon 30 units away through the default 45 deg camera is
     * well under a pixel, so it is drawn as a point. */
    assert(render_projected_radius_pixels(0.012, 30, 45, 1000) < RENDER_MESH_MIN_RADIUS_PIXELS);
    assert(render_projected_radius_pixels(1, 0, 45, 1000) == 0);
    /* One pixel back in world units is the inverse: a body that many units
     * across projects to exactly one pixel. */
    double pixel = render_world_units_per_pixel(30, 45, 1000);
    assert(fabs(render_projected_radius_pixels(pixel / 2, 30, 45, 1000) - 0.5) < 1e-9);
    assert(render_world_units_per_pixel(0, 45, 1000) == 0);
    assert(render_projected_radius_pixels(-1, 1, 45, 1000) == 0);
}

static void test_labels_name_planets_and_zoomed_in_moons(void)
{
    assert(render_body_wants_label(BODY_KIND_STAR, false, true, 0.3, 0));
    assert(render_body_wants_label(BODY_KIND_PLANET, false, true, 0.3, 0));
    assert(render_body_wants_label(BODY_KIND_MOON, true, false, 0.1, 0)); /* selected */
    /* Through the telescope (A149) the target's moons are named however
     * small, once they stand clear of the planet; unknown radii stay
     * unnamed markers. */
    assert(render_moon_spread_factor(BODY_ID_MARS) == 40.0);
    assert(render_moon_spread_factor(BODY_ID_JUPITER) == 1.0);
    assert(render_sky_moon_wants_label(true, RENDER_LABEL_MIN_SEPARATION_PIXELS + 1));
    assert(!render_sky_moon_wants_label(true, RENDER_LABEL_MIN_SEPARATION_PIXELS - 1));
    assert(!render_sky_moon_wants_label(false, 500));
    /* A moon tucked against its parent in the overview stays unnamed... */
    assert(!render_body_wants_label(BODY_KIND_MOON, false, true, 5, 10));
    /* ...as does a sub-pixel one, but a visible moon of a framed family is named. */
    assert(!render_body_wants_label(BODY_KIND_MOON, false, true, 1, 200));
    assert(render_body_wants_label(BODY_KIND_MOON, false, true, 4, 200));
    assert(render_body_wants_label(BODY_KIND_ASTEROID, false, true, 3, 80));
    /* Unknown-size tracer moons (wire markers) stay unnamed unless selected. */
    assert(!render_body_wants_label(BODY_KIND_MOON, false, false, 4, 200));
}

static void test_declutter_keeps_the_most_important_labels(void)
{
    RenderLabelBox boxes[4] = {
        {100, 100, 60, 16, 10, false},  /* low priority, overlaps the Sun's label */
        {110, 104, 40, 16, 90, false},  /* Sun */
        {300, 100, 50, 16, 80, false},  /* a planet far away */
        {305, 110, 50, 16, 80, false},  /* equal priority overlap: first one wins */
    };
    render_declutter_labels(boxes, 4);
    assert(!boxes[0].visible && boxes[1].visible && boxes[2].visible && !boxes[3].visible);
    RenderLabelBox apart[2] = {{0, 0, 10, 10, 1, false}, {30, 0, 10, 10, 1, false}};
    render_declutter_labels(apart, 2);
    assert(apart[0].visible && apart[1].visible);
}

static void test_trail_detail_follows_screen_extent(void)
{
    /* A full 1025-point trail along 8000 px of path keeps every sample;
     * along 4000 px (1000 points at 4 px spacing) every second one... */
    assert(render_trail_stride_for_extent(1025, 1, 8000) == 1);
    assert(render_trail_stride_for_extent(1025, 1, 4000) == 2);
    /* ...one spanning 100 px needs ~25 points (4 px apart: on a curve of
     * radius 20 px a 4 px chord strays 0.1 px from the arc)... */
    size_t stride = render_trail_stride_for_extent(1025, 1, 100);
    assert(stride >= 40 && stride <= 42 && 1024 / stride >= 24);
    /* ...a sub-pixel trail keeps the 12-point floor so its shape survives,
     * and the renderer's own segment budget is never undercut. */
    assert(1024 / render_trail_stride_for_extent(1025, 1, 0.5) >= RENDER_TRAIL_MIN_POINTS - 1);
    assert(render_trail_stride_for_extent(5000, 5, 4000) == 5);
    assert(render_trail_stride_for_extent(10, 1, 1) == 1);
    assert(render_trail_stride_for_extent(1025, 1, -1) == 1);
}

static void test_small_wire_markers_become_dots(void)
{
    /* A marker that reads as a wireframe keeps it; a few-pixel blob becomes
     * a dot of about the same size, never smaller than any other dot. */
    assert(render_marker_draws_wire(3.0) && render_marker_draws_wire(40.0));
    assert(!render_marker_draws_wire(2.9) && !render_marker_draws_wire(0.0));
    assert(render_marker_dot_pixels(2.0) == 3.0);
    assert(render_marker_dot_pixels(0.5) == RENDER_DOT_PIXELS);
}

static void test_frustum_outcodes_skip_only_invisible_segments(void)
{
    /* Camera at (0, 0, 10) looking down -z with +y up, 60 degree vertical
     * field of view, a 2:1 image: tan(30 deg) = 0.577 up, 1.155 sideways. */
    RenderFrustum f = render_frustum((Vec3d){0, 0, 10}, (Vec3d){0, 0, -1}, (Vec3d){0, 1, 0}, 60, 2, 0);
    assert(render_frustum_outcode(&f, (Vec3d){0, 0, 0}) == 0);
    assert(render_frustum_outcode(&f, (Vec3d){10, 5, 0}) == 0);      /* inside, near the corner */
    unsigned right = render_frustum_outcode(&f, (Vec3d){12, 0, 0});
    unsigned left = render_frustum_outcode(&f, (Vec3d){-12, 0, 0});
    unsigned above = render_frustum_outcode(&f, (Vec3d){0, 6, 0});
    assert(right && left && above && !(right & left) && !(right & above));
    /* Behind the camera is outside some side plane. */
    assert(render_frustum_outcode(&f, (Vec3d){0, 0, 20}) != 0);
    /* Both ends right of the view: skipped. Ends on opposite sides of the
     * view: the segment crosses the screen, so it must be kept. */
    assert(right & render_frustum_outcode(&f, (Vec3d){30, 1, -5}));
    assert(!(right & left));
    /* The margin widens the view: a point just outside becomes inside. */
    RenderFrustum wide = render_frustum((Vec3d){0, 0, 10}, (Vec3d){0, 0, -1}, (Vec3d){0, 1, 0}, 60, 2, 0.05);
    assert(render_frustum_outcode(&f, (Vec3d){0, 5.9, 0}) != 0 && render_frustum_outcode(&wide, (Vec3d){0, 5.9, 0}) == 0);
}

static void test_trail_segments_stop_at_the_body_surface(void)
{
    Vec3d center = {0, 0, 0};
    /* Outside -> centre: the end moves out to the surface along the segment. */
    Vec3d a = {3, 0, 0}, b = {0, 0, 0};
    assert(render_clip_segment_outside_sphere(&a, &b, center, 1.0));
    assert(fabs(a.x - 3) < 1e-12 && fabs(b.x - 1) < 1e-12 && b.y == 0 && b.z == 0);
    /* Centre -> outside: the start moves out instead. */
    a = (Vec3d){0, 0, 0}; b = (Vec3d){0, 0, -4};
    assert(render_clip_segment_outside_sphere(&a, &b, center, 2.0));
    assert(fabs(a.z + 2) < 1e-12 && fabs(b.z + 4) < 1e-12);
    /* Entirely outside: untouched. Entirely inside: dropped. */
    a = (Vec3d){5, 0, 0}; b = (Vec3d){6, 1, 0};
    assert(render_clip_segment_outside_sphere(&a, &b, center, 1.0) && a.x == 5 && b.x == 6);
    a = (Vec3d){0.1, 0, 0}; b = (Vec3d){0.2, 0, 0};
    assert(!render_clip_segment_outside_sphere(&a, &b, center, 1.0));
}

static void test_sphere_matches_simulation_handedness_and_map_layout(void)
{
    const int slices = 16, rings = 8;
    size_t vertices = render_sphere_vertex_count(slices, rings);
    size_t indices = render_sphere_index_count(slices, rings);
    assert(vertices == (size_t)(slices + 1) * (rings + 1));
    assert(indices == (size_t)slices * rings * 6);
    float positions[17 * 9 * 3], normals[17 * 9 * 3], texcoords[17 * 9 * 2];
    unsigned short index[16 * 8 * 6];
    assert(render_build_sphere(slices, rings, positions, normals, texcoords, index));
    for (size_t i = 0; i < vertices; ++i) {
        float *p = &positions[i * 3], *n = &normals[i * 3];
        assert(fabsf(p[0] * p[0] + p[1] * p[1] + p[2] * p[2] - 1) < 1e-5f);
        assert(fabsf(p[0] - n[0]) < 1e-6f && fabsf(p[1] - n[1]) < 1e-6f && fabsf(p[2] - n[2]) < 1e-6f);
        assert(texcoords[i * 2] >= 0 && texcoords[i * 2] <= 1 && texcoords[i * 2 + 1] >= 0 && texcoords[i * 2 + 1] <= 1);
    }
    for (size_t i = 0; i < indices; ++i) assert(index[i] < vertices);
    /* Row 0 is the north pole (+Y) at the top of the image (v = 0). */
    assert(fabsf(positions[1] - 1) < 1e-6f && texcoords[1] == 0.0f);
    /* Image centre (u = 0.5) on the equator is longitude 0, pointing +X;
     * a quarter turn east (u = 0.75) points -Z, i.e. ecliptic +Y. */
    size_t equator = (size_t)(rings / 2) * (slices + 1);
    float *zero = &positions[(equator + slices / 2) * 3];
    float *east = &positions[(equator + slices * 3 / 4) * 3];
    assert(fabsf(texcoords[(equator + slices / 2) * 2] - 0.5f) < 1e-6f);
    assert(zero[0] > 0.999f && fabsf(zero[2]) < 1e-5f);
    assert(east[2] < -0.999f);
    /* Triangles wind counterclockwise seen from outside (OpenGL front faces). */
    for (size_t t = 0; t < indices; t += 3) {
        float *a = &positions[index[t] * 3], *b = &positions[index[t + 1] * 3], *c = &positions[index[t + 2] * 3];
        Vec3d ab = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, ac = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        Vec3d centroid = {(a[0] + b[0] + c[0]) / 3, (a[1] + b[1] + c[1]) / 3, (a[2] + b[2] + c[2]) / 3};
        Vec3d normal = vec3d_cross(ab, ac);
        if (vec3d_length(normal) > 1e-9) assert(vec3d_dot(normal, centroid) > 0);
    }
}

static void test_ring_annulus_spans_requested_radii(void)
{
    const int segments = 12;
    size_t vertices = render_ring_vertex_count(segments), indices = render_ring_index_count(segments);
    assert(vertices == (size_t)(segments + 1) * 2 && indices == (size_t)segments * 6);
    float positions[26 * 3], normals[26 * 3], texcoords[26 * 2];
    unsigned short index[12 * 6];
    assert(render_build_ring(segments, 1.15f, 2.4f, positions, normals, texcoords, index));
    for (size_t i = 0; i < vertices; ++i) {
        float r = sqrtf(positions[i * 3] * positions[i * 3] + positions[i * 3 + 2] * positions[i * 3 + 2]);
        bool inner = (i % 2) == 0;
        assert(fabsf(r - (inner ? 1.15f : 2.4f)) < 1e-5f);
        assert(positions[i * 3 + 1] == 0 && normals[i * 3 + 1] == 1);
        assert(texcoords[i * 2] == (inner ? 0.0f : 1.0f));
    }
    for (size_t i = 0; i < indices; ++i) assert(index[i] < vertices);
    assert(!render_build_ring(2, 1.15f, 2.4f, positions, normals, texcoords, index));
    assert(!render_build_ring(segments, 2.4f, 1.15f, positions, normals, texcoords, index));
}

/* Orbit guides (SPEC A107): the two-body conic through a body's current
 * parent-relative state, drawn for reading only. */
static void test_orbit_guide_reproduces_the_conic_through_the_state(void)
{
    const double mu = 1.32712440018e20, q = 4.6e10, e = 0.2056;
    OrbitState perihelion;
    assert(orbit_state(q, e, mu, 0.0, &perihelion));
    /* Tilt the orbit so the guide is checked off the reference plane too. */
    Vec3d r = orbit_orient(perihelion.position_m, 7.0, 48.3, 29.1);
    Vec3d v = orbit_orient(perihelion.velocity_mps, 7.0, 48.3, 29.1);
    RenderOrbitGuide guide;
    assert(render_orbit_guide(r, v, mu, &guide));
    double a = q / (1 - e);
    assert(fabs(guide.semi_major_m - a) / a < 1e-9);
    assert(fabs(guide.eccentricity - e) < 1e-9);
    /* E = 0 is periapsis (where the body is), E = pi is apoapsis. */
    Vec3d periapsis = render_orbit_guide_point(&guide, 1.0, 0.0);
    assert(vec3d_length(vec3d_sub(periapsis, r)) / q < 1e-9);
    assert(fabs(vec3d_length(render_orbit_guide_point(&guide, -1.0, 0.0)) - a * (1 + e)) / a < 1e-9);
    /* Every point lies in the orbit plane (normal r x v) at the conic radius
     * a (1 - e cos E). */
    Vec3d normal = vec3d_cross(r, v);
    for (int k = 0; k < 16; ++k) {
        double E = k * 2 * 3.14159265358979323846 / 16;
        Vec3d point = render_orbit_guide_point(&guide, cos(E), sin(E));
        assert(fabs(vec3d_dot(point, normal)) / (vec3d_length(point) * vec3d_length(normal)) < 1e-9);
        assert(fabs(vec3d_length(point) - a * (1 - e * cos(E))) / a < 1e-9);
    }
    /* Seen from the orbit normal the guide turns the way the body moves:
     * a quarter turn after periapsis points along the velocity there. */
    Vec3d ahead = render_orbit_guide_point(&guide, 0.0, 1.0);
    assert(vec3d_dot(vec3d_sub(ahead, periapsis), v) > 0);
}

static void test_orbit_guide_skips_open_or_degenerate_orbits(void)
{
    RenderOrbitGuide guide;
    const double mu = 3.986004418e14, r = 7.0e6;
    double escape = sqrt(2 * mu / r);
    assert(!render_orbit_guide((Vec3d){r, 0, 0}, (Vec3d){0, 0, -1.01 * escape}, mu, &guide));
    assert(!render_orbit_guide((Vec3d){r, 0, 0}, (Vec3d){0, 0, -escape}, mu, &guide));
    assert(!render_orbit_guide((Vec3d){r, 0, 0}, (Vec3d){-1000, 0, 0}, mu, &guide)); /* radial: no plane */
    assert(!render_orbit_guide((Vec3d){r, 0, 0}, (Vec3d){0, 0, -7000}, 0.0, &guide));
    assert(!render_orbit_guide((Vec3d){0, 0, 0}, (Vec3d){0, 0, -7000}, mu, &guide));
    assert(render_orbit_guide((Vec3d){r, 0, 0}, (Vec3d){0, 0, -sqrt(mu / r)}, mu, &guide));
    assert(guide.eccentricity < 1e-9 && fabs(guide.semi_major_m - r) < 1e-3);
}

static void test_orbit_guide_detail_follows_screen_size(void)
{
    assert(render_orbit_guide_segments(0.0) == 0);
    assert(render_orbit_guide_segments(RENDER_ORBIT_GUIDE_MIN_PIXELS * 0.99) == 0);
    int small = render_orbit_guide_segments(RENDER_ORBIT_GUIDE_MIN_PIXELS);
    int large = render_orbit_guide_segments(2000.0);
    assert(small >= RENDER_ORBIT_GUIDE_MIN_SEGMENTS && large <= RENDER_ORBIT_GUIDE_MAX_SEGMENTS && large > small);
    assert(render_orbit_guide_segments(1e9) == RENDER_ORBIT_GUIDE_MAX_SEGMENTS);
    for (double px = 10; px < 5000; px *= 1.7)
        assert(render_orbit_guide_segments(px * 1.7) >= render_orbit_guide_segments(px));
}

/* Screen markers (SPEC A108): ring markers keep dot-sized planets findable;
 * the selection brackets always clear the drawn body. */
static void test_markers_find_small_planets_and_frame_the_selection(void)
{
    assert(render_body_wants_ring_marker(BODY_KIND_PLANET, 1.0));
    assert(render_body_wants_ring_marker(BODY_KIND_DWARF_PLANET, 2.0));
    assert(!render_body_wants_ring_marker(BODY_KIND_PLANET, RENDER_RING_MARKER_MAX_RADIUS_PIXELS));
    assert(!render_body_wants_ring_marker(BODY_KIND_MOON, 1.0));
    assert(!render_body_wants_ring_marker(BODY_KIND_STAR, 1.0));
    double previous = 0;
    for (double radius = 0; radius < 400; radius += 7) {
        double half = render_selection_bracket_half_size(radius);
        assert(half > radius && half >= RENDER_SELECTION_MIN_HALF_PIXELS && half >= previous);
        previous = half;
    }
    assert(render_selection_wants_brackets(0) && render_selection_wants_brackets(40));
    for (double radius = 0; radius < 300; radius += 11)
        assert(render_hover_ring_radius(radius) > radius && render_hover_ring_radius(radius) > RENDER_RING_MARKER_RADIUS_PIXELS);
    assert(!render_selection_wants_brackets(RENDER_SELECTION_MAX_RADIUS_PIXELS));
}

/* Eclipse shadows (SPEC A111): the fraction of the Sun's disc an occluding
 * sphere hides, as the lighting shader computes it. */
static void test_eclipse_fraction_covers_umbra_penumbra_and_annulus(void)
{
    const double sun = 0.0025; /* ~ the Sun's angular radius at Earth, rad */
    /* A large occluder centred on the Sun: total eclipse (umbra). */
    assert(fabs(render_eclipse_fraction(0, 0.01, sun) - 1) < 1e-12);
    /* Separated by more than both radii: no shadow. */
    assert(render_eclipse_fraction(0.01 + sun + 1e-6, 0.01, sun) == 0);
    /* Smaller than the Sun and centred: an annulus hides (rho/sigma)^2. */
    assert(fabs(render_eclipse_fraction(0, 0.5 * sun, sun) - 0.25) < 1e-12);
    /* The fraction never grows as the occluder moves away. */
    double previous = 1;
    for (double theta = 0; theta < 0.02; theta += 0.0002) {
        double f = render_eclipse_fraction(theta, 0.01, sun);
        assert(f >= 0 && f <= previous + 1e-15);
        previous = f;
    }
    assert(render_eclipse_fraction(0, 0, sun) == 0 && render_eclipse_fraction(0, 0.01, 0) == 1);
}

/* Click to select (SPEC A112): the body under the pointer, else the nearest
 * one within a finger-sized slop; planets win over the moons beside them. */
static void test_pick_prefers_the_body_under_the_pointer(void)
{
    RenderPickCandidate bodies[] = {
        {100, 100, 40, BODY_KIND_PLANET, true},   /* 0: a big disc */
        {130, 100, 2, BODY_KIND_MOON, true},      /* 1: a moon dot inside the disc's edge */
        {300, 300, 0.5, BODY_KIND_PLANET, true},  /* 2: a dot-sized planet */
        {306, 300, 0.5, BODY_KIND_MOON, true},    /* 3: its moon, 6 px away */
        {500, 500, 1, BODY_KIND_MOON, false},     /* 4: behind the camera */
    };
    size_t n = sizeof(bodies) / sizeof(bodies[0]);
    assert(render_pick_body(bodies, n, 100, 100) == 0);
    assert(render_pick_body(bodies, n, 131, 100) == 1); /* the moon dot under the pointer */
    assert(render_pick_body(bodies, n, 303, 301) == 2); /* between two dots: the planet */
    assert(render_pick_body(bodies, n, 307, 300) == 3); /* right on the moon */
    assert(render_pick_body(bodies, n, 500, 500) == -1);
    assert(render_pick_body(bodies, n, 700, 50) == -1);
    assert(render_pick_body(bodies, n, 306.5f + RENDER_PICK_SLOP_PIXELS + 1, 300) == -1);
}

/* Resolution governor (SPEC A116): sustained slow frames step the HiDPI
 * backing scale down, never below 1; fast devices keep it. */
static void test_resolution_governor_steps_down_only_under_sustained_load(void)
{
    RenderResolutionGovernor g;
    render_resolution_governor_init(&g, 2.0f);
    /* Warm-up (texture uploads, first layout) is ignored. */
    for (int i = 0; i < 60; ++i) render_resolution_governor_update(&g, 0.05f);
    assert(render_resolution_governor_update(&g, 0.016f) == 2.0f);
    /* A fast device stays sharp indefinitely, a brief hitch changes nothing,
     * and stalls (hidden tab, debugger) are ignored. */
    for (int i = 0; i < 3600; ++i) render_resolution_governor_update(&g, 1.0f / 60.0f);
    for (int i = 0; i < 20; ++i) render_resolution_governor_update(&g, 0.04f);
    render_resolution_governor_update(&g, 5.0f);
    assert(render_resolution_governor_update(&g, 1.0f / 60.0f) == 2.0f);
    /* Sustained 30 fps: one step down, then another, never below 1. */
    float scale = 2.0f;
    for (int i = 0; i < 90; ++i) scale = render_resolution_governor_update(&g, 1.0f / 30.0f);
    assert(scale == 1.5f);
    for (int i = 0; i < 1000; ++i) scale = render_resolution_governor_update(&g, 1.0f / 20.0f);
    assert(scale == 1.0f);
    {
        /* A device at 2 fps (0.5 s frames) is slow, not stalled: it steps down. */
        RenderResolutionGovernor slow;
        render_resolution_governor_init(&slow, 2.0f);
        float s2 = 2.0f;
        for (int i = 0; i < 20; ++i) s2 = render_resolution_governor_update(&slow, 0.5f);
        assert(s2 < 2.0f);
    }
    /* It does not climb back (no oscillation). */
    for (int i = 0; i < 3600; ++i) scale = render_resolution_governor_update(&g, 1.0f / 120.0f);
    assert(scale == 1.0f);
}

/* Eclipse casters from true geometry (SPEC A111): a scaled copy of the real
 * configuration around the receiver, so shadows occur only when they really
 * do, at their true size relative to the receiver. */
static void test_eclipse_caster_is_a_scaled_copy_of_the_true_geometry(void)
{
    const Vec3d receiver = {1e12, 0, 0}, sun = {0, 0, 0};
    RenderEclipseCaster c;
    /* A moon 4e8 m sunward of a 7e7 m planet, on the shadow line. */
    Vec3d moon = {1e12 - 4e8, 0, 0};
    assert(render_eclipse_caster(receiver, 7e7, moon, 1.8e6, sun, 6.957e8, 2e-9, &c));
    assert(fabs(c.offset.x - (-4e8 * 2e-9)) < 1e-12 && fabs(c.radius - 1.8e6 * 2e-9) < 1e-15);
    /* Same moon well off the line: no shadow can reach the planet. */
    Vec3d off = {1e12 - 4e8, 3e8, 0};
    assert(!render_eclipse_caster(receiver, 7e7, off, 1.8e6, sun, 6.957e8, 2e-9, &c));
    /* Anti-sunward bodies cast nothing. */
    Vec3d behind = {1e12 + 4e8, 0, 0};
    assert(!render_eclipse_caster(receiver, 7e7, behind, 1.8e6, sun, 6.957e8, 2e-9, &c));
    /* The penumbra widens the reach: just outside the umbra cylinder still counts. */
    Vec3d edge = {1e12 - 4e8, 7e7 + 1.8e6 + 0.5 * 4e8 * 6.957e8 / 1e12, 0};
    assert(render_eclipse_caster(receiver, 7e7, edge, 1.8e6, sun, 6.957e8, 1.0, &c));
}

/* The Milky Way map is a real all-sky image in galactic coordinates
 * (longitude increasing leftward, south galactic pole at the top). Oriented
 * by render_sky_orientation (SPEC A117), each star of the map lies in its true
 * direction: Sirius and Vega land where the map has them. */
static void test_sky_map_puts_stars_in_their_true_directions(void)
{
    RenderOrientation sky = render_sky_orientation();
    assert(fabs(vec3d_length(sky.pole) - 1) < 1e-12 && fabs(vec3d_length(sky.prime_meridian) - 1) < 1e-12);
    assert(fabs(vec3d_dot(sky.pole, sky.prime_meridian)) < 1e-9);
    const struct { double ra, dec, l, b; } stars[] = {
        {101.2872, -16.7161, 227.23, -8.89},  /* Sirius */
        {279.2347, 38.7837, 67.45, 19.24},    /* Vega */
        {95.9880, -52.6957, 261.21, -25.29},  /* Canopus */
    };
    Vec3d z = vec3d_cross(sky.prime_meridian, sky.pole);
    for (size_t k = 0; k < 3; ++k) {
        Vec3d d = orbit_icrf_direction(stars[k].ra, stars[k].dec);
        /* Body-frame coordinates, then the sphere's (u, v) as render_build_sphere lays them out. */
        double x = vec3d_dot(d, sky.prime_meridian), y = vec3d_dot(d, sky.pole), zz = vec3d_dot(d, z);
        double longitude = atan2(-zz, x), latitude = asin(y);
        double u = (longitude + 3.14159265358979323846) / (2 * 3.14159265358979323846);
        double v = 0.5 - latitude / 3.14159265358979323846;
        /* The map: u = 0.5 - l/360 (l wrapped to +-180), v = 0.5 + b/180 (south up). */
        double l = fmod(stars[k].l + 180.0, 360.0) - 180.0;
        double map_u = 0.5 - l / 360.0, map_v = 0.5 + stars[k].b / 180.0;
        assert(fabs(u - map_u) < 0.002 && fabs(v - map_v) < 0.002);
    }
}

/* Off-screen markers (SPEC A119) sit where the direction toward the body
 * leaves an inset rectangle around the view's centre. */
static void test_edge_anchor_follows_the_direction_to_the_body(void)
{
    double x, y;
    render_edge_anchor(10, 0, 600, 400, 30, &x, &y);
    assert(fabs(x - 570) < 1e-9 && fabs(y) < 1e-9);
    render_edge_anchor(0, -5, 600, 400, 30, &x, &y);
    assert(fabs(x) < 1e-9 && fabs(y + 370) < 1e-9);
    /* Diagonal: it meets the nearer (top/bottom) edge first on a wide view. */
    render_edge_anchor(1, 1, 600, 400, 30, &x, &y);
    assert(fabs(y - 370) < 1e-9 && fabs(x - 370) < 1e-9);
    /* Straight behind (no lateral offset): pointed down, never NaN. */
    render_edge_anchor(0, 0, 600, 400, 30, &x, &y);
    assert(fabs(x) < 1e-9 && fabs(y - 370) < 1e-9);
}

/* Off-screen markers slide past page panels floating over the canvas (A119). */
static void test_markers_slide_past_floating_panels(void)
{
    const double dock[4] = {300, 700, 600, 80}; /* bottom dock */
    /* A marker on the bottom edge under the dock moves sideways to the nearer free side. */
    double dx = render_slide_past(400, 740, 80, 20, dock, false, 10, 1270);
    assert(dx < 0 && 400 + dx + 80 <= 300);
    double dx2 = render_slide_past(820, 740, 80, 20, dock, false, 10, 1270);
    assert(dx2 > 0 && 820 + dx2 >= 900);
    /* Clear of the panel: unchanged. */
    assert(render_slide_past(100, 740, 80, 20, dock, false, 10, 1270) == 0);
    /* A right-edge marker under a top-right panel slides down below it. */
    const double inspector[4] = {920, 90, 350, 260};
    double dy = render_slide_past(1180, 150, 90, 20, inspector, true, 10, 790);
    assert(dy != 0 && (150 + dy + 20 <= 90 || 150 + dy >= 350)); /* clear above or below */
}

/* Small-body cloud (SPEC A121): the binary sample decodes, and the position
 * the vertex shader computes (render_belt_position mirrors it) is the same
 * heliocentric conic the atlas uses (catalog_coordinate). */
static void test_belt_sample_decodes_and_matches_the_atlas_conic(void)
{
    FILE *file = fopen("assets/belt/small_bodies.bin", "rb");
    assert(file);
    static unsigned char bytes[2 * 1024 * 1024];
    size_t length = fread(bytes, 1, sizeof(bytes), file);
    fclose(file);
    static RenderBeltBody bodies[RENDER_BELT_CAPACITY];
    size_t count = render_belt_decode(bytes, length, bodies, RENDER_BELT_CAPACITY);
    assert(count > 40000 && count <= RENDER_BELT_CAPACITY);
    size_t trojans = 0, belt = 0;
    for (size_t k = 0; k < count; ++k) {
        assert(bodies[k].a_au > 0.1f && bodies[k].a_au <= 1000.0f && bodies[k].e >= 0 && bodies[k].e < 0.97f);
        trojans += bodies[k].kind == RENDER_BELT_TROJAN;
        belt += bodies[k].kind == RENDER_BELT_MAIN && bodies[k].a_au > 2.1f && bodies[k].a_au < 3.3f;
    }
    assert(trojans > 5000 && belt > 15000);
    /* Corrupt input is refused, not half-read. */
    assert(render_belt_decode(bytes, 10, bodies, RENDER_BELT_CAPACITY) == 0);
    bytes[0] = 'X';
    assert(render_belt_decode(bytes, length, bodies, RENDER_BELT_CAPACITY) == 0);

    /* 588 Achilles from the atlas, propagated 0 and 3,000 days. */
    const double q = 4.443339755266702, e = 0.148117524873686, inc = 10.31507282705955;
    const double node = 316.5298906019931, peri = 134.3146427558605, tp = 2460072.687722241, epoch = 2461200.5;
    double a = q / (1 - e), n = render_belt_mean_motion(a);
    RenderBeltBody achilles = {(float)a, (float)e, (float)(inc * DEG), (float)(node * DEG), (float)(peri * DEG),
        (float)fmod(n * (epoch - tp), 2 * 3.14159265358979323846), RENDER_BELT_TROJAN};
    for (double days = 0; days <= 3000; days += 1500) {
        Vec3d mine = render_belt_position(&achilles, days);
        for (int c = 0; c < 3; ++c) {
            double expected = catalog_coordinate(q, e, inc, node, peri, tp, epoch + days, c) / 149597870700.0;
            double got = c == 0 ? mine.x : c == 1 ? mine.y : mine.z;
            assert(fabs(got - expected) < 2e-5); /* float elements: ~3,000 km at 5 AU */
        }
    }
}

/* Faint ring systems (SPEC A122, A126): the real-scale strip carries each
 * table's integrated opacity; the illustrative strip shows every ring, dense
 * narrow ones far above broad dusty ones, and leaves the gaps empty. */
static void test_ring_strips_follow_the_ring_tables(void)
{
    static unsigned char strip[RENDER_RING_STRIP_WIDTH * 4], wide[RENDER_RING_ILLUSTRATIVE_WIDTH * 4];
    for (size_t s = 0; s < RENDER_RING_SYSTEM_COUNT; ++s) {
        const RenderRingSystem *system = render_ring_system_at(s);
        assert(render_ring_system(system->planet) == system);
        render_ring_strip(system, strip);
        double per_pixel = (system->outer_m - system->inner_m) / RENDER_RING_STRIP_WIDTH, sum = 0, expected = 0;
        for (int x = 0; x < RENDER_RING_STRIP_WIDTH; ++x) sum += strip[x * 4 + 3] / 255.0 * per_pixel;
        for (size_t k = 0; k < system->count; ++k) {
            const RenderRing *r = &system->rings[k];
            assert(r->radius_m - r->width_m / 2 >= system->inner_m - 1 && r->radius_m + r->width_m / 2 <= system->outer_m + 1);
            expected += r->width_m * (1 - exp(-r->optical_depth));
        }
        /* Rings fainter than one 8-bit step per texel round away entirely. */
        if (expected / (system->outer_m - system->inner_m) > 0.01) assert(fabs(sum - expected) / expected < 0.03);
        render_ring_strip_illustrative(system, wide);
        double coarse = (system->outer_m - system->inner_m) / RENDER_RING_ILLUSTRATIVE_WIDTH;
        for (size_t k = 0; k < system->count; ++k) {
            int x = (int)((system->rings[k].radius_m - system->inner_m) / coarse);
            assert(wide[x * 4 + 3] > 0);
        }
    }
    assert(render_ring_system(BODY_ID_SATURN) == NULL && render_ring_system(BODY_ID_EARTH) == NULL);
    /* Backlit glow (A127): Jupiter's dust rings glow most, Uranus's least. */
    assert(render_ring_system(BODY_ID_JUPITER)->dust_fraction > render_ring_system(BODY_ID_NEPTUNE)->dust_fraction);
    assert(render_ring_system(BODY_ID_NEPTUNE)->dust_fraction > render_ring_system(BODY_ID_URANUS)->dust_fraction);
    for (size_t s = 0; s < RENDER_RING_SYSTEM_COUNT; ++s)
        assert(render_ring_system_at(s)->dust_fraction >= 0 && render_ring_system_at(s)->dust_fraction <= 1);
    /* Uranus: epsilon, 20-96 km wide and optically thick, is the densest
     * texel; the gap at 49,200 km is empty; every ring reads as a line. */
    const RenderRingSystem *uranus = render_ring_system(BODY_ID_URANUS);
    render_ring_strip(uranus, strip);
    double step = (uranus->outer_m - uranus->inner_m) / RENDER_RING_STRIP_WIDTH;
    int densest = 0;
    for (int x = 0; x < RENDER_RING_STRIP_WIDTH; ++x) if (strip[x * 4 + 3] > strip[densest * 4 + 3]) densest = x;
    assert(abs(densest - (int)((51149e3 - uranus->inner_m) / step)) <= 3);
    assert(strip[(int)((49200e3 - uranus->inner_m) / step) * 4 + 3] == 0);
    render_ring_strip_illustrative(uranus, wide);
    double coarse = (uranus->outer_m - uranus->inner_m) / RENDER_RING_ILLUSTRATIVE_WIDTH;
    for (size_t k = 0; k < uranus->count; ++k) assert(wide[(int)((uranus->rings[k].radius_m - uranus->inner_m) / coarse) * 4 + 3] >= 150);
    assert(wide[(int)((49200e3 - uranus->inner_m) / coarse) * 4 + 3] == 0);
    /* Jupiter's dusty rings stay ghostly next to Neptune's Adams ring. */
    const RenderRingSystem *jupiter = render_ring_system(BODY_ID_JUPITER), *neptune = render_ring_system(BODY_ID_NEPTUNE);
    render_ring_strip_illustrative(jupiter, wide);
    int main_ring = wide[(int)((125970e3 - jupiter->inner_m) / ((jupiter->outer_m - jupiter->inner_m) / RENDER_RING_ILLUSTRATIVE_WIDTH)) * 4 + 3];
    assert(main_ring > 0 && main_ring < 40);
    render_ring_strip_illustrative(neptune, wide);
    double nstep = (neptune->outer_m - neptune->inner_m) / RENDER_RING_ILLUSTRATIVE_WIDTH;
    assert(wide[(int)((62933e3 - neptune->inner_m) / nstep) * 4 + 3] > 4 * main_ring);
    assert(wide[(int)((47500e3 - neptune->inner_m) / nstep) * 4 + 3] == 0);
}

/* Scale bar (SPEC A123): the longest round distance (1, 2 or 5 x 10^n km, or
 * in AU from 0.1 AU) that fits the target width, with its label. */
static void test_scale_bar_picks_a_round_distance(void)
{
    RenderScaleBar bar;
    /* 1,000 km per pixel, 120 px available: 100,000 km is 100 px. */
    assert(render_scale_bar(1e6, 120, &bar));
    assert(fabs(bar.length_m - 1e8) < 1 && fabs(bar.pixels - 100) < 1e-9);
    assert(strcmp(bar.label, "100 000 km") == 0);
    /* 2 km per pixel: 200 km (100 px). */
    assert(render_scale_bar(2e3, 120, &bar) && strcmp(bar.label, "200 km") == 0);
    /* Planetary distances switch to AU: 1e7 km per pixel x 120 px is 8 AU, so 5 AU (75 px). */
    assert(render_scale_bar(1e10, 120, &bar) && strcmp(bar.label, "5 AU") == 0);
    assert(bar.pixels > 60 && bar.pixels <= 120);
    assert(render_scale_bar(3e9, 120, &bar) && strcmp(bar.label, "2 AU") == 0);
    /* Never wider than asked, and at least half of it. */
    for (double mpp = 1; mpp < 1e13; mpp *= 1.37) {
        assert(render_scale_bar(mpp, 120, &bar));
        assert(bar.pixels <= 120 + 1e-9 && bar.pixels >= 120 * 0.4);
    }
    assert(!render_scale_bar(0, 120, &bar) && !render_scale_bar(1e3, 0, &bar));
}

/* Constellation lines (SPEC A124): every endpoint is a unit direction in
 * simulation axes, and Orion's figure has Betelgeuse at its true position. */
static void test_constellation_lines_sit_on_their_stars(void)
{
    size_t count = render_constellation_count();
    assert(count == 743);
    Vec3d betelgeuse = orbit_icrf_direction(88.7929, 7.4071);
    double nearest = 1;
    for (size_t k = 0; k < count; ++k) {
        Vec3d a, b;
        assert(render_constellation_segment(k, &a, &b));
        assert(fabs(vec3d_length(a) - 1) < 1e-9 && fabs(vec3d_length(b) - 1) < 1e-9);
        nearest = fmin(nearest, angle_degrees(a, betelgeuse));
    }
    assert(nearest < 0.01);
    Vec3d a, b;
    assert(!render_constellation_segment(count, &a, &b));
}

/* The sky captions (A145) sit at the bottom left, above any page panel
 * whose span they would cross: the desktop dock overlays the canvas. */
static void test_captions_clear_page_panels_they_would_cross(void)
{
    const double dock[2][4] = {{300, 815, 850, 72}, {1076, 91, 352, 278}};
    /* Text from x 16 to 280 misses the dock: it may reach the bottom. */
    assert(render_clear_above(dock, 2, 16, 280, 850) == 850);
    /* Text reaching x 400 crosses it: it must end above the dock's top. */
    assert(render_clear_above(dock, 2, 16, 400, 850) == 815);
    /* A panel entirely above the limit does not lower it. */
    const double high[1][4] = {{0, 0, 400, 100}};
    assert(render_clear_above(high, 1, 16, 400, 850) == 850);
    assert(render_clear_above(NULL, 0, 16, 400, 850) == 850);
}

/* Trails about a parent show the last 1.5 orbits (A152): a hundred loops of
 * Io drawn over each other are a solid band, not a path. */
static void test_trails_about_a_parent_keep_the_last_orbits(void)
{
    /* Io (1.77 d) sampled every hour for 176 days: 4224 points, keep 1.5
     * orbits (64 hourly samples) plus the live endpoint. */
    size_t first = render_trail_first_point(4224, 3600.0, 1.769 * 86400.0, 1.5);
    assert(4224 - first == (size_t)ceil(1.5 * 1.769 * 24.0) + 1);
    /* A trail shorter than the window is drawn whole. */
    assert(render_trail_first_point(40, 3600.0, 1.769 * 86400.0, 1.5) == 0);
    /* Unbound or unknown: whole. */
    assert(render_trail_first_point(4224, 3600.0, NAN, 1.5) == 0);
    assert(render_trail_first_point(4224, 0.0, 86400.0, 1.5) == 0);
    /* Fewer than 12 samples per orbit draw a polygon, not a path: such a
     * trail gives way to the orbit guide. */
    assert(render_trail_too_coarse(1.769 * 86400.0 / 8.0, 1.769 * 86400.0));
    assert(!render_trail_too_coarse(3600.0, 1.769 * 86400.0));
    assert(!render_trail_too_coarse(3600.0, NAN));
}

int main(void)
{
    test_trails_about_a_parent_keep_the_last_orbits();
    test_captions_clear_page_panels_they_would_cross();
    test_texture_inventory_matches_assets();
    test_atmospheres_only_where_bodies_have_air();
    test_orientation_vectors_are_orthonormal();
    test_spin_axes_reproduce_known_obliquities();
    test_venus_and_uranus_spin_backwards();
    test_greenwich_points_at_its_j2000_sidereal_angle();
    test_trail_and_grid_fades();
    test_grid_levels_cross_fade_by_decade();
    test_projected_radius_follows_pinhole_camera();
    test_labels_name_planets_and_zoomed_in_moons();
    test_declutter_keeps_the_most_important_labels();
    test_trail_detail_follows_screen_extent();
    test_frustum_outcodes_skip_only_invisible_segments();
    test_small_wire_markers_become_dots();
    test_trail_segments_stop_at_the_body_surface();
    test_sphere_matches_simulation_handedness_and_map_layout();
    test_ring_annulus_spans_requested_radii();
    test_orbit_guide_reproduces_the_conic_through_the_state();
    test_orbit_guide_skips_open_or_degenerate_orbits();
    test_orbit_guide_detail_follows_screen_size();
    test_markers_find_small_planets_and_frame_the_selection();
    test_eclipse_fraction_covers_umbra_penumbra_and_annulus();
    test_pick_prefers_the_body_under_the_pointer();
    test_resolution_governor_steps_down_only_under_sustained_load();
    test_eclipse_caster_is_a_scaled_copy_of_the_true_geometry();
    test_sky_map_puts_stars_in_their_true_directions();
    test_edge_anchor_follows_the_direction_to_the_body();
    test_markers_slide_past_floating_panels();
    test_belt_sample_decodes_and_matches_the_atlas_conic();
    test_ring_strips_follow_the_ring_tables();
    test_scale_bar_picks_a_round_distance();
    test_constellation_lines_sit_on_their_stars();
    puts("test_scene_style passed");
    return 0;
}

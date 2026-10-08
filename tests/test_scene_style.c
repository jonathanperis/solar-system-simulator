#include "require_assert.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "render/scene_style.h"

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
    /* No source-backed maps: these stay lit colours instead of borrowed art. */
    assert(render_texture_for_body(BODY_ID_IO) == RENDER_TEXTURE_NONE);
    assert(render_texture_for_body(BODY_ID_PHOBOS) == RENDER_TEXTURE_NONE);
    assert(render_texture_for_body(BODY_ID_VESTA) == RENDER_TEXTURE_NONE);
}

static void test_atmospheres_only_where_bodies_have_air(void)
{
    assert(render_atmosphere_for_body(BODY_ID_EARTH).strength > 0.3f);
    assert(render_atmosphere_for_body(BODY_ID_VENUS).strength > 0);
    assert(render_atmosphere_for_body(BODY_ID_NEPTUNE).strength > 0);
    assert(render_atmosphere_for_body(BODY_ID_MOON).strength == 0);
    assert(render_atmosphere_for_body(BODY_ID_MERCURY).strength == 0);
    RenderAtmosphere earth = render_atmosphere_for_body(BODY_ID_EARTH);
    assert(earth.b > earth.r); /* Rayleigh scattering reads blue */
}

static void test_orientation_vectors_are_orthonormal(void)
{
    const BodyId ids[] = {BODY_ID_SUN, BODY_ID_MERCURY, BODY_ID_VENUS, BODY_ID_EARTH, BODY_ID_MOON,
        BODY_ID_MARS, BODY_ID_JUPITER, BODY_ID_SATURN, BODY_ID_URANUS, BODY_ID_NEPTUNE};
    for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
        for (double d = -5000; d <= 15000; d += 2500) {
            RenderOrientation o = render_body_orientation(ids[i], d);
            assert(o.known);
            assert(fabs(vec3d_length(o.pole) - 1) < 1e-12);
            assert(fabs(vec3d_length(o.prime_meridian) - 1) < 1e-12);
            assert(fabs(vec3d_dot(o.pole, o.prime_meridian)) < 1e-12);
        }
    }
    RenderOrientation unknown = render_body_orientation(BODY_ID_IO, 0);
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

int main(void)
{
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
    puts("test_scene_style passed");
    return 0;
}

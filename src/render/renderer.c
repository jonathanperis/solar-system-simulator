#include "renderer.h"

#include <math.h>

#include "../sim/constants.h"
#include "../sim/units.h"

#define SOLAR_MIN_GRID_SLICES 20
#define SOLAR_GRID_PADDING_UNITS 2.0
#define SOLAR_ILLUSTRATIVE_SATELLITE_GAP_UNITS 0.03
#define SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS 0.012f

Vector3 renderer_relative_vector(Vec3d position, Vec3d origin)
{
    Vec3d vector = vec3d_sub(position, origin);
    return (Vector3){
        (float)vector.x,
        (float)vector.y,
        (float)vector.z,
    };
}

float renderer_body_radius(const Body *body, RenderScaleMode mode)
{
    /* Unknown radii use an explicitly nonphysical wire marker in either view. */
    if (body->radius_quality == PHYSICAL_UNKNOWN) return SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS;
    float scaled_radius = meters_to_render_units(body->radius_m);
    if (mode == RENDER_SCALE_REAL) {
        return scaled_radius;
    }

    if (body->kind == BODY_KIND_STAR) {
        return scaled_radius < SOLAR_MIN_VISIBLE_BODY_RADIUS ? SOLAR_MIN_VISIBLE_BODY_RADIUS : scaled_radius;
    }

    if (body->kind == BODY_KIND_MOON) {
        /* Moons remain visible but smaller than planets; this is a visual
         * compromise only, while RENDER_SCALE_REAL keeps physical radii. */
        float moon_radius = SOLAR_ILLUSTRATIVE_PLANET_RADIUS * (float)(body->radius_m / SOLAR_EARTH_RADIUS_M);
        return moon_radius < SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS ? SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS : moon_radius;
    }

    if (body->kind == BODY_KIND_ASTEROID) {
        return SOLAR_ILLUSTRATIVE_ASTEROID_RADIUS;
    }

    return SOLAR_ILLUSTRATIVE_PLANET_RADIUS;
}

float renderer_body_visual_radius(const Body *body, RenderScaleMode mode)
{
    float body_radius = renderer_body_radius(body, mode);
    if (body->id != BODY_ID_SATURN) return body_radius;

    if (mode == RENDER_SCALE_REAL) {
        return meters_to_render_units(SOLAR_SATURN_RING_OUTER_RADIUS_M);
    }
    return body_radius * (float)(SOLAR_SATURN_RING_OUTER_RADIUS_M / SOLAR_SATURN_RADIUS_M);
}

static Vec3d visible_satellite_position(const Body *body, const Body *parent, Vec3d position, Vec3d parent_position, RenderScaleMode mode)
{
    Vec3d relative_position = vec3d_sub(position, parent_position);
    double current_distance = vec3d_length(relative_position);
    if (current_distance <= 0.0) {
        return position;
    }

    double required_distance =
        renderer_body_radius(parent, mode) +
        renderer_body_radius(body, mode) +
        SOLAR_ILLUSTRATIVE_SATELLITE_GAP_UNITS;

    if (body->id == BODY_ID_MOON) {
        Vec3d expanded = vec3d_scale(relative_position, SOLAR_ILLUSTRATIVE_MOON_DISTANCE_FACTOR);
        if (vec3d_length(expanded) > required_distance) {
            return vec3d_add(parent_position, expanded);
        }
    }

    if (current_distance < required_distance) {
        return vec3d_add(parent_position, vec3d_scale(relative_position, required_distance / current_distance));
    }

    return position;
}

Vec3d renderer_body_position(const SolarSystem *system, size_t body_index, RenderScaleMode mode)
{
    if (system->body_count == 0 || body_index >= system->body_count) {
        return vec3d_zero();
    }

    const Body *body = &system->bodies[body_index];
    Vec3d position = meters_vec_to_render_vec3d(body->position_m);
    if (mode == RENDER_SCALE_REAL || body->kind != BODY_KIND_MOON) {
        return position;
    }

    int parent_index = solar_system_parent_index(system, body_index);
    if (parent_index < 0) {
        return position;
    }

    const Body *parent = &system->bodies[parent_index];
    Vec3d parent_position = meters_vec_to_render_vec3d(parent->position_m);
    return visible_satellite_position(body, parent, position, parent_position, mode);
}

Vec3d renderer_trail_point_position(const SolarSystem *system, const BodyTrails *trails, size_t body_index, size_t point_index, RenderScaleMode mode)
{
    if (system->body_count == 0 || body_index >= system->body_count || body_index >= SOLAR_SYSTEM_BODY_CAPACITY) {
        return vec3d_zero();
    }

    Vec3d position = meters_vec_to_render_vec3d(body_trails_point_at(trails, body_index, point_index));
    const Body *body = &system->bodies[body_index];
    if (mode == RENDER_SCALE_REAL || body->kind != BODY_KIND_MOON) {
        return position;
    }

    int parent_index = solar_system_parent_index(system, body_index);
    if (parent_index < 0 || point_index >= body_trails_point_count(trails, (size_t)parent_index)) {
        return position;
    }

    const Body *parent = &system->bodies[parent_index];
    Vec3d parent_position = meters_vec_to_render_vec3d(body_trails_point_at(trails, (size_t)parent_index, point_index));
    return visible_satellite_position(body, parent, position, parent_position, mode);
}

RenderSystemFrame renderer_system_frame(const SolarSystem *system, size_t selected, RenderScaleMode mode)
{
    RenderSystemFrame frame = {.root_index = selected};
    int parent = solar_system_parent_index(system, selected);
    if (system->bodies[selected].kind == BODY_KIND_MOON && parent >= 0) frame.root_index = (size_t)parent;
    const Body *root = &system->bodies[frame.root_index];
    Vec3d center = renderer_body_position(system, frame.root_index, mode);
    for (size_t i = 0; i < system->body_count; ++i) {
        if (root->kind == BODY_KIND_STAR || i == frame.root_index || system->bodies[i].parent_id == root->id) {
            double extent = vec3d_length(vec3d_sub(renderer_body_position(system, i, mode), center))
                + renderer_body_visual_radius(&system->bodies[i], mode);
            frame.radius = fmax(frame.radius, extent);
        }
    }
    return frame;
}

RenderSystemFrame renderer_body_frame(const SolarSystem *system, size_t selected, RenderScaleMode mode)
{
    return (RenderSystemFrame){selected, renderer_body_visual_radius(&system->bodies[selected], mode)};
}

Color renderer_body_color(const Body *body)
{
    switch (body->id) {
        case BODY_ID_SUN:
            return GOLD;
        case BODY_ID_MERCURY:
            return GRAY;
        case BODY_ID_VENUS:
            return BEIGE;
        case BODY_ID_EARTH:
            return BLUE;
        case BODY_ID_MOON:
            return RAYWHITE;
        case BODY_ID_MARS:
            return ORANGE;
        case BODY_ID_PHOBOS:
            return BROWN;
        case BODY_ID_DEIMOS:
            return MAROON;
        case BODY_ID_VESTA:
            return LIGHTGRAY;
        case BODY_ID_JUPITER:
            return (Color){206, 164, 118, 255};
        case BODY_ID_SATURN:
            return (Color){218, 190, 130, 255};
        case BODY_ID_URANUS: return (Color){139, 211, 220, 255};
        case BODY_ID_NEPTUNE: return (Color){72, 113, 216, 255};
        case BODY_ID_IO: return (Color){230, 205, 94, 255};
        case BODY_ID_EUROPA: return (Color){205, 215, 223, 255};
        case BODY_ID_GANYMEDE: return (Color){164, 152, 129, 255};
        case BODY_ID_CALLISTO: return (Color){130, 145, 160, 255};
        case BODY_ID_UNKNOWN:
        case BODY_ID_NONE:
        default:
            break;
    }

    if (body->kind == BODY_KIND_STAR) {
        return GOLD;
    }
    return LIGHTGRAY;
}

const char *renderer_scale_mode_label(RenderScaleMode mode)
{
    switch (mode) {
        case RENDER_SCALE_REAL:
            return "Real scale";
        case RENDER_SCALE_ILLUSTRATIVE:
        default:
            return "Illustrative";
    }
}

int renderer_grid_slices_for_system(const SolarSystem *system, RenderScaleMode mode)
{
    double max_horizontal_extent = 0.0;

    for (size_t i = 0; i < system->body_count; ++i) {
        Vec3d position = renderer_body_position(system, i, mode);
        double x_extent = fabs(position.x);
        double z_extent = fabs(position.z);
        max_horizontal_extent = fmax(max_horizontal_extent, fmax(x_extent, z_extent));
    }

    int slices = (int)fmin(512.0, ceil((max_horizontal_extent + SOLAR_GRID_PADDING_UNITS) * 2.0));
    if (slices < SOLAR_MIN_GRID_SLICES) {
        slices = SOLAR_MIN_GRID_SLICES;
    }

    if ((slices % 2) != 0) {
        ++slices;
    }

    return slices;
}

size_t renderer_trail_sample_stride(size_t point_count)
{
    if (point_count <= SOLAR_RENDER_MAX_TRAIL_SEGMENTS + 1) {
        return 1;
    }

    size_t segment_count = point_count - 1;
    return (segment_count + SOLAR_RENDER_MAX_TRAIL_SEGMENTS - 2) / (SOLAR_RENDER_MAX_TRAIL_SEGMENTS - 1);
}

size_t renderer_trail_draw_segment_count(size_t point_count)
{
    if (point_count < 2) {
        return 0;
    }

    size_t stride = renderer_trail_sample_stride(point_count);
    size_t segment_count = point_count - 1;
    size_t drawn_segments = segment_count / stride;
    if ((segment_count % stride) != 0) {
        ++drawn_segments;
    }
    return drawn_segments;
}

static Vector3 saturn_ring_point(Vector3 center, float radius, float radians)
{
    float tilt = (float)(SOLAR_SATURN_AXIAL_TILT_DEGREES * acos(-1.0) / 180.0);
    float x = cosf(radians) * radius;
    return (Vector3){center.x + x * cosf(tilt), center.y + x * sinf(tilt), center.z + sinf(radians) * radius};
}

static void draw_saturn_rings(Vector3 center, float body_radius, float outer_radius)
{
    const int ring_count = 13;
    const int segments = 96;
    float inner_radius = body_radius * SOLAR_SATURN_RING_VISUAL_INNER_RATIO;
    Color ring_color = {205, 184, 145, 180};

    /* Thin concentric lines keep the rings legible without creating physical
     * geometry. The skipped lines suggest the Cassini Division. */
    for (int ring = 0; ring < ring_count; ++ring) {
        if (ring == 9 || ring == 10) continue;
        float radius = inner_radius + (outer_radius - inner_radius) * (float)ring / (float)(ring_count - 1);
        Vector3 start = saturn_ring_point(center, radius, 0.0f);
        for (int segment = 1; segment <= segments; ++segment) {
            float angle = (float)(2.0 * acos(-1.0) * (double)segment / (double)segments);
            Vector3 end = saturn_ring_point(center, radius, angle);
            DrawLine3D(start, end, ring_color);
            start = end;
        }
    }
}

void renderer_draw_solar_system(const SolarSystem *system, const BodyTrails *trails, RenderScaleMode mode, Vec3d origin)
{
    int slices = renderer_grid_slices_for_system(system, mode);
    float half_width = (float)slices * 0.5f;
    /* A subdued reference plane sits below the orbital plane so its lines do
     * not compete with trails or share their depth at grid intersections. */
    for (int i = 0; i <= slices; ++i) {
        float offset = (float)i - half_width;
        Color grid_color = {45, 57, 69, 255};
        double x = floor(origin.x), z = floor(origin.z);
        DrawLine3D(renderer_relative_vector((Vec3d){x+offset,-.02,z-half_width},origin),
            renderer_relative_vector((Vec3d){x+offset,-.02,z+half_width},origin),grid_color);
        DrawLine3D(renderer_relative_vector((Vec3d){x-half_width,-.02,z+offset},origin),
            renderer_relative_vector((Vec3d){x+half_width,-.02,z+offset},origin),grid_color);
    }

    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        if (body->kind == BODY_KIND_STAR) {
            continue;
        }

        size_t point_count = body_trails_point_count(trails, i);
        if (point_count < 2) {
            continue;
        }

        Color trail_color = Fade(renderer_body_color(body), 0.85f);
        size_t stride = renderer_trail_sample_stride(point_count);
        size_t previous_point = 0;
        /* Adjacent segments share an endpoint. Reuse its render transform;
         * the simulation and synchronized history are immutable while drawing. */
        Vec3d start = renderer_trail_point_position(system, trails, i, 0, mode);
        for (size_t j = stride; j < point_count; j += stride) {
            Vec3d end = renderer_trail_point_position(system, trails, i, j, mode);
            DrawLine3D(renderer_relative_vector(start, origin), renderer_relative_vector(end, origin), trail_color);
            start = end;
            previous_point = j;
        }
        if (previous_point + 1 < point_count) {
            Vec3d end = renderer_trail_point_position(system, trails, i, point_count - 1, mode);
            DrawLine3D(renderer_relative_vector(start, origin), renderer_relative_vector(end, origin), trail_color);
        }
    }

    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        Color color = renderer_body_color(body);
        Vector3 position = renderer_relative_vector(renderer_body_position(system, i, mode), origin);
        float radius = renderer_body_radius(body, mode);

        if (body->id == BODY_ID_SATURN) {
            draw_saturn_rings(position, radius, renderer_body_visual_radius(body, mode));
        }
        if (body->radius_quality == PHYSICAL_UNKNOWN) DrawSphereWires(position, radius, 4, 4, color);
        else DrawSphere(position, radius, color);
    }
}

#include "renderer.h"

#include <math.h>
#include <stdint.h>

#include "../sim/constants.h"
#include "../sim/units.h"

#define SOLAR_MIN_GRID_SLICES 20
#define SOLAR_GRID_PADDING_UNITS 2.0
#define SOLAR_ILLUSTRATIVE_SATELLITE_GAP_UNITS 0.03
#define SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS 0.012f
#define SOLAR_GRID_SPACING_UNITS 1.0f
#define SOLAR_BODY_HIGHLIGHT_RATIO 0.30f

static float clamp_float(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static Color color_rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    return (Color){r, g, b, a};
}

static Vector3 vec3d_to_raylib(Vec3d vector)
{
    return (Vector3){
        (float)vector.x,
        (float)vector.y,
        (float)vector.z,
    };
}

static Vector3 vector3_subtract(Vector3 a, Vector3 b)
{
    return (Vector3){a.x - b.x, a.y - b.y, a.z - b.z};
}

static Vector3 vector3_add(Vector3 a, Vector3 b)
{
    return (Vector3){a.x + b.x, a.y + b.y, a.z + b.z};
}

static Vector3 vector3_scale(Vector3 vector, float scale)
{
    return (Vector3){vector.x * scale, vector.y * scale, vector.z * scale};
}

static Vector3 vector3_normalize_or(Vector3 vector, Vector3 fallback)
{
    float length = sqrtf(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
    if (length <= 1e-6f) {
        return fallback;
    }
    return vector3_scale(vector, 1.0f / length);
}

static uint32_t render_hash(size_t index, uint32_t salt)
{
    uint32_t value = (uint32_t)index * 747796405u + salt * 2891336453u + 277803737u;
    value ^= value >> 16;
    value *= 2246822519u;
    value ^= value >> 13;
    value *= 3266489917u;
    value ^= value >> 16;
    return value;
}

static float hash_unit(size_t index, uint32_t salt)
{
    return (float)(render_hash(index, salt) & 0xffffu) / 65535.0f;
}

static int moon_parent_index(const SolarSystem *system, size_t body_index)
{
    /* Renderer-only parent lookup: physics does not need parent links, but the
     * illustrative view uses catalog metadata to separate tiny satellites from
     * readable planet radii without depending on fragile body-name strings. */
    const Body *body = &system->bodies[body_index];
    if (body->parent_id == BODY_ID_NONE || body->parent_id == BODY_ID_UNKNOWN) {
        return -1;
    }

    for (size_t i = 0; i < system->body_count; ++i) {
        if (system->bodies[i].id == body->parent_id) {
            return (int)i;
        }
    }

    return -1;
}

float renderer_body_radius(const Body *body, RenderScaleMode mode)
{
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

    return SOLAR_ILLUSTRATIVE_PLANET_RADIUS;
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

    int parent_index = moon_parent_index(system, body_index);
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

    int parent_index = moon_parent_index(system, body_index);
    if (parent_index < 0 || point_index >= body_trails_point_count(trails, (size_t)parent_index)) {
        return position;
    }

    const Body *parent = &system->bodies[parent_index];
    Vec3d parent_position = meters_vec_to_render_vec3d(body_trails_point_at(trails, (size_t)parent_index, point_index));
    return visible_satellite_position(body, parent, position, parent_position, mode);
}

RendererBodyStyle renderer_body_style(const Body *body)
{
    switch (body->id) {
        case BODY_ID_SUN:
            return (RendererBodyStyle){
                .base = color_rgba(255, 205, 88, 255),
                .accent = color_rgba(255, 237, 168, 255),
                .rim = color_rgba(255, 142, 43, 255),
                .glow = color_rgba(255, 190, 76, 120),
            };
        case BODY_ID_MERCURY:
            return (RendererBodyStyle){GRAY, color_rgba(205, 196, 178, 255), color_rgba(116, 108, 102, 255), color_rgba(170, 160, 145, 26)};
        case BODY_ID_VENUS:
            return (RendererBodyStyle){BEIGE, color_rgba(255, 226, 145, 255), color_rgba(184, 122, 65, 255), color_rgba(244, 195, 92, 32)};
        case BODY_ID_EARTH:
            return (RendererBodyStyle){BLUE, color_rgba(71, 210, 164, 255), color_rgba(141, 221, 255, 255), color_rgba(82, 169, 255, 36)};
        case BODY_ID_MOON:
            return (RendererBodyStyle){RAYWHITE, color_rgba(176, 181, 191, 255), color_rgba(224, 224, 218, 255), color_rgba(224, 224, 218, 22)};
        case BODY_ID_MARS:
            return (RendererBodyStyle){ORANGE, color_rgba(255, 197, 111, 255), color_rgba(160, 65, 48, 255), color_rgba(255, 119, 70, 34)};
        case BODY_ID_PHOBOS:
            return (RendererBodyStyle){BROWN, color_rgba(150, 118, 86, 255), color_rgba(88, 61, 50, 255), color_rgba(139, 96, 64, 24)};
        case BODY_ID_DEIMOS:
            return (RendererBodyStyle){MAROON, color_rgba(174, 105, 91, 255), color_rgba(94, 55, 65, 255), color_rgba(146, 76, 82, 22)};
        case BODY_ID_UNKNOWN:
        case BODY_ID_NONE:
        default:
            break;
    }

    if (body->kind == BODY_KIND_STAR) {
        return (RendererBodyStyle){color_rgba(255, 205, 88, 255), color_rgba(255, 237, 168, 255), ORANGE, color_rgba(255, 185, 65, 120)};
    }
    return (RendererBodyStyle){LIGHTGRAY, RAYWHITE, GRAY, color_rgba(180, 180, 180, 20)};
}

Color renderer_body_color(const Body *body)
{
    return renderer_body_style(body).base;
}

RendererStar renderer_starfield_star(size_t index, double elapsed_seconds)
{
    float x_norm = 0.02f + hash_unit(index, 11u) * 0.96f;
    float y_norm = 0.02f + hash_unit(index, 23u) * 0.94f;
    float radius = 0.8f + hash_unit(index, 37u) * 1.6f;
    float base_alpha = 0.44f + hash_unit(index, 41u) * 0.44f;
    float twinkle = sinf((float)elapsed_seconds * 0.35f + hash_unit(index, 53u) * 6.2831853f) * 0.14f;

    return (RendererStar){
        .x_norm = x_norm,
        .y_norm = y_norm,
        .radius = radius,
        .alpha = clamp_float(base_alpha + twinkle, 0.28f, 1.0f),
    };
}

float renderer_trail_segment_alpha(size_t segment_index, size_t segment_count)
{
    if (segment_count <= 1) {
        return 0.55f;
    }

    float progress = (float)(segment_index + 1) / (float)segment_count;
    return clamp_float(0.18f + 0.68f * sqrtf(progress), 0.16f, 0.90f);
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

    int slices = (int)ceil((max_horizontal_extent + SOLAR_GRID_PADDING_UNITS) * 2.0);
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

void renderer_draw_backdrop(int screen_width, int screen_height, double elapsed_seconds)
{
    Color top = color_rgba(14, 17, 48, 255);
    Color bottom = color_rgba(24, 22, 66, 255);
    DrawRectangleGradientV(0, 0, screen_width, screen_height, top, bottom);

    for (size_t i = 0; i < SOLAR_RENDER_STAR_COUNT; ++i) {
        RendererStar star = renderer_starfield_star(i, elapsed_seconds);
        Vector2 center = {(float)screen_width * star.x_norm, (float)screen_height * star.y_norm};
        DrawCircleV(center, star.radius, Fade(color_rgba(245, 236, 203, 255), star.alpha));
    }
}

static void draw_orbit_grid(int slices)
{
    int half_slices = slices / 2;
    float half_width = (float)half_slices * SOLAR_GRID_SPACING_UNITS;
    Color minor = color_rgba(105, 141, 196, 255);
    Color major = color_rgba(151, 191, 244, 255);
    Color axis = color_rgba(255, 201, 105, 255);

    for (int i = -half_slices; i <= half_slices; ++i) {
        float offset = (float)i * SOLAR_GRID_SPACING_UNITS;
        Color line_color = Fade(minor, 0.22f);
        if (i == 0) {
            line_color = Fade(axis, 0.62f);
        } else if ((i % 5) == 0) {
            line_color = Fade(major, 0.34f);
        }

        DrawLine3D((Vector3){offset, 0.0f, -half_width}, (Vector3){offset, 0.0f, half_width}, line_color);
        DrawLine3D((Vector3){-half_width, 0.0f, offset}, (Vector3){half_width, 0.0f, offset}, line_color);
    }
}

static Vector3 sun_position_for_light(const SolarSystem *system)
{
    for (size_t i = 0; i < system->body_count; ++i) {
        if (system->bodies[i].kind == BODY_KIND_STAR) {
            return vec3d_to_raylib(renderer_body_position(system, i, RENDER_SCALE_ILLUSTRATIVE));
        }
    }
    return (Vector3){0.0f, 0.0f, 0.0f};
}

static void draw_body_surface(const SolarSystem *system, const Body *body, Vector3 position, float radius)
{
    RendererBodyStyle style = renderer_body_style(body);
    Vector3 fallback_light = vector3_normalize_or((Vector3){-0.35f, 0.70f, -0.45f}, (Vector3){0.0f, 1.0f, 0.0f});
    Vector3 light_direction = vector3_normalize_or(vector3_subtract(sun_position_for_light(system), position), fallback_light);

    if (body->kind == BODY_KIND_STAR) {
        /* Keep the Sun emissive and solid. Transparent 3D halo spheres wrote
         * depth before the core and made the live Sun read like a dark target. */
        DrawSphereEx(position, radius, 32, 32, style.base);
        DrawSphereWires(position, radius * 1.04f, 24, 24, Fade(style.accent, 0.82f));
        DrawSphereWires(position, radius * 1.24f, 24, 24, Fade(style.rim, 0.34f));
        return;
    }

    DrawSphereEx(position, radius, 18, 18, style.base);

    Vector3 highlight = vector3_add(position, vector3_scale(light_direction, radius * 0.64f));
    highlight.y += radius * 0.12f;
    DrawSphereEx(highlight, radius * (SOLAR_BODY_HIGHLIGHT_RATIO * 0.78f), 10, 10, Fade(style.accent, 0.78f));

    Vector3 shadow = vector3_add(position, vector3_scale(light_direction, -radius * 0.36f));
    DrawSphereEx(shadow, radius * 0.24f, 10, 10, Fade(color_rgba(13, 11, 31, 255), 0.18f));
    DrawSphereWires(position, radius * 1.02f, 14, 14, Fade(style.rim, 0.44f));
}

void renderer_draw_solar_system(const SolarSystem *system, const BodyTrails *trails, RenderScaleMode mode)
{
    draw_orbit_grid(renderer_grid_slices_for_system(system, mode));

    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        if (body->kind == BODY_KIND_STAR) {
            continue;
        }

        size_t point_count = body_trails_point_count(trails, i);
        if (point_count < 2) {
            continue;
        }

        RendererBodyStyle style = renderer_body_style(body);
        size_t stride = renderer_trail_sample_stride(point_count);
        size_t drawn_segments = renderer_trail_draw_segment_count(point_count);
        size_t segment_index = 0;
        size_t previous_point = 0;
        for (size_t j = stride; j < point_count; j += stride) {
            Vec3d start = renderer_trail_point_position(system, trails, i, previous_point, mode);
            Vec3d end = renderer_trail_point_position(system, trails, i, j, mode);
            DrawLine3D(
                vec3d_to_raylib(start),
                vec3d_to_raylib(end),
                Fade(style.accent, renderer_trail_segment_alpha(segment_index, drawn_segments))
            );
            previous_point = j;
            ++segment_index;
        }
        if (previous_point + 1 < point_count) {
            Vec3d start = renderer_trail_point_position(system, trails, i, previous_point, mode);
            Vec3d end = renderer_trail_point_position(system, trails, i, point_count - 1, mode);
            DrawLine3D(
                vec3d_to_raylib(start),
                vec3d_to_raylib(end),
                Fade(style.accent, renderer_trail_segment_alpha(segment_index, drawn_segments))
            );
        }
    }

    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        Vector3 position = vec3d_to_raylib(renderer_body_position(system, i, mode));
        float radius = renderer_body_radius(body, mode);
        draw_body_surface(system, body, position, radius);
    }
}

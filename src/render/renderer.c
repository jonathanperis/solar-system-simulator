#include "renderer.h"

#include <math.h>
#include <raylib.h>
#include <string.h>

#include "../sim/constants.h"
#include "../sim/units.h"

#define SOLAR_MIN_GRID_SLICES 20
#define SOLAR_GRID_PADDING_UNITS 2.0
#define SOLAR_ILLUSTRATIVE_SATELLITE_GAP_UNITS 0.03
#define SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS 0.012f

static int body_named(const Body *body, const char *name)
{
    return body->name != NULL && strcmp(body->name, name) == 0;
}

static Vector3 vec3d_to_raylib(Vec3d vector)
{
    return (Vector3){
        (float)vector.x,
        (float)vector.y,
        (float)vector.z,
    };
}

static int moon_parent_index(const SolarSystem *system, size_t body_index)
{
    const Body *body = &system->bodies[body_index];
    const char *parent_name = NULL;

    if (body_named(body, "Moon")) {
        parent_name = "Earth";
    } else if (body_named(body, "Phobos") || body_named(body, "Deimos")) {
        parent_name = "Mars";
    }

    if (parent_name == NULL) {
        return -1;
    }

    for (size_t i = 0; i < system->body_count; ++i) {
        if (body_named(&system->bodies[i], parent_name)) {
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

    if (body_named(body, "Moon")) {
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

static Color body_render_color(const Body *body)
{
    if (body->kind == BODY_KIND_STAR) {
        return GOLD;
    }

    if (body->name != NULL && strcmp(body->name, "Mercury") == 0) {
        return GRAY;
    }

    if (body->name != NULL && strcmp(body->name, "Venus") == 0) {
        return BEIGE;
    }

    if (body->name != NULL && strcmp(body->name, "Earth") == 0) {
        return BLUE;
    }

    if (body->name != NULL && strcmp(body->name, "Moon") == 0) {
        return RAYWHITE;
    }

    if (body->name != NULL && strcmp(body->name, "Mars") == 0) {
        return ORANGE;
    }

    if (body->name != NULL && strcmp(body->name, "Phobos") == 0) {
        return BROWN;
    }

    if (body->name != NULL && strcmp(body->name, "Deimos") == 0) {
        return MAROON;
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

    int slices = (int)ceil((max_horizontal_extent + SOLAR_GRID_PADDING_UNITS) * 2.0);
    if (slices < SOLAR_MIN_GRID_SLICES) {
        slices = SOLAR_MIN_GRID_SLICES;
    }

    if ((slices % 2) != 0) {
        ++slices;
    }

    return slices;
}

void renderer_draw_solar_system(const SolarSystem *system, const BodyTrails *trails, RenderScaleMode mode)
{
    DrawGrid(renderer_grid_slices_for_system(system, mode), 1.0f);

    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        if (body->kind == BODY_KIND_STAR) {
            continue;
        }

        size_t point_count = body_trails_point_count(trails, i);
        if (point_count < 2) {
            continue;
        }

        Color trail_color = Fade(body_render_color(body), 0.55f);
        for (size_t j = 1; j < point_count; ++j) {
            Vec3d start = renderer_trail_point_position(system, trails, i, j - 1, mode);
            Vec3d end = renderer_trail_point_position(system, trails, i, j, mode);
            DrawLine3D(vec3d_to_raylib(start), vec3d_to_raylib(end), trail_color);
        }
    }

    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        Color color = body_render_color(body);
        Vector3 position = vec3d_to_raylib(renderer_body_position(system, i, mode));
        float radius = renderer_body_radius(body, mode);

        DrawSphere(position, radius, color);
        DrawSphereWires(position, radius, 16, 16, ORANGE);
    }
}

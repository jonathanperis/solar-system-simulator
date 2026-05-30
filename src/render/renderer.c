#include "renderer.h"

#include <raylib.h>
#include <string.h>

#include "../sim/constants.h"
#include "../sim/units.h"

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

Vec3d renderer_body_position(const SolarSystem *system, size_t body_index, RenderScaleMode mode)
{
    if (system->body_count == 0 || body_index >= system->body_count) {
        return vec3d_zero();
    }

    const Body *body = &system->bodies[body_index];
    Vec3d position = meters_vec_to_render_vec3d(body->position_m);
    if (mode == RENDER_SCALE_REAL || !body_named(body, "Moon")) {
        return position;
    }

    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *candidate_parent = &system->bodies[i];
        if (body_named(candidate_parent, "Earth")) {
            Vec3d parent_position = meters_vec_to_render_vec3d(candidate_parent->position_m);
            Vec3d relative_position = vec3d_sub(position, parent_position);
            return vec3d_add(parent_position, vec3d_scale(relative_position, SOLAR_ILLUSTRATIVE_MOON_DISTANCE_FACTOR));
        }
    }

    return position;
}

Vec3d renderer_trail_point_position(const SolarSystem *system, const BodyTrails *trails, size_t body_index, size_t point_index, RenderScaleMode mode)
{
    if (system->body_count == 0 || body_index >= system->body_count || body_index >= SOLAR_SYSTEM_BODY_CAPACITY) {
        return vec3d_zero();
    }

    Vec3d position = meters_vec_to_render_vec3d(body_trails_point_at(trails, body_index, point_index));
    const Body *body = &system->bodies[body_index];
    if (mode == RENDER_SCALE_REAL || !body_named(body, "Moon")) {
        return position;
    }

    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *candidate_parent = &system->bodies[i];
        if (body_named(candidate_parent, "Earth") && point_index < body_trails_point_count(trails, i)) {
            Vec3d parent_position = meters_vec_to_render_vec3d(body_trails_point_at(trails, i, point_index));
            Vec3d relative_position = vec3d_sub(position, parent_position);
            return vec3d_add(parent_position, vec3d_scale(relative_position, SOLAR_ILLUSTRATIVE_MOON_DISTANCE_FACTOR));
        }
    }

    return position;
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
        return SOLAR_ILLUSTRATIVE_PLANET_RADIUS * (float)(body->radius_m / SOLAR_EARTH_RADIUS_M);
    }

    return SOLAR_ILLUSTRATIVE_PLANET_RADIUS;
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

void renderer_draw_solar_system(const SolarSystem *system, const BodyTrails *trails, RenderScaleMode mode)
{
    DrawGrid(20, 1.0f);

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

#include "renderer.h"

#include <math.h>
#include <raylib.h>
#include <string.h>

#include "../sim/constants.h"
#include "../sim/units.h"

static Vector3 body_render_position(const Body *body)
{
    Vec3d render_position = meters_vec_to_render_vec3d(body->position_m);
    return (Vector3){
        (float)render_position.x,
        (float)render_position.y,
        (float)render_position.z,
    };
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

    float illustrative_radius = sqrtf(scaled_radius) * SOLAR_ILLUSTRATIVE_NON_STAR_RADIUS_FACTOR;
    return illustrative_radius < SOLAR_MIN_VISIBLE_NON_STAR_RADIUS ? SOLAR_MIN_VISIBLE_NON_STAR_RADIUS : illustrative_radius;
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

void renderer_draw_solar_system(const SolarSystem *system, RenderScaleMode mode)
{
    DrawGrid(20, 1.0f);

    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        Color color = body_render_color(body);
        Vector3 position = body_render_position(body);
        float radius = renderer_body_radius(body, mode);

        DrawSphere(position, radius, color);
        DrawSphereWires(position, radius, 16, 16, ORANGE);
    }
}

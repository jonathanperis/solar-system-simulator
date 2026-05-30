#include "renderer.h"

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

static float body_min_visible_radius(const Body *body)
{
    return body->kind == BODY_KIND_STAR ? SOLAR_MIN_VISIBLE_BODY_RADIUS : SOLAR_MIN_VISIBLE_NON_STAR_RADIUS;
}

static float body_render_radius(const Body *body, RenderScaleMode mode)
{
    float scaled_radius = meters_to_render_units(body->radius_m);
    if (mode == RENDER_SCALE_REAL) {
        return scaled_radius;
    }

    float min_radius = body_min_visible_radius(body);
    return scaled_radius < min_radius ? min_radius : scaled_radius;
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
        float radius = body_render_radius(body, mode);

        DrawSphere(position, radius, color);
        DrawSphereWires(position, radius, 16, 16, ORANGE);
    }
}

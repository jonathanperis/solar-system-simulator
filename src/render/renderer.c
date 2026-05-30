#include "renderer.h"

#include <raylib.h>

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

static float body_render_radius(const Body *body)
{
    float scaled_radius = meters_to_render_units(body->radius_m);
    return scaled_radius < SOLAR_MIN_VISIBLE_BODY_RADIUS ? SOLAR_MIN_VISIBLE_BODY_RADIUS : scaled_radius;
}

void renderer_draw_solar_system(const SolarSystem *system)
{
    DrawGrid(20, 1.0f);

    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        Color color = body->kind == BODY_KIND_STAR ? GOLD : LIGHTGRAY;
        Vector3 position = body_render_position(body);
        float radius = body_render_radius(body);

        DrawSphere(position, radius, color);
        DrawSphereWires(position, radius, 16, 16, ORANGE);
    }
}

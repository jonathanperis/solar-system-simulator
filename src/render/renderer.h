#ifndef SOLAR_RENDERER_H
#define SOLAR_RENDERER_H

#include "../sim/solar_system.h"

typedef enum RenderScaleMode {
    RENDER_SCALE_ILLUSTRATIVE,
    RENDER_SCALE_REAL
} RenderScaleMode;

const char *renderer_scale_mode_label(RenderScaleMode mode);
float renderer_body_radius(const Body *body, RenderScaleMode mode);
void renderer_draw_solar_system(const SolarSystem *system, RenderScaleMode mode);

#endif

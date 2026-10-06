#ifndef SOLAR_RENDER_SCALE_H
#define SOLAR_RENDER_SCALE_H

/* Presentation policy only. These numbers decide how SI simulation state is
 * drawn; none of them may feed back into src/sim (V1, V10). This header is
 * deliberately raylib-free so render-scale math can be tested without a window. */

#include "../sim/vec3d.h"

/* One astronomical unit spans ten render units in the 3D scene. */
#define SOLAR_RENDER_UNITS_PER_AU 10.0
/* Illustrative view: the Sun stays at least this large, planets share one
 * readable size, asteroids a smaller one, and close moons are pushed outward
 * from their parent by this factor. Real-scale view ignores all of these. */
#define SOLAR_MIN_VISIBLE_BODY_RADIUS 0.5f
#define SOLAR_ILLUSTRATIVE_PLANET_RADIUS 0.12f
#define SOLAR_ILLUSTRATIVE_ASTEROID_RADIUS 0.03f
#define SOLAR_ILLUSTRATIVE_MOON_DISTANCE_FACTOR 8.0
/* Inner edge of the drawn ring lines, as a multiple of Saturn's drawn radius. */
#define SOLAR_SATURN_RING_VISUAL_INNER_RATIO 1.15

float meters_to_render_units(double meters);
Vec3d meters_vec_to_render_vec3d(Vec3d meters);

#endif

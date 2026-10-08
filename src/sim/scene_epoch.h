#ifndef SOLAR_SCENE_EPOCH_H
#define SOLAR_SCENE_EPOCH_H

#include <stdbool.h>
#include <stddef.h>

#include "vec3d.h"

/* Every scene starts from the real sky of this epoch (SPEC A92): JD 2461200.5
 * TDB, 2026-06-09 00:00 TDB, the epoch of the catalog experiments too. */
#define SOLAR_SCENE_EPOCH_JD 2461200.5

/* Heliocentric state of a planetary-system barycenter at the epoch, in
 * simulation axes and SI units: index 0..7 is Mercury..Neptune (Earth means
 * the Earth-Moon barycenter). */
void scene_epoch_planet_state(size_t planet_index, Vec3d *position_m, Vec3d *velocity_mps);

/* Pinned Horizons state of body `code` at the epoch, in simulation axes and SI
 * units: a moon relative to its primary's centre; Vesta (8), the Pluto system
 * barycenter (999) and the Didymos system barycenter (920065803)
 * heliocentric. False for an undated body, which keeps its mean-element
 * state. */
bool scene_epoch_state(int code, Vec3d *position_m, Vec3d *velocity_mps);

/* Writes the calendar date `elapsed_seconds` after the epoch as
 * "YYYY-MM-DD HH:MM TDB" (proleptic Gregorian). False if it does not fit. */
bool scene_epoch_format_date(double elapsed_seconds, char *out, size_t size);

#endif

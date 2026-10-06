#ifndef SOLAR_LESSONS_H
#define SOLAR_LESSONS_H

#include "solar_system.h"

typedef enum LessonPreset {
    LESSON_CORE, LESSON_CIRCULAR, LESSON_ECCENTRIC, LESSON_ESCAPE,
    LESSON_EARTH_MOON, LESSON_INCLINED, LESSON_PHOBOS,
    LESSON_BARYCENTRIC_CORE, LESSON_RESONANCE, LESSON_ENCOUNTER, LESSON_COLLISION,
    LESSON_COUNT, LESSON_CATALOG = -1
} LessonPreset;

const char *lesson_name(LessonPreset preset);
/* Accepts initial-speed factors from lesson_minimum_velocity_factor() to 2. */
bool lesson_create(LessonPreset preset, double velocity_factor, SolarSystem *result);
/* Smallest initial-speed factor whose analytic two-body orbit keeps the subject
 * outside its parent (closest approach >= sum of radii), rounded up to whole
 * hundredths and never below 0.1. Fixed presets (core, barycentric-core)
 * return 1; the collision lesson models contact itself and returns 0.1. */
double lesson_minimum_velocity_factor(LessonPreset preset);
/* Lessons whose runs flag sphere contact (all but core, barycentric-core,
 * collision and catalog scenes). Contact invalidates published lesson errors. */
bool lesson_monitors_contact(LessonPreset preset);
size_t lesson_subject_index(LessonPreset preset);
double lesson_default_step(LessonPreset preset);
double lesson_resonant_angle_degrees(const SolarSystem *system);
bool lesson_reference_position(const SolarSystem *initial, size_t subject, double seconds, Vec3d *relative);

#endif

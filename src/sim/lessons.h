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
bool lesson_create(LessonPreset preset, double velocity_factor, SolarSystem *result);
size_t lesson_subject_index(LessonPreset preset);
double lesson_default_step(LessonPreset preset);
double lesson_resonant_angle_degrees(const SolarSystem *system);
bool lesson_reference_position(const SolarSystem *initial, size_t subject, double seconds, Vec3d *relative);

#endif

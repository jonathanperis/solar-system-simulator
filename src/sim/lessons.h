#ifndef SOLAR_LESSONS_H
#define SOLAR_LESSONS_H

#include "solar_system.h"

typedef enum LessonPreset {
    LESSON_CORE, LESSON_CIRCULAR, LESSON_ECCENTRIC, LESSON_ESCAPE,
    LESSON_EARTH_MOON, LESSON_INCLINED, LESSON_PHOBOS,
    LESSON_BARYCENTRIC_CORE, LESSON_RESONANCE, LESSON_ENCOUNTER, LESSON_COLLISION,
    /* Family scenes: a giant planet with its complete moon catalog. Like the
     * main scene they are fixed 15-second Verlet scenes, not guided lessons. */
    LESSON_JUPITER_SYSTEM, LESSON_SATURN_SYSTEM, LESSON_URANUS_SYSTEM, LESSON_NEPTUNE_SYSTEM,
    LESSON_PLUTO_SYSTEM, LESSON_DIDYMOS_SYSTEM,
    /* Two-body lessons on small-body binaries (SPEC A96). */
    LESSON_PLUTO_CHARON, LESSON_DART,
    LESSON_COUNT, LESSON_CATALOG = -1
} LessonPreset;

const char *lesson_name(LessonPreset preset);
/* True for the main scene and the four family scenes: fixed 15 s Verlet
 * astronomy scenes rather than guided lessons. */
bool lesson_is_scene(LessonPreset preset);
/* True when the preset starts from the dated 2026-06-09 sky (every scene and
 * the barycentric-core lesson, which releases the Sun in the main scene), so
 * clocks show calendar dates and spin uses the scene epoch. */
bool lesson_starts_at_epoch(LessonPreset preset);
/* The primary a family scene centres on, or BODY_ID_NONE. */
BodyId lesson_family_planet(LessonPreset preset);
/* Accepts initial-speed factors from lesson_minimum_velocity_factor() to 2. */
bool lesson_create(LessonPreset preset, double velocity_factor, SolarSystem *result);
/* Smallest initial-speed factor whose analytic two-body orbit keeps the subject
 * outside its parent (closest approach >= sum of radii), rounded up to whole
 * hundredths and never below 0.1. Fixed presets (core, barycentric-core)
 * return 1; the collision lesson models contact itself and returns 0.1. */
double lesson_minimum_velocity_factor(LessonPreset preset);
/* Runs that record swept contact-sphere crossings: every orbital lesson and
 * catalog experiments (not core, barycentric-core or the collision lesson,
 * which models contact itself). A crossing withholds published lesson errors. */
bool lesson_monitors_contact(LessonPreset preset);
size_t lesson_subject_index(LessonPreset preset);
double lesson_default_step(LessonPreset preset);
double lesson_resonant_angle_degrees(const SolarSystem *system);
bool lesson_reference_position(const SolarSystem *initial, size_t subject, double seconds, Vec3d *relative);

#endif

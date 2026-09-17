#include "lab_config.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

bool lab_ticks_for(double seconds, double step, uint64_t *ticks)
{
    if (!isfinite(seconds) || !isfinite(step) || seconds <= 0 || step <= 0) return false;
    double value = seconds / step;
    if (!isfinite(value) || value < 1 || value > 9007199254740991.0 || fabs(value - round(value)) > 1e-8) return false;
    *ticks = (uint64_t)round(value);
    return true;
}

bool lab_configuration_valid(const LabConfiguration *config)
{
    uint64_t samples;
    if (!lab_ticks_for(config->duration_seconds, config->sample_seconds, &samples)) return false;
    for (size_t side = 0; side < 2; ++side) {
        uint64_t per_sample, total;
        if (!lesson_configuration_valid(config->lesson, config->velocity_factor, config->integrator[side],
            config->step_seconds[side], config->collision[side]) ||
            !lab_ticks_for(config->sample_seconds, config->step_seconds[side], &per_sample) ||
            !lab_ticks_for(config->duration_seconds, config->step_seconds[side], &total) ||
            per_sample > UINT64_MAX / samples || per_sample * samples != total) return false;
    }
    return true;
}

bool lab_configuration_parse(const char *text, LabConfiguration *config)
{
    if (strlen(text) >= SOLAR_LAB_CONFIG_BYTES) return false;
    LabConfiguration candidate = {.lesson = LESSON_CATALOG};
    char scene[32], method[2][16], collision[2][16]; int consumed = 0;
    if (sscanf(text, "SOLAR_LAB_V1 %31s %lf %15s %lf %15s %15s %lf %15s %lf %lf %n", scene,
        &candidate.velocity_factor, method[0], &candidate.step_seconds[0], collision[0],
        method[1], &candidate.step_seconds[1], collision[1], &candidate.sample_seconds,
        &candidate.duration_seconds, &consumed) != 10 || !consumed) return false;
    while (isspace((unsigned char)text[consumed])) ++consumed;
    if (text[consumed]) return false;
    for (int i = 0; i < LESSON_COUNT; ++i) if (!strcmp(scene, lesson_name((LessonPreset)i))) candidate.lesson = (LessonPreset)i;
    for (size_t side = 0; side < 2; ++side) {
        if (strcmp(method[side], "verlet") && strcmp(method[side], "euler")) return false;
        candidate.integrator[side] = !strcmp(method[side], "euler") ? PHYSICS_EULER : PHYSICS_VERLET;
        candidate.collision[side] = (CollisionMode)-1;
        for (int mode = COLLISION_NONE; mode <= COLLISION_MERGE; ++mode)
            if (!strcmp(collision[side], collision_mode_name((CollisionMode)mode))) candidate.collision[side] = (CollisionMode)mode;
    }
    if (!lab_configuration_valid(&candidate)) return false;
    *config = candidate;
    return true;
}

bool lab_configuration_format(const LabConfiguration *config, char *out, size_t size)
{
    if (!lab_configuration_valid(config)) return false;
    int count = snprintf(out, size, "SOLAR_LAB_V1 %s %.17g %s %.17g %s %s %.17g %s %.17g %.17g\n",
        lesson_name(config->lesson), config->velocity_factor,
        config->integrator[0] == PHYSICS_EULER ? "euler" : "verlet", config->step_seconds[0], collision_mode_name(config->collision[0]),
        config->integrator[1] == PHYSICS_EULER ? "euler" : "verlet", config->step_seconds[1], collision_mode_name(config->collision[1]),
        config->sample_seconds, config->duration_seconds);
    return count >= 0 && (size_t)count < size;
}

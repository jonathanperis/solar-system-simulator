#ifndef SOLAR_LAB_CONFIG_H
#define SOLAR_LAB_CONFIG_H
#include "simulation_session.h"

#define SOLAR_LAB_CONFIG_BYTES 512
typedef struct LabConfiguration {
    LessonPreset lesson;
    double velocity_factor;
    PhysicsIntegrator integrator[2];
    double step_seconds[2];
    CollisionMode collision[2];
    double sample_seconds;
    double duration_seconds;
} LabConfiguration;

bool lab_ticks_for(double seconds, double step, uint64_t *ticks);
bool lab_configuration_valid(const LabConfiguration *config);
bool lab_configuration_parse(const char *text, LabConfiguration *config);
bool lab_configuration_format(const LabConfiguration *config, char *out, size_t size);
#endif

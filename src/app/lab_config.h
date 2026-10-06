#ifndef SOLAR_LAB_CONFIG_H
#define SOLAR_LAB_CONFIG_H
#include "simulation_session.h"

#define SOLAR_LAB_CONFIG_BYTES 512
/* Upper bound on fixed physics ticks per run (each side of a comparison, or a
 * headless series). The longest shipped example, the resonance comparison,
 * needs 2.1e7 ticks; 1e9 leaves ~50x headroom. Measured on an Apple-silicon
 * laptop, that is under a minute per side for small lesson scenes and about
 * 1.6 hours for the 128-body core scene (~475 simulated years at 15 s), where
 * the old 2^53 limit allowed descriptors that would never finish. */
#define SOLAR_LAB_MAX_TICKS 1000000000ULL
typedef struct LabConfiguration {
    LessonPreset lesson;
    double velocity_factor;
    PhysicsIntegrator integrator[2];
    double step_seconds[2];
    CollisionMode collision[2];
    double sample_seconds;
    double duration_seconds;
} LabConfiguration;

/* Whole number of steps in `seconds`, rejecting misaligned, non-finite or
 * over-budget counts. Every count derived from a run (total ticks, ticks per
 * sample, sample count) is at most its total ticks, so applying the cap here
 * bounds the whole run. */
bool lab_ticks_for(double seconds, double step, uint64_t *ticks);
bool lab_configuration_valid(const LabConfiguration *config);
bool lab_configuration_parse(const char *text, LabConfiguration *config);
bool lab_configuration_format(const LabConfiguration *config, char *out, size_t size);
#endif

#ifndef SOLAR_COMPARISON_H
#define SOLAR_COMPARISON_H
#include <stdio.h>
#include "lab_config.h"

#define SOLAR_COMPARISON_MAX_POINTS 1025
typedef enum LabField {
    LAB_RADIUS_M, LAB_SPEED_MPS, LAB_ENERGY_CHANGE, LAB_TOTAL_ENERGY_J,
    LAB_PHASE_ERROR_DEG, LAB_REFERENCE_ERROR_M, LAB_SPECIFIC_ENERGY_JPKG,
    LAB_LINEAR_MOMENTUM, LAB_ANGULAR_MOMENTUM, LAB_RESONANT_ANGLE_DEG,
    LAB_MIN_DISTANCE_M, LAB_COLLISION_COUNT, LAB_KINETIC_LOSS_J, LAB_BODY_COUNT,
    LAB_X_M, LAB_Y_M, LAB_Z_M, LAB_SUBJECT_PRESENT, LAB_TICKS, LAB_FIELD_COUNT
} LabField;

typedef struct ComparisonPoint {
    double time_seconds;
    double position_difference_m;
    double run[2][LAB_FIELD_COUNT];
} ComparisonPoint;

typedef struct ComparisonRun {
    bool initialized, configured, complete, failed;
    LabConfiguration config;
    SimulationSession runs[2];
    SolarSystem observed[2]; /* Matched snapshots for inspector/force consumers. */
    BodyId subject_id;
    uint64_t sample_index, total_samples, ticks_per_sample[2], stride;
    double minimum_distance[2], maximum_phase_error[2];
    size_t stored_count;
    ComparisonPoint points[SOLAR_COMPARISON_MAX_POINTS];
    ComparisonPoint latest;
} ComparisonRun;

/* Zero-initialize before first use. Owns two independent session allocations. */
bool comparison_start(ComparisonRun *run, const LabConfiguration *config);
void comparison_destroy(ComparisonRun *run);
size_t comparison_advance(ComparisonRun *run, size_t max_steps);
int comparison_subject_index(const ComparisonRun *run, size_t side);
size_t comparison_point_count(const ComparisonRun *run);
const ComparisonPoint *comparison_point_at(const ComparisonRun *run, size_t index);
const char *comparison_field_name(LabField field);
bool comparison_csv_begin(FILE *stream, const ComparisonRun *run, bool retained_history);
bool comparison_csv_sample(FILE *stream, const ComparisonPoint *point);
#endif

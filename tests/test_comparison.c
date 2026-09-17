#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "app/comparison.h"

static void test_configuration_round_trip_and_matched_bounded_runs(void)
{
    const char *text = "SOLAR_LAB_V1 circular 1 verlet 300 none verlet 150 none 3600 86400";
    LabConfiguration config, parsed;
    assert(lab_configuration_parse(text, &config));
    char serialized[SOLAR_LAB_CONFIG_BYTES];
    assert(lab_configuration_format(&config, serialized, sizeof(serialized)));
    assert(lab_configuration_parse(serialized, &parsed));
    assert(parsed.step_seconds[0] == 300 && parsed.step_seconds[1] == 150);
    assert(!lab_configuration_parse("SOLAR_LAB_V1 circular 1 verlet 301 none verlet 150 none 3600 86400", &parsed));
    assert(!lab_configuration_parse("SOLAR_LAB_V1 circular nan verlet 300 none verlet 150 none 3600 86400", &parsed));
    assert(!lab_configuration_parse("SOLAR_LAB_V2 circular 1 verlet 300 none verlet 150 none 3600 86400", &parsed));
    ComparisonRun small = {0}, large = {0};
    assert(comparison_start(&small, &config) && comparison_start(&large, &config));
    comparison_advance(&small, 1);
    assert(small.runs[0].clock.ticks + small.runs[1].clock.ticks == 1);
    assert(small.latest.time_seconds == 0); /* No unmatched sample leaks out. */
    assert(small.observed[0].elapsed_seconds == 0 && small.observed[1].elapsed_seconds == 0);
    while (!small.complete) comparison_advance(&small, 7);
    while (!large.complete) comparison_advance(&large, 2048);
    assert(!small.failed && !large.failed);
    assert(small.latest.time_seconds == 86400 && small.sample_index == 24);
    assert(small.runs[0].clock.ticks == 288 && small.runs[1].clock.ticks == 576);
    assert(small.latest.run[0][LAB_PHASE_ERROR_DEG] > small.latest.run[1][LAB_PHASE_ERROR_DEG]);
    assert(small.latest.position_difference_m > 0);
    for (size_t side = 0; side < 2; ++side)
        assert(vec3d_length(vec3d_sub(small.runs[side].system.bodies[1].position_m, large.runs[side].system.bodies[1].position_m)) == 0);
    LabConfiguration invalid = config; invalid.step_seconds[0] = NAN;
    assert(!comparison_start(&small, &invalid));
    assert(small.latest.time_seconds == 86400);
    comparison_destroy(&small); comparison_destroy(&large);
}

static void test_trace_is_bounded_and_missing_merged_subject_is_explicit(void)
{
    LabConfiguration config;
    assert(lab_configuration_parse("SOLAR_LAB_V1 circular 1 verlet 15 none verlet 15 none 15 45000", &config));
    ComparisonRun run = {0};
    assert(comparison_start(&run, &config));
    while (!run.complete) comparison_advance(&run, 2048);
    assert(comparison_point_count(&run) <= SOLAR_COMPARISON_MAX_POINTS);
    assert(comparison_point_at(&run, 0)->time_seconds == 0);
    assert(comparison_point_at(&run, comparison_point_count(&run)-1)->time_seconds == 45000);
    assert(lab_configuration_parse("SOLAR_LAB_V1 collision 1 verlet 0.1 bounce verlet 0.1 merge 1 20", &config));
    assert(comparison_start(&run, &config));
    while (!run.complete) comparison_advance(&run, 2048);
    assert(run.latest.run[0][LAB_BODY_COUNT] == 2 && run.latest.run[1][LAB_BODY_COUNT] == 1);
    assert(run.latest.run[0][LAB_SUBJECT_PRESENT] == 1 && run.latest.run[1][LAB_SUBJECT_PRESENT] == 0);
    assert(isnan(run.latest.position_difference_m));
    assert(run.latest.run[1][LAB_KINETIC_LOSS_J] > 999);
    comparison_destroy(&run);
}

int main(void)
{
    test_configuration_round_trip_and_matched_bounded_runs();
    test_trace_is_bounded_and_missing_merged_subject_is_explicit();
    puts("test_comparison passed");
    return 0;
}

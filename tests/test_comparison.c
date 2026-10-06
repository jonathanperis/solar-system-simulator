#include "require_assert.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "app/comparison.h"
#include "sim/constants.h"

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

static void test_total_ticks_per_side_are_capped(void)
{
    uint64_t ticks;
    assert(lab_ticks_for(SOLAR_LAB_MAX_TICKS, 1, &ticks) && ticks == SOLAR_LAB_MAX_TICKS);
    assert(!lab_ticks_for(SOLAR_LAB_MAX_TICKS + 1.0, 1, &ticks));
    assert(!lab_ticks_for(9e13, 0.01, &ticks));
    LabConfiguration config;
    /* Exactly at the cap is accepted (validation only; nothing runs here). */
    assert(lab_configuration_parse("SOLAR_LAB_V1 circular 1 verlet 15 none verlet 15 none 15000000000 15000000000", &config));
    assert(!lab_configuration_parse("SOLAR_LAB_V1 circular 1 verlet 15 none verlet 15 none 15000000015 15000000015", &config));
    /* One coarse side cannot hide an unbounded fine side. */
    assert(!lab_configuration_parse("SOLAR_LAB_V1 circular 1 verlet 3600 none verlet 3600 none 3600 3.6e18", &config));
    assert(!lab_configuration_parse("SOLAR_LAB_V1 circular 1 verlet 3600 none verlet 1 none 3600 1.8e9", &config));
}

static void test_measurements_follow_the_subject_not_the_selection(void)
{
    LabConfiguration config;
    assert(lab_configuration_parse("SOLAR_LAB_V1 earth-moon 1 verlet 300 none verlet 300 none 3600 7200", &config));
    ComparisonRun run = {0};
    assert(comparison_start(&run, &config));
    /* Both sides integrate identically; only side A's inspector selection moves. */
    size_t subject = (size_t)comparison_subject_index(&run, 0);
    run.runs[0].selected_body_index = subject == 0 ? 1 : 0;
    while (!run.complete) comparison_advance(&run, 2048);
    const double *a = run.latest.run[0], *b = run.latest.run[1];
    assert(isfinite(b[LAB_SPEED_MPS]) && isfinite(b[LAB_SPECIFIC_ENERGY_JPKG]));
    assert(a[LAB_SPEED_MPS] == b[LAB_SPEED_MPS]);
    assert(a[LAB_SPECIFIC_ENERGY_JPKG] == b[LAB_SPECIFIC_ENERGY_JPKG]);
    comparison_destroy(&run);
}

static void test_unformattable_configuration_writes_no_header(void)
{
    ComparisonRun run = {0}; /* Never configured: its descriptor cannot be formatted. */
    FILE *stream = tmpfile();
    assert(stream);
    assert(!comparison_csv_begin(stream, &run, false));
    assert(ftell(stream) == 0);
    fclose(stream);
}

/* A run keeps going after a contact-sphere crossing (it is still a numerical
 * experiment) but withholds its analytical reference errors and maximum phase
 * error from the first crossing onward (A54). */
static void test_contact_withholds_reference_errors(void)
{
    LabConfiguration config;
    assert(!lab_configuration_parse("SOLAR_LAB_V1 phobos 0.5 verlet 300 none verlet 15 none 21600 8640000", &config));
    assert(lab_configuration_parse("SOLAR_LAB_V1 phobos 0.73 verlet 1800 none verlet 15 none 21600 8640000", &config));
    ComparisonRun run = {0};
    assert(comparison_start(&run, &config));
    assert(run.latest.run[0][LAB_CONTACT_SPHERE_CROSSED] == 0 && run.latest.run[1][LAB_CONTACT_SPHERE_CROSSED] == 0);
    while (!run.complete && !run.failed) comparison_advance(&run, 2048);
    assert(!run.failed);
    assert(strcmp(comparison_field_name(LAB_CONTACT_SPHERE_CROSSED), "contact_sphere_crossed") == 0);
    /* Run A's 1800 s steps are about 1/8 of this low orbit. Within 100 days
     * one step's straight chord cuts across Mars's contact sphere although
     * every sampled position stays above it: a coarse-step artifact the
     * conservative swept test still flags. Run B (15 s) never crosses.
     * Deterministic: all C is compiled with -ffp-contract=off. */
    const double contact = SOLAR_MARS_RADIUS_M + SOLAR_PHOBOS_RADIUS_M;
    assert(run.latest.run[0][LAB_CONTACT_SPHERE_CROSSED] == 1);
    assert(run.minimum_distance[0] > contact);
    assert(isnan(run.latest.run[0][LAB_REFERENCE_ERROR_M]) && isnan(run.latest.run[0][LAB_PHASE_ERROR_DEG]));
    assert(isnan(run.maximum_phase_error[0]) && isfinite(run.maximum_phase_error[1]));
    assert(run.latest.run[1][LAB_CONTACT_SPHERE_CROSSED] == 0);
    assert(isfinite(run.latest.run[1][LAB_REFERENCE_ERROR_M]) && isfinite(run.latest.run[1][LAB_PHASE_ERROR_DEG]));
    assert(run.minimum_distance[1] >= contact);
    comparison_destroy(&run);
}

int main(void)
{
    test_total_ticks_per_side_are_capped();
    test_measurements_follow_the_subject_not_the_selection();
    test_unformattable_configuration_writes_no_header();
    test_contact_withholds_reference_errors();
    test_configuration_round_trip_and_matched_bounded_runs();
    test_trace_is_bounded_and_missing_merged_subject_is_explicit();
    puts("test_comparison passed");
    return 0;
}

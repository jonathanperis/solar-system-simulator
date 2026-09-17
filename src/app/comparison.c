#include "comparison.h"
#include "csv_export.h"
#include <math.h>
#include <string.h>

const char *comparison_field_name(LabField field)
{
    const char *names[] = {"radius_m", "speed_mps", "energy_change", "total_energy_j", "phase_error_deg",
        "reference_error_m", "specific_energy_jpkg", "linear_momentum_kg_mps", "angular_momentum_kg_m2ps",
        "resonant_angle_deg", "minimum_distance_m", "collision_count", "kinetic_loss_j", "body_count",
        "x_m", "y_m", "z_m", "subject_present", "ticks"};
    return field >= 0 && field < LAB_FIELD_COUNT ? names[field] : "";
}

int comparison_subject_index(const ComparisonRun *run, size_t side)
{
    if (side > 1) return -1;
    const SolarSystem *system = &run->runs[side].system;
    for (size_t i = 0; i < system->body_count; ++i) if (system->bodies[i].id == run->subject_id) return (int)i;
    return -1;
}

static Vec3d relative_position(const SolarSystem *system, size_t index)
{
    int parent = solar_system_parent_index(system, index);
    return parent < 0 ? system->bodies[index].position_m : vec3d_sub(system->bodies[index].position_m, system->bodies[parent].position_m);
}

static void measure(ComparisonRun *run, size_t side)
{
    SimulationSession *session = &run->runs[side];
    const SolarSystem *system = &session->system;
    double *values = run->latest.run[side];
    for (size_t i = 0; i < LAB_FIELD_COUNT; ++i) values[i] = NAN;
    PhysicsDiagnostics d = physics_diagnostics(system);
    values[LAB_ENERGY_CHANGE] = simulation_session_energy_change(session, &d);
    values[LAB_TOTAL_ENERGY_J] = d.total_energy_j;
    values[LAB_LINEAR_MOMENTUM] = vec3d_length(d.momentum_kg_mps);
    values[LAB_ANGULAR_MOMENTUM] = vec3d_length(d.angular_momentum_kg_m2ps);
    values[LAB_RESONANT_ANGLE_DEG] = lesson_resonant_angle_degrees(system);
    values[LAB_COLLISION_COUNT] = (double)session->clock.collision_count;
    values[LAB_KINETIC_LOSS_J] = session->clock.dissipated_energy_j;
    values[LAB_BODY_COUNT] = (double)system->body_count;
    values[LAB_TICKS] = (double)session->clock.ticks;
    values[LAB_MIN_DISTANCE_M] = run->minimum_distance[side];
    int index = comparison_subject_index(run, side);
    values[LAB_SUBJECT_PRESENT] = index >= 0;
    if (index < 0) return;
    Vec3d position = relative_position(system, (size_t)index);
    BodyInspection body = simulation_session_inspect(session);
    values[LAB_X_M] = position.x; values[LAB_Y_M] = position.y; values[LAB_Z_M] = position.z;
    values[LAB_RADIUS_M] = vec3d_length(position);
    values[LAB_SPEED_MPS] = body.has_parent ? body.speed_mps : vec3d_length(system->bodies[index].velocity_mps);
    values[LAB_SPECIFIC_ENERGY_JPKG] = body.has_parent ? body.specific_energy_jpkg : NAN;
    Vec3d expected;
    if (lesson_reference_position(&session->initial_system, lesson_subject_index(run->config.lesson), run->latest.time_seconds, &expected)) {
        values[LAB_REFERENCE_ERROR_M] = vec3d_length(vec3d_sub(position, expected));
        values[LAB_PHASE_ERROR_DEG] = atan2(vec3d_length(vec3d_cross(position, expected)), vec3d_dot(position, expected)) * 180 / acos(-1.0);
        run->maximum_phase_error[side] = fmax(run->maximum_phase_error[side], values[LAB_PHASE_ERROR_DEG]);
    }
}

static void record_checkpoint(ComparisonRun *run)
{
    run->observed[0] = run->runs[0].system;
    run->observed[1] = run->runs[1].system;
    run->latest.time_seconds = (double)run->sample_index * run->config.sample_seconds;
    measure(run, 0); measure(run, 1);
    run->latest.position_difference_m = NAN;
    if (run->latest.run[0][LAB_SUBJECT_PRESENT] && run->latest.run[1][LAB_SUBJECT_PRESENT]) {
        double *a = run->latest.run[0], *b = run->latest.run[1];
        run->latest.position_difference_m = vec3d_length((Vec3d){a[LAB_X_M]-b[LAB_X_M], a[LAB_Y_M]-b[LAB_Y_M], a[LAB_Z_M]-b[LAB_Z_M]});
    }
    if (run->sample_index % run->stride == 0) {
        run->points[run->stored_count++] = run->latest;
        if (run->stored_count == SOLAR_COMPARISON_MAX_POINTS) {
            size_t count = 0;
            for (size_t i = 0; i < run->stored_count; i += 2) run->points[count++] = run->points[i];
            run->stored_count = count;
            run->stride *= 2;
        }
    }
}

void comparison_destroy(ComparisonRun *run)
{
    if (run->initialized) for (size_t side = 0; side < 2; ++side) simulation_session_destroy(&run->runs[side]);
    memset(run, 0, sizeof(*run));
}

bool comparison_start(ComparisonRun *run, const LabConfiguration *config)
{
    if (!lab_configuration_valid(config)) return false;
    /* Copy first: callers may restart using run->config itself. */
    LabConfiguration next = *config;
    comparison_destroy(run);
    run->config = next;
    run->initialized = run->configured = true;
    run->stride = 1;
    lab_ticks_for(next.duration_seconds, next.sample_seconds, &run->total_samples);
    for (size_t side = 0; side < 2; ++side) {
        run->runs[side] = simulation_session_create();
        simulation_session_start_configured_lesson(&run->runs[side], next.lesson, next.velocity_factor,
            next.integrator[side], next.step_seconds[side], next.collision[side]);
        lab_ticks_for(next.sample_seconds, next.step_seconds[side], &run->ticks_per_sample[side]);
        size_t index = lesson_subject_index(next.lesson);
        run->subject_id = run->runs[side].system.bodies[index].id;
        run->minimum_distance[side] = vec3d_length(relative_position(&run->runs[side].system, index));
    }
    record_checkpoint(run);
    return true;
}

size_t comparison_advance(ComparisonRun *run, size_t max_steps)
{
    if (!run->configured || run->complete || run->failed) return 0;
    size_t steps = 0;
    /* Neither plotting nor frame duration changes the physics sequence. Partial
     * work stays private until both integrators reach the next shared checkpoint. */
    while (steps < max_steps && !run->complete) {
        bool reached = true;
        for (size_t side = 0; side < 2; ++side) {
            uint64_t target = (run->sample_index + 1) * run->ticks_per_sample[side];
            SimulationSession *session = &run->runs[side];
            while (steps < max_steps && session->clock.ticks < target) {
                simulation_session_advance_tick(session, false);
                ++steps;
                int index = comparison_subject_index(run, side);
                if (index >= 0) run->minimum_distance[side] = fmin(run->minimum_distance[side], vec3d_length(relative_position(&session->system, (size_t)index)));
            }
            if (session->clock.ticks != target) reached = false;
        }
        if (!reached) break;
        for (size_t side = 0; side < 2; ++side) for (size_t i = 0; i < run->runs[side].system.body_count; ++i) {
            const Body *body = &run->runs[side].system.bodies[i];
            if (!isfinite(vec3d_length(body->position_m)) || !isfinite(vec3d_length(body->velocity_mps))) run->failed = true;
        }
        if (run->failed) break;
        ++run->sample_index;
        record_checkpoint(run);
        run->complete = run->sample_index == run->total_samples;
        /* Return on a checkpoint so headless callers can stream every sample. */
        break;
    }
    return steps;
}

size_t comparison_point_count(const ComparisonRun *run)
{
    if (!run->configured) return 0;
    return run->stored_count + (run->points[run->stored_count-1].time_seconds < run->latest.time_seconds ? 1 : 0);
}

const ComparisonPoint *comparison_point_at(const ComparisonRun *run, size_t index)
{
    if (index >= comparison_point_count(run)) return NULL;
    return index < run->stored_count ? &run->points[index] : &run->latest;
}

bool comparison_csv_begin(FILE *stream, const ComparisonRun *run, bool retained_history)
{
    char definition[SOLAR_LAB_CONFIG_BYTES];
    lab_configuration_format(&run->config, definition, sizeof(definition));
    fprintf(stream, "# solar-comparison-v1\n# revision: %s\n# configuration: %s# samples: %s\n",
        solar_build_revision(), definition, retained_history ? "bounded uniformly coarsened history plus live endpoint" : "every configured checkpoint");
    fputs("time_s,position_difference_m", stream);
    for (size_t side = 0; side < 2; ++side) for (int field = 0; field < LAB_FIELD_COUNT; ++field)
        fprintf(stream, ",%s_%c", comparison_field_name((LabField)field), side == 0 ? 'a' : 'b');
    fputc('\n', stream);
    return !ferror(stream);
}

bool comparison_csv_sample(FILE *stream, const ComparisonPoint *point)
{
    fprintf(stream, "%.17g,", point->time_seconds);
    if (isfinite(point->position_difference_m)) fprintf(stream, "%.17g", point->position_difference_m);
    for (size_t side = 0; side < 2; ++side) for (size_t field = 0; field < LAB_FIELD_COUNT; ++field) {
        fputc(',', stream);
        if (isfinite(point->run[side][field])) fprintf(stream, "%.17g", point->run[side][field]);
    }
    fputc('\n', stream);
    return !ferror(stream);
}

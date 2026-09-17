#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "app/csv_export.h"
#include "app/comparison.h"

static int usage(FILE *stream)
{
    fputs("solar-lab [--scene NAME] (use --lessons for available presets)\n"
        "  [--duration SECONDS | --days DAYS] [--dt SECONDS] [--sample SECONDS]\n"
        "  [--integrator verlet|euler] [--velocity-factor 0.1..2] [--collision none|bounce|merge] [--output FILE]\n"
        "  [--experiment FILE] | --compare FILE | --catalog | --lessons | --version | --help\n"
        "Defaults: circular, 86400 s duration, 15 s step, 3600 s samples, Verlet.\n"
        "Duration and sample spacing must be whole multiples of dt. Core/catalog use 15 s Verlet.\n", stream);
    return stream == stdout ? 0 : 2;
}

static bool number(const char *text, double *value)
{
    char *end;
    errno = 0;
    *value = strtod(text, &end);
    return !errno && end != text && !*end && isfinite(*value) && *value > 0;
}

static bool read_input(const char *path, char *text, size_t size)
{
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    size_t bytes = fread(text, 1, size - 1, file);
    bool ok = !ferror(file) && fgetc(file) == EOF && !memchr(text, 0, bytes);
    fclose(file); text[bytes] = 0;
    return ok;
}

static int compare_file(const char *path)
{
    char text[SOLAR_LAB_CONFIG_BYTES]; LabConfiguration config;
    if (!read_input(path, text, sizeof(text)) || !lab_configuration_parse(text, &config)) {
        fputs("Invalid comparison descriptor. Check version, lesson, contact policy and aligned sampling.\n", stderr); return 2;
    }
    ComparisonRun run = {0};
    comparison_start(&run, &config);
    bool ok = comparison_csv_begin(stdout, &run, false) && comparison_csv_sample(stdout, &run.latest);
    while (ok && !run.complete && !run.failed) {
        uint64_t previous = run.sample_index;
        comparison_advance(&run, 2048);
        if (previous != run.sample_index) ok = comparison_csv_sample(stdout, &run.latest);
    }
    ok = ok && !run.failed && fflush(stdout) == 0;
    comparison_destroy(&run);
    if (!ok) fputs("Comparison failed: numerical state or CSV output unavailable.\n", stderr);
    return ok ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--help")) return usage(stdout);
    if (argc == 2 && !strcmp(argv[1], "--version")) { puts(solar_build_revision()); return 0; }
    if (argc == 3 && !strcmp(argv[1], "--compare")) return compare_file(argv[2]);
    if (argc == 2 && !strcmp(argv[1], "--lessons")) {
        for (int i = 0; i < LESSON_COUNT; ++i) puts(lesson_name((LessonPreset)i));
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--catalog")) {
        SolarSystem system = solar_system_create_current();
        const char *kinds[] = {"Star", "Planet", "Moon", "Asteroid", "Dwarf planet"};
        puts("id\tname\tkind\tparent");
        for (size_t i = 0; i < system.body_count; ++i) {
            int parent = solar_system_parent_index(&system, i);
            printf("%d\t%s\t%s\t%s\n", system.bodies[i].id, system.bodies[i].name,
                kinds[system.bodies[i].kind], parent < 0 ? "None" : system.bodies[parent].name);
        }
        return ferror(stdout) ? 1 : 0;
    }
    LessonPreset lesson = LESSON_CIRCULAR;
    PhysicsIntegrator method = PHYSICS_VERLET;
    double duration = 86400, dt = 15, sample = 3600, factor = 1;
    const char *output = NULL, *experiment = NULL;
    bool explicit_scene = false;
    bool explicit_collision = false;
    CollisionMode collision = COLLISION_NONE;
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) return usage(stderr);
        const char *key = argv[i], *value = argv[i + 1];
        if (!strcmp(key, "--scene")) {
            explicit_scene = true;
            lesson = LESSON_COUNT;
            for (int j = 0; j < LESSON_COUNT; ++j) if (!strcmp(value, lesson_name((LessonPreset)j))) lesson = (LessonPreset)j;
            if (lesson == LESSON_COUNT) return usage(stderr);
        } else if (!strcmp(key, "--integrator")) {
            if (strcmp(value, "verlet") && strcmp(value, "euler")) return usage(stderr);
            method = !strcmp(value, "euler") ? PHYSICS_EULER : PHYSICS_VERLET;
        } else if (!strcmp(key, "--collision")) {
            explicit_collision = true; collision = (CollisionMode)-1;
            for (int mode = COLLISION_NONE; mode <= COLLISION_MERGE; ++mode)
                if (!strcmp(value, collision_mode_name((CollisionMode)mode))) collision = (CollisionMode)mode;
            if ((int)collision < 0) return usage(stderr);
        } else if (!strcmp(key, "--output")) output = value;
        else if (!strcmp(key, "--experiment")) experiment = value;
        else {
            double parsed;
            if (!number(value, &parsed)) return usage(stderr);
            if (!strcmp(key, "--duration")) duration = parsed;
            else if (!strcmp(key, "--days")) duration = parsed * 86400;
            else if (!strcmp(key, "--dt")) dt = parsed;
            else if (!strcmp(key, "--sample")) sample = parsed;
            else if (!strcmp(key, "--velocity-factor")) factor = parsed;
            else return usage(stderr);
        }
    }
    uint64_t total_ticks, sample_ticks;
    if (!explicit_collision && lesson == LESSON_COLLISION) collision = COLLISION_BOUNCE;
    if (!lab_ticks_for(duration, dt, &total_ticks) || !lab_ticks_for(sample, dt, &sample_ticks) ||
        (experiment && (explicit_scene || dt != 15 || factor != 1 || method != PHYSICS_VERLET || collision != COLLISION_NONE))) return usage(stderr);
    SimulationSession session = simulation_session_create();
    bool valid;
    if (experiment) {
        char text[SOLAR_EXPERIMENT_TEXT_BYTES];
        valid = read_input(experiment, text, sizeof(text)) && simulation_session_start_experiment(&session, text);
    } else valid = simulation_session_start_configured_lesson(&session, lesson, factor, method, dt, collision);
    if (!valid) {
        simulation_session_destroy(&session);
        fputs("Invalid lesson configuration or experiment input.\n", stderr);
        return 2;
    }
    FILE *stream = output ? fopen(output, "w") : stdout;
    if (!stream) { perror("CSV output"); simulation_session_destroy(&session); return 1; }
    bool ok = simulation_csv_begin(stream, &session) && simulation_csv_sample(stream, &session);
    /* Stream samples instead of retaining a series. No graphics or wall clock
     * enters this run: ticks are the sole source of simulation time. */
    physics_compute_accelerations(session.system.bodies, session.system.body_count);
    for (uint64_t tick = 1; ok && tick <= total_ticks; ++tick) {
        simulation_session_advance_tick(&session, false);
        if (tick % sample_ticks == 0 || tick == total_ticks) ok = simulation_csv_sample(stream, &session);
    }
    if (fflush(stream) != 0) ok = false;
    if (output && fclose(stream) != 0) ok = false;
    simulation_session_destroy(&session);
    if (!ok) fputs("Could not finish writing CSV output.\n", stderr);
    return ok ? 0 : 1;
}

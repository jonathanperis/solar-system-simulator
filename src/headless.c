#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L /* sigaction, unlink */
#endif

#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "app/csv_export.h"
#include "app/comparison.h"
#include "app/input_file.h"

#ifndef _WIN32
/* While a --output run is in progress its rows go to a private temporary
 * beside the destination. Ctrl-C or `kill` would otherwise leave that file
 * behind. A signal can interrupt any instruction, so the handler may only use
 * async-signal-safe calls (unlink, signal, raise) and state that is complete
 * before it is published: a fixed buffer, then a sig_atomic_t flag. */
static char cleanup_temp_path[4096];
static volatile sig_atomic_t cleanup_temp_active;

static void remove_temp_and_reraise(int signum)
{
    if (cleanup_temp_active) unlink(cleanup_temp_path);
    /* Restore the default action and deliver the signal again, so the shell
     * sees a death by SIGINT/SIGTERM rather than an ordinary exit code. */
    signal(signum, SIG_DFL);
    raise(signum);
}

static void protect_temporary(const char *temp_path)
{
    size_t length = strlen(temp_path);
    if (length >= sizeof(cleanup_temp_path)) return; /* Too long to protect; the run still works. */
    memcpy(cleanup_temp_path, temp_path, length + 1);
    cleanup_temp_active = 1;
    const int signals[] = {SIGINT, SIGTERM, SIGHUP};
    for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); ++i) {
        struct sigaction previous, action = {.sa_handler = remove_temp_and_reraise};
        sigemptyset(&action.sa_mask);
        /* A shell starts background jobs with SIGINT ignored; keep that choice. */
        if (sigaction(signals[i], NULL, &previous) == 0 && previous.sa_handler == SIG_IGN) continue;
        sigaction(signals[i], &action, NULL);
    }
}

static void release_temporary(void)
{
    cleanup_temp_active = 0;
}
#else
static void protect_temporary(const char *temp_path) { (void)temp_path; }
static void release_temporary(void) {}
#endif

static int usage(FILE *stream)
{
    fputs("solar-lab [--scene NAME] (use --lessons for available presets)\n"
        "  [--duration SECONDS | --days DAYS] [--dt SECONDS] [--sample SECONDS]\n"
        "  [--integrator verlet|euler] [--velocity-factor F (lesson minimum..2)] [--collision none|bounce|merge] [--output FILE]\n"
        "  [--experiment FILE] | --compare FILE | --catalog | --lessons | --version | --help\n"
        "Defaults: circular, 86400 s duration, 15 s step, 3600 s samples, Verlet.\n"
        "Duration and sample spacing must be whole multiples of dt. Core/catalog use 15 s Verlet.\n"
        "At most 1e9 ticks (duration/dt) per run.\n"
        "--output must name a new file or an existing regular file you can write, in a directory\n"
        "you can write: the series goes to a temporary beside it and replaces it only after a\n"
        "complete run (symlinks, FIFOs and devices are refused).\n", stream);
    return stream == stdout ? 0 : 2;
}

static bool number(const char *text, double *value)
{
    char *end;
    errno = 0;
    *value = strtod(text, &end);
    return !errno && end != text && !*end && isfinite(*value) && *value > 0;
}

static int compare_file(const char *path)
{
    char text[SOLAR_LAB_CONFIG_BYTES]; LabConfiguration config;
    if (!solar_read_text_file(path, text, sizeof(text)) || !lab_configuration_parse(text, &config)) {
        fputs("Invalid comparison descriptor. Check version, lesson, contact policy and aligned sampling.\n", stderr); return 2;
    }
    /* A ComparisonRun holds two whole sessions plus 1025 retained points,
     * roughly half a megabyte: too much for a default thread stack on some
     * systems, so it gets static storage (as in lab_web.c). This function
     * runs once per process, so the single instance is never shared. */
    static ComparisonRun run;
    if (!comparison_start(&run, &config)) {
        comparison_destroy(&run);
        fputs("Comparison could not start: a lesson rejected this configuration.\n", stderr); return 2;
    }
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
        } else if (!strcmp(key, "--output")) {
            /* Refuse an empty name or a directory spelling before any work. */
            if (!*value || value[strlen(value) - 1] == '/') {
                fputs("--output needs a file name, not an empty value or a directory.\n", stderr);
                return usage(stderr);
            }
            output = value;
        }
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
        valid = solar_read_text_file(experiment, text, sizeof(text)) && simulation_session_start_experiment(&session, text);
    } else valid = simulation_session_start_configured_lesson(&session, lesson, factor, method, dt, collision);
    if (!valid) {
        simulation_session_destroy(&session);
        fputs("Invalid lesson configuration or experiment input.\n", stderr);
        return 2;
    }
    CsvOutputFile file = {0};
    if (output && !simulation_csv_output_open(&file, output)) {
        fprintf(stderr, "CSV output %s: %s (use a new path or an existing regular file)\n", output, strerror(errno));
        simulation_session_destroy(&session);
        return 1;
    }
    if (output) protect_temporary(file.temp_path);
    FILE *stream = output ? file.stream : stdout;
    bool ok = simulation_csv_begin(stream, &session) && simulation_csv_sample(stream, &session);
    bool finite = true;
    /* Stream samples instead of retaining a series. No graphics or wall clock
     * enters this run: ticks are the sole source of simulation time. */
    physics_compute_accelerations(session.system.bodies, session.system.body_count);
    for (uint64_t tick = 1; ok && tick <= total_ticks; ++tick) {
        simulation_session_advance_tick(&session, false);
        if (tick % sample_ticks == 0 || tick == total_ticks) {
            /* Once a position or velocity overflows, every later row is
             * meaningless (NaN spreads through the force sum). Stop and fail,
             * as the comparison runner does, instead of exiting with success. */
            finite = simulation_session_state_is_finite(&session);
            ok = finite && simulation_csv_sample(stream, &session);
        }
    }
    /* A file destination is replaced only after every sample is written; on
     * failure its temporary is deleted and any previous file stays intact. */
    if (output) {
        if (ok) ok = simulation_csv_output_commit(&file);
        else simulation_csv_output_abort(&file);
    } else if (fflush(stream) != 0) ok = false;
    release_temporary();
    simulation_session_destroy(&session);
    if (!finite) fputs("Numerical state became non-finite; reduce the timestep or check the input.\n", stderr);
    if (!ok) fputs(output ? "Could not finish writing CSV output; no partial file was kept.\n"
        : "Could not finish writing CSV output.\n", stderr);
    return ok ? 0 : 1;
}

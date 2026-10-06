#include "require_assert.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/simulation_session.h"
#include "sim/constants.h"
static void test_rows_are_strict_tab_separated_values(void)
{
    SolarSystem system;
    char names[SOLAR_EXPERIMENT_CAPACITY][SOLAR_EXPERIMENT_NAME_BYTES];
    const char *header = "SOLAR_EXPERIMENT_V1 2461200.5\n";
    const char *valid = "20000004\tVesta\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n";
    char text[512];
    snprintf(text, sizeof(text), "%s%s", header, valid);
    assert(experiment_parse(text, &system, names));
    assert(system.body_count == 10 && strcmp(names[0], "Vesta") == 0);
    /* The final row may omit its newline; exponent notation stays accepted. */
    assert(experiment_parse("SOLAR_EXPERIMENT_V1 2461200.5\n"
        "20000004\tVesta\t2e0\t1E-1\t0\t0\t0\t2461200.5\t0\t0\t2\t2", &system, names));

    /* Each rejected row differs from the valid row in one way. Spaces are not
     * separators, names cannot carry line breaks or control bytes, and numbers
     * are plain finite decimals without whitespace, hex or trailing junk. */
    const char *rejected[] = {
        "20000004\tVesta\t2 .1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\t0\n",
        "20000004 Vesta\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n",
        "20000004\tVesta\t2\t.1\t0\t0\t0\t2461200.5\t0\t0 \t2\t2\n",
        "20000004\tVes\nta\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n",
        "20000004\tVes\x01ta\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n",
        "20000004\t\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n",
        "20000004\tVesta\t 2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n",
        "20000004\tVesta\t2x\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n",
        "20000004\tVesta\t0x1p1\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n",
        "20000004\tVesta\tinf\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n",
        "20000004\tVesta\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\t\n",
        "20000004\tVesta\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\n",
        "20000004\tVesta\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\r\n",
        " 20000004\tVesta\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n",
        "+20000004\tVesta\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n",
        "20000004\tVesta\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n\n",
        "20000004\tVesta\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2.0\t2\n",
        "20000004\tVesta\t2\t.1\t0\t0\t0\t2461200.5\t5\t0\t2\t2\n",
    };
    for (size_t i = 0; i < sizeof(rejected) / sizeof(rejected[0]); ++i) {
        snprintf(text, sizeof(text), "%s%s", header, rejected[i]);
        assert(!experiment_parse(text, &system, names));
    }
    /* The exact header text and the numeric epoch name the same instant. */
    assert(strtod(SOLAR_CATALOG_EPOCH_TEXT, NULL) == SOLAR_CATALOG_EPOCH_JD);
    const char *headers[] = {"SOLAR_EXPERIMENT_V1  2461200.5\n", "SOLAR_EXPERIMENT_V1\t2461200.5\n",
        "SOLAR_EXPERIMENT_V1 2461200.5 \n", "SOLAR_EXPERIMENT_V1 0x1.2c70840000000p+21\n"};
    for (size_t i = 0; i < sizeof(headers) / sizeof(headers[0]); ++i) {
        snprintf(text, sizeof(text), "%s%s", headers[i], valid);
        assert(!experiment_parse(text, &system, names));
    }
}

/* Experiments have no moons, so each planet stands for its whole system: the
 * Horizons snapshot gives system barycenters and the masses are DE440 system
 * GMs (https://ssd.jpl.nasa.gov/astro_par.html). The core scene, which does
 * contain moons, keeps planet-only masses. */
static void test_experiment_planets_are_whole_systems(void)
{
    SolarSystem system;
    char names[SOLAR_EXPERIMENT_CAPACITY][SOLAR_EXPERIMENT_NAME_BYTES];
    assert(experiment_parse("SOLAR_EXPERIMENT_V1 2461200.5\n"
        "20000004\tVesta\t2.148\t0.09\t7.14\t103.7\t151.4\t2461000.5\t0\t0\t2\t2\n", &system, names));
    const double system_gm_km3_s2[] = {22031.868551, 324858.592, 398600.435507 + 4902.800118, 42828.375816,
        126712764.1, 37940584.8418, 5794556.4, 6836527.10058};
    for (size_t i = 0; i < 8; ++i)
        assert(fabs(system.bodies[1 + i].mass_kg * SOLAR_G / (system_gm_km3_s2[i] * 1e9) - 1) < 1e-9);
    SolarSystem core = solar_system_create_current();
    assert(core.bodies[3].mass_kg == SOLAR_EARTH_MASS_KG && core.bodies[9].mass_kg == SOLAR_JUPITER_MASS_KG);
}

int main(void)
{
    test_rows_are_strict_tab_separated_values();
    test_experiment_planets_are_whole_systems();
    SimulationSession session = simulation_session_create();
    const char *input = "SOLAR_EXPERIMENT_V1 2461200.5\n"
        "20000004\tVesta\t2.148\t0.09\t7.14\t103.7\t151.4\t2461000.5\t0\t0\t2\t2\n"
        "50788063\tOumuamua\t0.2559\t1.201\t122.7\t24.6\t241.8\t2458006\t0\t0\t2\t2\n";
    assert(simulation_session_start_experiment(&session, input));
    assert(session.system.body_count == 11 && session.system.bodies[9].id == BODY_ID_VESTA);
    assert(session.system.bodies[7].id == BODY_ID_URANUS && session.system.bodies[8].id == BODY_ID_NEPTUNE);
    assert(strcmp(session.system.bodies[10].name, "Oumuamua") == 0);
    session.paused = true;
    simulation_session_select_body(&session, 10);
    Vec3d original = session.system.bodies[10].position_m;
    simulation_session_single_step(&session);
    assert(session.system.elapsed_seconds == 15);
    simulation_session_reset(&session);
    assert(session.system.body_count == 11 && session.selected_body_index == 10);
    assert(vec3d_length(vec3d_sub(original, session.system.bodies[10].position_m)) == 0);
    assert(strcmp(session.system.bodies[10].name, "Oumuamua") == 0);
    assert(!simulation_session_start_experiment(&session, "SOLAR_EXPERIMENT_V1 2451545\n"));
    assert(session.system.body_count == 11 && session.selected_body_index == 10);
    assert(!simulation_session_start_experiment(&session,
        "SOLAR_EXPERIMENT_V1 2461200.5\n20000001\tBad\tNaN\t.2\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n"));
    char maximum[8192]="SOLAR_EXPERIMENT_V1 2461200.5\n";
    for(int i=0;i<16;++i) {
        size_t used=strlen(maximum);
        snprintf(maximum+used,sizeof(maximum)-used,"%d\tTest %d\t2\t.1\t5\t3\t10\t2461200.5\t0\t0\t2\t2\n",20000001+i,i);
    }
    assert(simulation_session_start_experiment(&session,maximum));
    assert(session.system.body_count==25);
    strcat(maximum,"20000050\tExtra\t2\t.1\t5\t3\t10\t2461200.5\t0\t0\t2\t2\n");
    assert(!simulation_session_start_experiment(&session,maximum));
    assert(session.system.body_count==25);
    assert(!simulation_session_start_experiment(&session,
        "SOLAR_EXPERIMENT_V1 2461200.5\n20000004\tVesta\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n"
        "20000004\tDuplicate\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n"));
    char name[SOLAR_EXPERIMENT_NAME_BYTES + 1], named_input[512];
    for (size_t length = SOLAR_EXPERIMENT_NAME_BYTES - 1; length <= SOLAR_EXPERIMENT_NAME_BYTES; ++length) {
        memset(name, 'x', length);
        name[length] = '\0';
        snprintf(named_input, sizeof(named_input),
            "SOLAR_EXPERIMENT_V1 2461200.5\n20000004\t%s\t2\t.1\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n", name);
        bool accepted = simulation_session_start_experiment(&session, named_input);
        assert(accepted == (length < SOLAR_EXPERIMENT_NAME_BYTES));
        assert(session.system.body_count == 10);
        assert(strlen(session.system.bodies[9].name) == SOLAR_EXPERIMENT_NAME_BYTES - 1);
        simulation_session_reset(&session);
        assert(strlen(session.system.bodies[9].name) == SOLAR_EXPERIMENT_NAME_BYTES - 1);
    }
    simulation_session_destroy(&session);
    puts("test_experiment passed");
}

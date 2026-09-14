#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "app/simulation_session.h"
#include "sim/constants.h"
int main(void)
{
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
    simulation_session_destroy(&session);
    puts("test_experiment passed");
}

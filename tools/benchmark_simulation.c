/* Headless throughput probe for the actual fixed-step scene and trail policy.
 * Compile natively or with Emscripten/Node; browser rendering is measured in UI. */
#include <stdio.h>
#include <time.h>

#include "app/body_trails.h"
#include "app/simulation_step.h"
#include "sim/constants.h"
#include "sim/experiment.h"

int main(int argc, char **argv)
{
    SolarSystem system = solar_system_create_current();
    char names[SOLAR_EXPERIMENT_CAPACITY][SOLAR_EXPERIMENT_NAME_BYTES];
    if(argc==2) {
        FILE *file=fopen(argv[1],"rb");
        char input[SOLAR_EXPERIMENT_TEXT_BYTES];
        size_t count=file?fread(input,1,sizeof(input)-1,file):0;
        bool complete=file && !ferror(file) && fgetc(file)==EOF;
        if(file)fclose(file);
        input[count]=0;
        if(!complete || !experiment_parse(input,&system,names))return 1;
    }
    BodyTrails trails = body_trails_create();
    body_trails_record_system(&trails, &system);
    struct timespec start, stop;
    timespec_get(&start, TIME_UTC);
    SimulationClock clock = {0};
    solar_app_step_system_with_trails(&system, &trails, &clock, 15 * SOLAR_DAY_SECONDS, 86400);
    timespec_get(&stop, TIME_UTC);
    double real_seconds = (double)(stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / 1e9;
    printf("%zu bodies, %.0f simulated seconds in %.6f wall seconds: %.3f days/second\n",
        system.body_count, system.elapsed_seconds, real_seconds, system.elapsed_seconds / SOLAR_DAY_SECONDS / real_seconds);
    body_trails_destroy(&trails);
}

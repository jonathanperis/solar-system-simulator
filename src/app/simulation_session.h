#ifndef SOLAR_SIMULATION_SESSION_H
#define SOLAR_SIMULATION_SESSION_H

#include "simulation_step.h"

#define SOLAR_SPEED_PRESET_COUNT 3

typedef struct SimulationSession {
    SolarSystem system;
    BodyTrails trails;
    SimulationClock clock;
    bool paused;
    int speed_preset;
    size_t selected_body_index;
} SimulationSession;

typedef struct BodyInspection {
    const char *name;
    const char *parent_name;
    bool has_parent;
    double distance_m;
    double speed_mps;
    double mass_kg;
    double radius_m;
} BodyInspection;

SimulationSession simulation_session_create(void);
void simulation_session_destroy(SimulationSession *session);
void simulation_session_reset(SimulationSession *session);
void simulation_session_update(SimulationSession *session, double real_seconds);
void simulation_session_single_step(SimulationSession *session);
void simulation_session_set_speed(SimulationSession *session, int preset);
void simulation_session_select_body(SimulationSession *session, int index);
double simulation_session_time_scale(const SimulationSession *session);
BodyInspection simulation_session_inspect(const SimulationSession *session);

#endif

#ifndef SOLAR_SIMULATION_SESSION_H
#define SOLAR_SIMULATION_SESSION_H

#include "simulation_step.h"
#include "../sim/experiment.h"
#include "../sim/lessons.h"
#include "../sim/diagnostics.h"

#define SOLAR_SPEED_PRESET_COUNT 5
#define SOLAR_APP_MAX_STEPS_PER_UPDATE 2048

typedef struct SimulationSession {
    SolarSystem system;
    SolarSystem initial_system;
    char imported_names[SOLAR_EXPERIMENT_CAPACITY][SOLAR_EXPERIMENT_NAME_BYTES];
    bool catalog_experiment;
    LessonPreset lesson;
    double velocity_factor;
    double initial_energy_j;
    double energy_scale_j;
    bool background;
    bool discard_resumed_frame;
    BodyTrails trails;
    SimulationClock clock;
    bool paused;
    int speed_preset;
    size_t selected_body_index;
    double achieved_time_scale;
    double rate_real_seconds;
    double rate_sim_seconds;
} SimulationSession;

typedef struct BodyInspection {
    const char *name;
    const char *parent_name;
    bool has_parent;
    double distance_m;
    double speed_mps;
    double mass_kg;
    double radius_m;
    double acceleration_mps2;
    double specific_energy_jpkg;
    Vec3d relative_position_m;
    Vec3d relative_velocity_mps;
    Vec3d relative_acceleration_mps2;
    PhysicalQuality mass_quality;
    PhysicalQuality radius_quality;
} BodyInspection;

SimulationSession simulation_session_create(void);
bool simulation_session_start_lesson(SimulationSession *session, LessonPreset lesson, double velocity_factor,
    PhysicsIntegrator integrator, double step_seconds);
bool lesson_configuration_valid(LessonPreset lesson, double velocity_factor, PhysicsIntegrator integrator,
    double step_seconds, CollisionMode collision_mode);
bool simulation_session_start_configured_lesson(SimulationSession *session, LessonPreset lesson, double velocity_factor,
    PhysicsIntegrator integrator, double step_seconds, CollisionMode collision_mode);
void simulation_session_advance_tick(SimulationSession *session, bool record_history);
void simulation_session_set_background(SimulationSession *session, bool hidden);
double simulation_session_energy_change(const SimulationSession *session, const PhysicsDiagnostics *diagnostics);
bool simulation_session_start_experiment(SimulationSession *session, const char *text);
void simulation_session_demo(SimulationSession *session);
void simulation_session_destroy(SimulationSession *session);
void simulation_session_reset(SimulationSession *session);
void simulation_session_update(SimulationSession *session, double real_seconds);
void simulation_session_single_step(SimulationSession *session);
void simulation_session_set_speed(SimulationSession *session, int preset);
void simulation_session_select_body(SimulationSession *session, int index);
int simulation_session_find_body(const SimulationSession *session, const char *query, size_t start);
double simulation_session_time_scale(const SimulationSession *session);
BodyInspection simulation_session_inspect(const SimulationSession *session);

#endif

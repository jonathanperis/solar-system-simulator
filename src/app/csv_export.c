#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "csv_export.h"
#include "revision.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

const char *solar_build_revision(void) { return SOLAR_BUILD_REVISION; }

FILE *simulation_csv_open_output(const char *path)
{
#ifdef _WIN32
    int descriptor = _open(path, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_TEXT, _S_IREAD | _S_IWRITE);
    if (descriptor < 0) return NULL;
    FILE *stream = _fdopen(descriptor, "w");
    if (!stream) _close(descriptor);
#else
    int descriptor = open(path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (descriptor >= 0) {
        /* Override umask only on the file this call created, never an existing destination. */
        if (fchmod(descriptor, S_IRUSR | S_IWUSR) != 0) {
            int error = errno;
            close(descriptor);
            errno = error;
            return NULL;
        }
    } else if (errno == EEXIST) {
        descriptor = open(path, O_WRONLY | O_TRUNC);
    }
    if (descriptor < 0) return NULL;
    FILE *stream = fdopen(descriptor, "w");
    if (!stream) close(descriptor);
#endif
    return stream;
}

static void csv_string(FILE *stream, const char *text)
{
    fputc('"', stream);
    for (; *text; ++text) {
        if (*text == '"') fputc('"', stream);
        fputc(*text, stream);
    }
    fputc('"', stream);
}

bool simulation_csv_begin(FILE *stream, const SimulationSession *session)
{
    fprintf(stream, "# solar-lab-v1\n# revision: %s\n# scene: %s\n# integrator: %s\n# dt_seconds: %.17g\n"
        "# velocity_factor: %.17g\n# frame: absolute simulation SI; X/Z reference plane\n"
        "# initial_energy_j: %.17g\n# energy_normalization_j: %.17g\n",
        solar_build_revision(), session->catalog_experiment ? "catalog" : lesson_name(session->lesson),
        session->clock.integrator == PHYSICS_EULER ? "euler" : "verlet",
        simulation_clock_step_seconds(&session->clock), session->velocity_factor,
        session->initial_energy_j, session->energy_scale_j);
    if (session->catalog_experiment) fprintf(stream, "# epoch_jd_tdb: %.1f\n", SOLAR_CATALOG_EPOCH_JD);
    fprintf(stream, "# collision_policy: %s\n", collision_mode_name(session->clock.collision_mode));
    fputs("time_s,tick,id,name,parent_id,mass_kg,radius_m,mass_quality,radius_quality,x_m,y_m,z_m,"
        "vx_mps,vy_mps,vz_mps,ax_mps2,ay_mps2,az_mps2,total_energy_j,normalized_energy_change,"
        "px_kg_mps,py_kg_mps,pz_kg_mps,lx_kg_m2ps,ly_kg_m2ps,lz_kg_m2ps,com_x_m,com_y_m,com_z_m\n", stream);
    return !ferror(stream);
}

bool simulation_csv_sample(FILE *stream, const SimulationSession *session)
{
    const char *quality[] = {"measured", "estimated", "unknown", "published"};
    PhysicsDiagnostics diagnostics = physics_diagnostics(&session->system);
    for (size_t i = 0; i < session->system.body_count; ++i) {
        const Body *body = &session->system.bodies[i];
        fprintf(stream, "%.17g,%llu,%d,", session->system.elapsed_seconds, (unsigned long long)session->clock.ticks, body->id);
        csv_string(stream, body->name);
        fprintf(stream, ",%d,", body->parent_id);
        /* Empty physical fields represent unknown data; the internal zero mass
         * of a tracer must not become a claimed measured zero in exported data. */
        if (body->mass_quality != PHYSICAL_UNKNOWN) fprintf(stream, "%.17g", body->mass_kg);
        fputc(',', stream);
        if (body->radius_quality != PHYSICAL_UNKNOWN) fprintf(stream, "%.17g", body->radius_m);
        fprintf(stream, ",%s,%s", quality[body->mass_quality], quality[body->radius_quality]);
        const double values[] = {body->position_m.x, body->position_m.y, body->position_m.z,
            body->velocity_mps.x, body->velocity_mps.y, body->velocity_mps.z,
            body->fixed ? 0 : body->acceleration_mps2.x, body->fixed ? 0 : body->acceleration_mps2.y,
            body->fixed ? 0 : body->acceleration_mps2.z, diagnostics.total_energy_j,
            simulation_session_energy_change(session, &diagnostics),
            diagnostics.momentum_kg_mps.x, diagnostics.momentum_kg_mps.y, diagnostics.momentum_kg_mps.z,
            diagnostics.angular_momentum_kg_m2ps.x, diagnostics.angular_momentum_kg_m2ps.y, diagnostics.angular_momentum_kg_m2ps.z,
            diagnostics.center_of_mass_m.x, diagnostics.center_of_mass_m.y, diagnostics.center_of_mass_m.z};
        for (size_t j = 0; j < sizeof(values) / sizeof(values[0]); ++j) fprintf(stream, ",%.17g", values[j]);
        fputc('\n', stream);
    }
    return !ferror(stream);
}

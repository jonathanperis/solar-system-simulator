#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "csv_export.h"
#include "revision.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

const char *solar_build_revision(void) { return SOLAR_BUILD_REVISION; }

/* Close a descriptor without letting close() overwrite the errno that
 * explains the original failure. */
static void close_preserving_errno(int descriptor)
{
    int error = errno;
#ifdef _WIN32
    _close(descriptor);
#else
    close(descriptor);
#endif
    errno = error;
}

FILE *simulation_csv_create_new(const char *path)
{
#ifdef _WIN32
    int descriptor = _open(path, _O_WRONLY | _O_CREAT | _O_EXCL | _O_TEXT, _S_IREAD | _S_IWRITE);
    if (descriptor < 0) return NULL;
    FILE *stream = _fdopen(descriptor, "w");
#else
    /* O_EXCL fails if anything at all already has this name, including a
     * symlink or FIFO, so this call can never write through or block on one. */
    int descriptor = open(path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (descriptor < 0) return NULL;
    /* The creation mode is filtered by umask; set 0600 explicitly instead. */
    if (fchmod(descriptor, S_IRUSR | S_IWUSR) != 0) {
        close_preserving_errno(descriptor);
        unlink(path);
        return NULL;
    }
    FILE *stream = fdopen(descriptor, "w");
#endif
    if (!stream) close_preserving_errno(descriptor);
    return stream;
}

static void release_output(CsvOutputFile *output)
{
    free(output->path);
    free(output->temp_path);
    *output = (CsvOutputFile){0};
}

bool simulation_csv_output_open(CsvOutputFile *output, const char *path)
{
    *output = (CsvOutputFile){0};
#ifdef _WIN32
    /* No atomic replace here: Windows writes the destination in place. */
    int descriptor = _open(path, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_TEXT, _S_IREAD | _S_IWRITE);
    if (descriptor < 0) return false;
    output->stream = _fdopen(descriptor, "w");
    if (!output->stream) close_preserving_errno(descriptor);
    return output->stream != NULL;
#else
    /* lstat describes the name itself rather than whatever a symlink points
     * at. Only a missing path or an existing regular file is an acceptable
     * destination; links, FIFOs, devices and directories are refused. */
    struct stat existing;
    mode_t mode = S_IRUSR | S_IWUSR;
    if (lstat(path, &existing) == 0) {
        if (!S_ISREG(existing.st_mode)) {
            errno = S_ISLNK(existing.st_mode) ? ELOOP : EINVAL;
            return false;
        }
        mode = existing.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    } else if (errno != ENOENT) {
        return false;
    }

    /* The temporary file lives beside the destination so the final rename()
     * stays within one filesystem, where POSIX makes it atomic. */
    static const char suffix[] = ".XXXXXX";
    size_t length = strlen(path);
    output->path = malloc(length + 1);
    output->temp_path = malloc(length + sizeof(suffix));
    if (!output->path || !output->temp_path) {
        release_output(output);
        errno = ENOMEM;
        return false;
    }
    memcpy(output->path, path, length + 1);
    memcpy(output->temp_path, path, length);
    memcpy(output->temp_path + length, suffix, sizeof(suffix));

    /* mkstemp creates a fresh, uniquely named 0600 file with O_EXCL, so it
     * never reuses or follows anything already present. A replaced file keeps
     * its permission bits; a new one stays owner-only even under umask 0. */
    int descriptor = mkstemp(output->temp_path);
    if (descriptor < 0) {
        release_output(output);
        return false;
    }
    if (fchmod(descriptor, mode) != 0 || !(output->stream = fdopen(descriptor, "w"))) {
        close_preserving_errno(descriptor);
        int error = errno;
        simulation_csv_output_abort(output);
        errno = error;
        return false;
    }
    return true;
#endif
}

bool simulation_csv_output_commit(CsvOutputFile *output)
{
    if (!output->stream) return false;
    bool ok = fflush(output->stream) == 0 && !ferror(output->stream);
#ifndef _WIN32
    /* Make the bytes durable before the name points at them; otherwise a crash
     * just after rename() could expose an empty file under the final name. */
    if (ok && fsync(fileno(output->stream)) != 0) ok = false;
#endif
    if (fclose(output->stream) != 0) ok = false;
    output->stream = NULL;
#ifndef _WIN32
    /* rename() replaces the destination's directory entry in one step and never
     * writes through it: readers see the old file or the complete new one. */
    if (ok && rename(output->temp_path, output->path) != 0) ok = false;
    if (!ok) {
        int error = errno;
        unlink(output->temp_path);
        errno = error;
    }
#endif
    release_output(output);
    return ok;
}

void simulation_csv_output_abort(CsvOutputFile *output)
{
    if (output->stream) fclose(output->stream);
#ifndef _WIN32
    /* Discard only our private temporary; the destination was never touched. */
    if (output->temp_path) unlink(output->temp_path);
#endif
    release_output(output);
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
        "# velocity_factor: %.17g\n# frame: absolute simulation SI; (x, y, z) = J2000 ecliptic (X, Z, -Y), +y north\n"
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

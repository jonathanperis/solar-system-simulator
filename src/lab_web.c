/* The comparison page uses this graphics-free C module. Its clock, references,
 * collisions and measurements are identical to the native headless runner. */
#include <emscripten/emscripten.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "app/comparison.h"
#include "app/csv_export.h"

static ComparisonRun lab;
static char normalized[SOLAR_LAB_CONFIG_BYTES];
static char *csv;

EMSCRIPTEN_KEEPALIVE const char *lab_revision(void) { return solar_build_revision(); }
EMSCRIPTEN_KEEPALIVE const char *lab_field_name(int field) { return comparison_field_name((LabField)field); }
EMSCRIPTEN_KEEPALIVE int lab_field_count(void) { return LAB_FIELD_COUNT; }

EMSCRIPTEN_KEEPALIVE const char *lab_normalize(const char *text)
{
    LabConfiguration config;
    return lab_configuration_parse(text, &config) && lab_configuration_format(&config, normalized, sizeof(normalized)) ? normalized : NULL;
}

EMSCRIPTEN_KEEPALIVE int lab_start(const char *text)
{
    LabConfiguration config;
    return lab_configuration_parse(text, &config) && comparison_start(&lab, &config);
}

EMSCRIPTEN_KEEPALIVE int lab_advance(int budget, int one_checkpoint)
{
    size_t remaining = budget < 0 ? 0 : budget > 8192 ? 8192 : (size_t)budget;
    uint64_t before = lab.sample_index;
    while (remaining && !lab.complete && !lab.failed && lab.configured) {
        size_t used = comparison_advance(&lab, remaining);
        remaining -= used;
        if (!used || (one_checkpoint && lab.sample_index != before)) break;
    }
    return lab.failed ? -1 : lab.complete ? 2 : 1;
}

EMSCRIPTEN_KEEPALIVE double lab_status(int field)
{
    switch (field) {
        case 0: return lab.configured;
        case 1: return lab.complete;
        case 2: return lab.failed;
        case 3: return (double)lab.sample_index;
        case 4: return (double)lab.total_samples;
        case 5: return lab.latest.time_seconds;
        case 6: return (double)lab.stride;
        case 7: return lab.maximum_phase_error[0];
        case 8: return lab.maximum_phase_error[1];
        default: return NAN;
    }
}

EMSCRIPTEN_KEEPALIVE int lab_point_count(void) { return (int)comparison_point_count(&lab); }

EMSCRIPTEN_KEEPALIVE double lab_point(int index, int side, int field)
{
    const ComparisonPoint *point = index < 0 ? NULL : comparison_point_at(&lab, (size_t)index);
    if (!point) return NAN;
    if (field == -1) return point->time_seconds;
    if (field == -2) return point->position_difference_m;
    return side >= 0 && side < 2 && field >= 0 && field < LAB_FIELD_COUNT ? point->run[side][field] : NAN;
}

EMSCRIPTEN_KEEPALIVE const char *lab_subject(void)
{
    return lab.configured ? lab.runs[0].initial_system.bodies[lesson_subject_index(lab.config.lesson)].name : "";
}

static size_t forces_for(int side, ForceContribution *out)
{
    if (!lab.configured || side < 0 || side > 1) return 0;
    const SolarSystem *system = &lab.observed[side];
    for (size_t i = 0; i < system->body_count; ++i)
        if (system->bodies[i].id == lab.subject_id) return physics_force_breakdown(system, i, out, SOLAR_SYSTEM_BODY_CAPACITY);
    return 0;
}

EMSCRIPTEN_KEEPALIVE int lab_force_count(int side)
{
    ForceContribution forces[SOLAR_SYSTEM_BODY_CAPACITY];
    return (int)forces_for(side, forces);
}

EMSCRIPTEN_KEEPALIVE const char *lab_force_name(int side, int index)
{
    ForceContribution forces[SOLAR_SYSTEM_BODY_CAPACITY];
    size_t count = forces_for(side, forces);
    return index >= 0 && (size_t)index < count ? lab.observed[side].bodies[forces[index].source_index].name : "";
}

EMSCRIPTEN_KEEPALIVE double lab_force(int side, int index, int field)
{
    ForceContribution forces[SOLAR_SYSTEM_BODY_CAPACITY];
    size_t count = forces_for(side, forces);
    if (index < 0 || (size_t)index >= count) return NAN;
    ForceContribution f = forces[index];
    const double values[] = {f.magnitude_mps2, f.magnitude_fraction, f.acceleration_mps2.x, f.acceleration_mps2.y, f.acceleration_mps2.z};
    return field >= 0 && field < 5 ? values[field] : NAN;
}

EMSCRIPTEN_KEEPALIVE const char *lab_export_csv(void)
{
    if (!lab.configured) return NULL;
    free(csv); csv = NULL;
    FILE *stream = tmpfile();
    if (!stream) return NULL;
    bool ok = comparison_csv_begin(stream, &lab, true);
    for (size_t i = 0; ok && i < comparison_point_count(&lab); ++i) ok = comparison_csv_sample(stream, comparison_point_at(&lab, i));
    if (ok && fflush(stream) == 0 && fseek(stream, 0, SEEK_END) == 0) {
        long length = ftell(stream);
        if (length > 0) csv = malloc((size_t)length + 1);
        ok = csv && fseek(stream, 0, SEEK_SET) == 0 && fread(csv, 1, (size_t)length, stream) == (size_t)length;
        if (ok) csv[length] = 0;
    } else ok = false;
    if (fclose(stream)) ok = false;
    if (!ok) { free(csv); csv = NULL; }
    return csv;
}

#ifndef SOLAR_EXPERIMENT_H
#define SOLAR_EXPERIMENT_H
#include "solar_system.h"
#define SOLAR_CATALOG_EPOCH_JD 2461200.5
#define SOLAR_EXPERIMENT_CAPACITY 16
#define SOLAR_EXPERIMENT_NAME_BYTES 96
#define SOLAR_EXPERIMENT_TEXT_BYTES 16384
/* Input is a bounded, versioned TSV document produced by the catalog explorer
 * or tools/export_experiment.py. This reader is shared by native and WASM. */
bool experiment_parse(const char *text, SolarSystem *out,
    char names[SOLAR_EXPERIMENT_CAPACITY][SOLAR_EXPERIMENT_NAME_BYTES]);
#endif

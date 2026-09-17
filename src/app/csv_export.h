#ifndef SOLAR_CSV_EXPORT_H
#define SOLAR_CSV_EXPORT_H

#include <stdio.h>
#include "simulation_session.h"

const char *solar_build_revision(void);
bool simulation_csv_begin(FILE *stream, const SimulationSession *session);
bool simulation_csv_sample(FILE *stream, const SimulationSession *session);

#endif

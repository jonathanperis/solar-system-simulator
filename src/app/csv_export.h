#ifndef SOLAR_CSV_EXPORT_H
#define SOLAR_CSV_EXPORT_H

#include <stdio.h>
#include "simulation_session.h"

const char *solar_build_revision(void);
/* A native CSV destination that is replaced only after the whole series has
 * been written. Writes go to a private temporary file beside `path`; commit
 * renames it into place, abort deletes it. A failed run therefore leaves either
 * the previous file or nothing, never a partial CSV.
 *
 * POSIX permission contract: a new file is owner read/write (0600) even with
 * umask 0; replacing an existing regular file keeps its permission bits (the
 * result is a new file, so ownership and hard links are not carried over).
 * Symlinks, FIFOs, devices and directories are refused at open, so a link
 * target is never truncated and a FIFO never blocks the run. Windows builds
 * write the destination in place. */
typedef struct CsvOutputFile {
    FILE *stream;
    char *path;
    char *temp_path;
} CsvOutputFile;

bool simulation_csv_output_open(CsvOutputFile *output, const char *path);
/* Flush, sync and rename into place; returns false (and removes the temporary)
 * if any write failed. Always releases the output. */
bool simulation_csv_output_commit(CsvOutputFile *output);
void simulation_csv_output_abort(CsvOutputFile *output);
/* Exclusive creation for snapshots: fails with EEXIST rather than replacing
 * anything already at `path`. New files are 0600 on POSIX, even with umask 0. */
FILE *simulation_csv_create_new(const char *path);
bool simulation_csv_begin(FILE *stream, const SimulationSession *session);
bool simulation_csv_sample(FILE *stream, const SimulationSession *session);

#endif

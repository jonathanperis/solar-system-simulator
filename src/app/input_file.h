#ifndef SOLAR_APP_INPUT_FILE_H
#define SOLAR_APP_INPUT_FILE_H

#include <stdbool.h>
#include <stddef.h>

/* Read a whole small text file (experiment table, comparison descriptor) into
 * text[size] and always NUL-terminate it. Returns false when the file cannot be
 * opened or read, holds more than size - 1 bytes, or contains an embedded NUL
 * byte. Both rejections matter: a silently truncated or NUL-cut text could
 * still parse, describing a different experiment than the file on disk. */
bool solar_read_text_file(const char *path, char *text, size_t size);

#endif

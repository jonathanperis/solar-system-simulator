#include "input_file.h"

#include <stdio.h>
#include <string.h>

bool solar_read_text_file(const char *path, char *text, size_t size)
{
    if (size == 0) return false;
    text[0] = '\0';
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    size_t bytes = fread(text, 1, size - 1, file);
    /* Reading one more byte distinguishes "exactly filled the buffer" from
     * "the file continues": only EOF here proves the whole file was read. */
    bool complete = !ferror(file) && fgetc(file) == EOF && !ferror(file);
    fclose(file);
    text[bytes] = '\0';
    return complete && !memchr(text, '\0', bytes);
}

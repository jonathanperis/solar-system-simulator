#include "body_labels.h"

#include <stdio.h>

void solar_app_body_list_label(const SolarSystem *system, char *buffer, size_t buffer_size)
{
    if (buffer_size == 0) {
        return;
    }

    if (system->body_count == 0) {
        snprintf(buffer, buffer_size, "none");
        return;
    }

    size_t written = 0;
    buffer[0] = '\0';
    for (size_t i = 0; i < system->body_count; ++i) {
        const char *separator = i == 0 ? "" : " + ";
        const char *name = system->bodies[i].name != NULL ? system->bodies[i].name : "Unnamed";
        int result = snprintf(buffer + written, buffer_size - written, "%s%s", separator, name);
        if (result < 0) {
            buffer[written] = '\0';
            return;
        }

        size_t appended = (size_t)result;
        if (appended >= buffer_size - written) {
            buffer[buffer_size - 1] = '\0';
            return;
        }

        written += appended;
    }
}

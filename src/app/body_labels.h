#ifndef SOLAR_APP_BODY_LABELS_H
#define SOLAR_APP_BODY_LABELS_H

#include <stddef.h>

#include "../sim/solar_system.h"

void solar_app_body_list_label(const SolarSystem *system, char *buffer, size_t buffer_size);

#endif

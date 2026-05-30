#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app/body_labels.h"
#include "sim/solar_system.h"

static void test_body_list_label_is_generated_from_system_body_names(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    char label[128];

    solar_app_body_list_label(&system, label, sizeof(label));

    assert(strcmp(label, "Sun + Mercury + Venus + Earth + Moon + Mars + Phobos + Deimos") == 0);
}

static void test_body_list_label_truncates_safely(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    char label[12];

    solar_app_body_list_label(&system, label, sizeof(label));

    assert(label[sizeof(label) - 1] == '\0');
    assert(strncmp(label, "Sun + Mercu", sizeof(label) - 1) == 0);
}

static void test_body_list_label_handles_empty_system(void)
{
    SolarSystem system = {0};
    char label[16];

    solar_app_body_list_label(&system, label, sizeof(label));

    assert(strcmp(label, "none") == 0);
}

int main(void)
{
    test_body_list_label_is_generated_from_system_body_names();
    test_body_list_label_truncates_safely();
    test_body_list_label_handles_empty_system();
    puts("test_body_labels passed");
    return 0;
}

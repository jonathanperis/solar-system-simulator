#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "sim/constants.h"
#include "sim/units.h"
#include "sim/vec3d.h"

static void assert_close(double actual, double expected, double epsilon)
{
    assert(fabs(actual - expected) <= epsilon);
}

static void test_addition_and_subtraction_preserve_components(void)
{
    Vec3d a = {1.0, -2.0, 3.5};
    Vec3d b = {4.0, 5.0, -6.5};

    Vec3d sum = vec3d_add(a, b);
    assert_close(sum.x, 5.0, 1e-12);
    assert_close(sum.y, 3.0, 1e-12);
    assert_close(sum.z, -3.0, 1e-12);

    Vec3d difference = vec3d_sub(sum, b);
    assert_close(difference.x, a.x, 1e-12);
    assert_close(difference.y, a.y, 1e-12);
    assert_close(difference.z, a.z, 1e-12);
}

static void test_scaling_supports_positive_and_negative_scalars(void)
{
    Vec3d v = {2.0, -3.0, 4.0};
    Vec3d positive = vec3d_scale(v, 2.5);
    Vec3d negative = vec3d_scale(v, -1.5);

    assert_close(positive.x, 5.0, 1e-12);
    assert_close(positive.y, -7.5, 1e-12);
    assert_close(positive.z, 10.0, 1e-12);
    assert_close(negative.x, -3.0, 1e-12);
    assert_close(negative.y, 4.5, 1e-12);
    assert_close(negative.z, -6.0, 1e-12);
}

static void test_dot_product_of_orthogonal_vectors_is_zero(void)
{
    Vec3d x_axis = {1.0, 0.0, 0.0};
    Vec3d y_axis = {0.0, 1.0, 0.0};

    assert_close(vec3d_dot(x_axis, y_axis), 0.0, 1e-12);
}

static void test_length_uses_all_components(void)
{
    Vec3d v = {3.0, 4.0, 12.0};

    assert_close(vec3d_length_squared(v), 169.0, 1e-12);
    assert_close(vec3d_length(v), 13.0, 1e-12);
}

static void test_unit_conversions_keep_render_scale_isolated(void)
{
    assert_close(meters_to_render_units(SOLAR_AU_METERS), 10.0, 1e-6);
    assert_close(seconds_to_days(SOLAR_DAY_SECONDS), 1.0, 1e-12);

    Vec3d meters = {SOLAR_AU_METERS, 0.5 * SOLAR_AU_METERS, -SOLAR_AU_METERS};
    Vec3d render = meters_vec_to_render_vec3d(meters);
    assert_close(render.x, 10.0, 1e-6);
    assert_close(render.y, 5.0, 1e-6);
    assert_close(render.z, -10.0, 1e-6);
}

int main(void)
{
    test_addition_and_subtraction_preserve_components();
    test_scaling_supports_positive_and_negative_scalars();
    test_dot_product_of_orthogonal_vectors_is_zero();
    test_length_uses_all_components();
    test_unit_conversions_keep_render_scale_isolated();
    puts("test_vec3d passed");
    return 0;
}

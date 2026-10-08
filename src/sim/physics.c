#include "physics.h"

#include <math.h>

#include "constants.h"

Vec3d gravitational_acceleration_from(const Body *target, const Body *source)
{
    if (target == source) {
        return vec3d_zero();
    }

    Vec3d displacement = vec3d_sub(source->position_m, target->position_m);
    double distance_squared = vec3d_length_squared(displacement);
    if (distance_squared == 0.0) {
        return vec3d_zero();
    }

    double distance = sqrt(distance_squared);
    /* Newtonian point-mass gravity: a = G*M*r_hat/r^2. Multiplying the
     * displacement vector by 1/r^3 preserves the direction toward the source
     * while producing acceleration in SI units (m/s^2). */
    double scale = SOLAR_G * source->mass_kg / (distance_squared * distance);
    return vec3d_scale(displacement, scale);
}

/* J2 oblateness (SPEC A94). An oblate primary's potential adds
 *   a = -(3/2) J2 GM R^2 / r^5 [(1 - 5 z^2/r^2) r + 2 z k]
 * to a body at offset r from it, where k is the unit spin axis, z = r.k and R
 * the reference radius. Only the primary's own moons feel it (bodies outside
 * a family see the planet as a point mass, where the term is negligible), and
 * the primary takes the equal and opposite force, so momentum is conserved. */
Vec3d physics_oblateness_acceleration(const Body *primary, const Body *moon)
{
    if (!(primary->j2 > 0.0) || moon->parent_id != primary->id || moon == primary) return vec3d_zero();
    double x = moon->position_m.x - primary->position_m.x;
    double y = moon->position_m.y - primary->position_m.y;
    double z_axis = moon->position_m.z - primary->position_m.z;
    double r2 = x * x + y * y + z_axis * z_axis;
    if (r2 == 0.0) return vec3d_zero();
    const Vec3d pole = primary->pole;
    const double radius = primary->j2_radius_m;
    const double k = 1.5 * primary->j2 * SOLAR_G * primary->mass_kg * radius * radius;
    double z = x * pole.x + y * pole.y + z_axis * pole.z;
    double scale = -k / (r2 * r2 * sqrt(r2));
    double radial = scale * (1.0 - 5.0 * z * z / r2), axial = scale * 2.0 * z;
    return (Vec3d){x * radial + pole.x * axial, y * radial + pole.y * axial, z_axis * radial + pole.z * axial};
}

double physics_oblateness_potential_j(const Body *primary, const Body *moon)
{
    if (!(primary->j2 > 0.0) || moon->parent_id != primary->id || moon == primary) return 0.0;
    Vec3d r = vec3d_sub(moon->position_m, primary->position_m);
    double distance = vec3d_length(r);
    if (distance == 0.0) return 0.0;
    /* U = G M m J2 R^2 P2(sin latitude) / r^3, with P2(s) = (3 s^2 - 1) / 2. */
    double s = vec3d_dot(r, primary->pole) / distance;
    return SOLAR_G * primary->mass_kg * moon->mass_kg * primary->j2 * primary->j2_radius_m * primary->j2_radius_m
        * 0.5 * (3.0 * s * s - 1.0) / (distance * distance * distance);
}

static void add_oblateness(Body *bodies, size_t body_count)
{
    for (size_t p = 0; p < body_count; ++p) {
        if (!(bodies[p].j2 > 0.0)) continue;
        for (size_t i = 0; i < body_count; ++i) {
            if (i == p || bodies[i].parent_id != bodies[p].id) continue;
            Vec3d a = physics_oblateness_acceleration(&bodies[p], &bodies[i]);
            bodies[i].acceleration_mps2 = vec3d_add(bodies[i].acceleration_mps2, a);
            /* Newton's third law: the planet is pulled back by m_moon/M_planet of it. */
            double share = bodies[i].mass_kg / bodies[p].mass_kg;
            bodies[p].acceleration_mps2 = vec3d_sub(bodies[p].acceleration_mps2, vec3d_scale(a, share));
        }
    }
}

/* Targets are processed in blocks through structure-of-arrays scratch so the
 * inner loop reads and writes contiguous doubles: compilers vectorize it
 * (SSE2/NEON natively, f64x2 with -msimd128 in WebAssembly) without changing
 * any result, because every lane still performs the same IEEE operations in
 * the same source order. The scratch is static: the simulator is
 * single-threaded. */
#define PHYSICS_TARGET_BLOCK 512
static double block_x[PHYSICS_TARGET_BLOCK], block_y[PHYSICS_TARGET_BLOCK], block_z[PHYSICS_TARGET_BLOCK];
static double block_ax[PHYSICS_TARGET_BLOCK], block_ay[PHYSICS_TARGET_BLOCK], block_az[PHYSICS_TARGET_BLOCK];

void physics_compute_accelerations(Body *bodies, size_t body_count)
{
    for (size_t start = 0; start < body_count; start += PHYSICS_TARGET_BLOCK) {
        const size_t count = body_count - start < PHYSICS_TARGET_BLOCK ? body_count - start : PHYSICS_TARGET_BLOCK;
        for (size_t i = 0; i < count; ++i) {
            block_x[i] = bodies[start + i].position_m.x;
            block_y[i] = bodies[start + i].position_m.y;
            block_z[i] = bodies[start + i].position_m.z;
            block_ax[i] = block_ay[i] = block_az[i] = 0.0;
        }
        /* Most satellites are explicitly massless tracers. Iterate sources
         * first to skip their entire zero-contribution columns, while
         * preserving the original source summation order for every target. */
        for (size_t j = 0; j < body_count; ++j) {
            if (bodies[j].mass_kg == 0.0) continue;
            const double sx = bodies[j].position_m.x, sy = bodies[j].position_m.y, sz = bodies[j].position_m.z;
            const double gm = SOLAR_G * bodies[j].mass_kg;
            for (size_t i = 0; i < count; ++i) {
                double x = sx - block_x[i];
                double y = sy - block_y[i];
                double z = sz - block_z[i];
                double r2 = x * x + y * y + z * z;
                /* a = G*M*r_vec/r^3. The source itself (and any coincident
                 * body) has r2 == 0 and contributes nothing; a select rather
                 * than a branch keeps the loop vectorizable. */
                double scale = r2 > 0.0 ? gm / (r2 * sqrt(r2)) : 0.0;
                block_ax[i] += x * scale;
                block_ay[i] += y * scale;
                block_az[i] += z * scale;
            }
        }
        for (size_t i = 0; i < count; ++i)
            bodies[start + i].acceleration_mps2 = (Vec3d){block_ax[i], block_ay[i], block_az[i]};
    }
    add_oblateness(bodies, body_count);
}

void physics_step(Body *bodies, size_t body_count, double dt_seconds)
{
    physics_compute_accelerations(bodies, body_count);
    physics_step_from_accelerations(bodies, body_count, dt_seconds);
}

void physics_step_with_integrator(Body *bodies, size_t body_count, double dt_seconds, PhysicsIntegrator method)
{
    physics_compute_accelerations(bodies, body_count);
    if (method == PHYSICS_EULER) physics_step_euler_from_accelerations(bodies, body_count, dt_seconds);
    else physics_step_from_accelerations(bodies, body_count, dt_seconds);
}

void physics_step_euler_from_accelerations(Body *bodies, size_t body_count, double dt_seconds)
{
    /* Explicit Euler is an intentionally less accurate classroom comparison:
     * drift with the old velocity, then kick with the old acceleration. */
    for (size_t i = 0; i < body_count; ++i) {
        if (bodies[i].fixed) continue;
        bodies[i].position_m = vec3d_add(bodies[i].position_m, vec3d_scale(bodies[i].velocity_mps, dt_seconds));
        bodies[i].velocity_mps = vec3d_add(bodies[i].velocity_mps, vec3d_scale(bodies[i].acceleration_mps2, dt_seconds));
    }
    physics_compute_accelerations(bodies, body_count);
}

void physics_step_from_accelerations(Body *bodies, size_t body_count, double dt_seconds)
{
    /* Velocity-Verlet: half-kick velocities, drift positions, recompute
     * acceleration from the new positions, then finish the second half-kick.
     * This is more orbit-friendly than explicit Euler for the same simple API.
     * Component arithmetic matches vec3d_add/vec3d_scale operation for
     * operation (same results) without cross-file calls in this hot loop. */
    const double half = 0.5 * dt_seconds;
    for (size_t i = 0; i < body_count; ++i) {
        Body *b = &bodies[i];
        if (b->fixed) continue;
        b->velocity_mps.x += b->acceleration_mps2.x * half;
        b->velocity_mps.y += b->acceleration_mps2.y * half;
        b->velocity_mps.z += b->acceleration_mps2.z * half;
        b->position_m.x += b->velocity_mps.x * dt_seconds;
        b->position_m.y += b->velocity_mps.y * dt_seconds;
        b->position_m.z += b->velocity_mps.z * dt_seconds;
    }

    physics_compute_accelerations(bodies, body_count);

    for (size_t i = 0; i < body_count; ++i) {
        Body *b = &bodies[i];
        if (b->fixed) continue;
        b->velocity_mps.x += b->acceleration_mps2.x * half;
        b->velocity_mps.y += b->acceleration_mps2.y * half;
        b->velocity_mps.z += b->acceleration_mps2.z * half;
    }
}

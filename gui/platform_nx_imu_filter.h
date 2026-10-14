#ifndef VANILLA_PI_PLATFORM_NX_IMU_FILTER_H
#define VANILLA_PI_PLATFORM_NX_IMU_FILTER_H

#include <math.h>

/**
 * Gyroscope smoothing for the Nintendo Switch's IMU.
 *
 * Two stages, both tunable with the constants below:
 *
 *  1. A soft dead zone removes the small constant "shimmer" the sensor reports
 *     while the console is sitting still. It is soft (values just above the
 *     threshold are reduced rather than passed straight through) so there is
 *     no sudden jump as you start to move.
 *
 *  2. A speed-adaptive low-pass filter. Slow movement is heavily smoothed,
 *     which is where jitter is most visible, while fast movement passes
 *     through almost unfiltered so quick turns don't feel laggy.
 *
 * Units are radians per second and seconds.
 */

#define NX_GYRO_DEADZONE 0.015f       // ~0.9 degrees/second
#define NX_GYRO_REST_TAU 0.060f       // Smoothing time constant when (nearly) still
#define NX_GYRO_PASSTHRU_SPEED 1.2f   // At/above this speed, no smoothing (~69 deg/s)
#define NX_GYRO_MIN_DT 0.0005f
#define NX_GYRO_MAX_DT 0.050f

typedef struct {
    float prev[3];
    int primed;
} nx_gyro_filter_t;

static inline void nx_gyro_filter_reset(nx_gyro_filter_t *f)
{
    f->prev[0] = f->prev[1] = f->prev[2] = 0.0f;
    f->primed = 0;
}

static inline float nx_gyro_soft_deadzone(float v)
{
    float mag = fabsf(v) - NX_GYRO_DEADZONE;
    if (mag <= 0.0f) {
        return 0.0f;
    }
    return v < 0.0f ? -mag : mag;
}

/**
 * Filters one gyro sample in place. `dt` is the time since the previous
 * sample, in seconds.
 */
static inline void nx_gyro_filter_apply(nx_gyro_filter_t *f, float g[3], float dt)
{
    if (!(dt > NX_GYRO_MIN_DT)) dt = NX_GYRO_MIN_DT; // Also catches NaN
    if (dt > NX_GYRO_MAX_DT) dt = NX_GYRO_MAX_DT;

    for (int i = 0; i < 3; i++) {
        g[i] = nx_gyro_soft_deadzone(g[i]);
    }

    if (!f->primed) {
        for (int i = 0; i < 3; i++) f->prev[i] = g[i];
        f->primed = 1;
        return;
    }

    float speed = sqrtf(g[0] * g[0] + g[1] * g[1] + g[2] * g[2]);
    float fast = speed / NX_GYRO_PASSTHRU_SPEED;
    if (fast > 1.0f) fast = 1.0f;

    float tau = NX_GYRO_REST_TAU * (1.0f - fast);
    float alpha = dt / (tau + dt);

    for (int i = 0; i < 3; i++) {
        f->prev[i] += alpha * (g[i] - f->prev[i]);
        g[i] = f->prev[i];
    }
}

#endif // VANILLA_PI_PLATFORM_NX_IMU_FILTER_H

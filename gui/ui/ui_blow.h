#ifndef VANILLA_PI_UI_BLOW_H
#define VANILLA_PI_UI_BLOW_H

#include <math.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Synthetic "blow into the microphone" signal.
 *
 * Some devices (notably the Nintendo Switch) have no built-in microphone, but
 * some Wii U games ask you to blow into the GamePad's mic. While a button is
 * held we mix broadband noise into the microphone stream, which looks like
 * breath noise to the console. The level is smoothed with a short attack and
 * release so that it doesn't click when the button is pressed or released.
 */

#define VUI_BLOW_PEAK_AMPLITUDE 20000.0f // Out of 32767
#define VUI_BLOW_ATTACK_SECONDS 0.012f
#define VUI_BLOW_RELEASE_SECONDS 0.030f

typedef struct {
    uint32_t rng;
    float gain; // Current envelope, 0.0 (silent) to 1.0 (full)
} vui_blow_t;

static inline void vui_blow_init(vui_blow_t *b)
{
    b->rng = 0x2545F491u;
    b->gain = 0.0f;
}

static inline int16_t vui_blow_clip(int32_t v)
{
    if (v > INT16_MAX) return INT16_MAX;
    if (v < INT16_MIN) return INT16_MIN;
    return (int16_t) v;
}

/**
 * Mixes the blow signal into `count` mono signed 16-bit samples in place.
 *
 * `pressed` is whether the blow button is currently held. When the button is
 * not held and the envelope has fully decayed, the samples are left untouched.
 */
static inline void vui_blow_mix(vui_blow_t *b, int16_t *samples, size_t count, int pressed, int sample_rate)
{
    if (!pressed && b->gain <= 0.0f) {
        return;
    }

    const float target = pressed ? 1.0f : 0.0f;
    const float seconds = pressed ? VUI_BLOW_ATTACK_SECONDS : VUI_BLOW_RELEASE_SECONDS;
    const float coef = 1.0f - expf(-1.0f / (seconds * (float) sample_rate));

    uint32_t rng = b->rng;
    float gain = b->gain;

    for (size_t i = 0; i < count; i++) {
        gain += (target - gain) * coef;

        // xorshift32 white noise, scaled to -1.0 .. 1.0
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        float noise = ((float) (rng >> 16) - 32768.0f) / 32768.0f;

        samples[i] = vui_blow_clip((int32_t) samples[i] + (int32_t) (noise * gain * VUI_BLOW_PEAK_AMPLITUDE));
    }

    // Snap to silence once the release has effectively finished
    if (!pressed && gain < 0.001f) {
        gain = 0.0f;
    }

    b->rng = rng;
    b->gain = gain;
}

#endif // VANILLA_PI_UI_BLOW_H

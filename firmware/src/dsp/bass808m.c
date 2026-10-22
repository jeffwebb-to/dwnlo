/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Jeff Webb */
/* See bass808m.h. */
#include "bass808m.h"
#include "fastmath.h"

#define M808_IDLE_HOLD 4410             /* 100 ms of near-silence before render idles (bass303's pattern) */
#define M808_CLICK_C 0.9955f            /* ~40 ms decay of the attack pitch "punch" */
#define M808_CLICK_DEPTH 0.6f            /* punch bends the pitch up by up to this many octaves */

static const x0x_param_t params[M808_NPARAMS] = {
    { "Tune", 127, 64, 0 },      /* A4 = 440 + (pot - 64) * 0.625 Hz, same range as bass303 */
    { "Decay", 127, 72, 0 },     /* 60 .. 3000 ms, exponential */
    { "Drive", 127, 40, 0 },     /* 0 = clean */
    { "Glide", 127, 20, 0 },     /* 2 .. 500 ms portamento */
};

int m808_nparams(void) { return M808_NPARAMS; }
const x0x_param_t *m808_param(int i) { return (i >= 0 && i < M808_NPARAMS) ? &params[i] : 0; }
int m808_get(const m808_t *b, int i) { return (i >= 0 && i < M808_NPARAMS) ? b->pot[i] : 0; }

static float rc_coeff(float tau_ms, float fs)
{
    return tau_ms > 0.0f ? fm_expf(-1000.0f / (tau_ms * fs)) : 0.0f;
}

void m808_set(m808_t *b, int i, int v)
{
    float n;
    if (i < 0 || i >= M808_NPARAMS)
        return;
    if (v < 0) v = 0;
    if (v > params[i].max) v = params[i].max;
    b->pot[i] = (uint8_t)v;
    n = (float)v * (1.0f / 127.0f);
    switch (i) {
    case M808_TUNE:
        b->tuning = 440.0f + (float)(v - 64) * 0.625f;
        break;
    case M808_DECAY:
        b->decay_c = rc_coeff(60.0f + 2940.0f * n, M808_SR);
        break;
    case M808_DRIVE:
        b->drive_amt = n;
        break;
    case M808_GLIDE:
        b->glide_c = rc_coeff(2.0f + 498.0f * n, M808_SR);
        break;
    }
}

void m808_init(m808_t *b)
{
    uint8_t *p = (uint8_t *)b;
    unsigned k;
    int i;
    for (k = 0; k < sizeof(*b); k++)
        p[k] = 0;
    for (i = 0; i < M808_NPARAMS; i++)
        m808_set(b, i, params[i].def);
    b->target_hz = b->freq = b->tuning;
    b->idle = 1;
}

void m808_note_on(m808_t *b, int note, int accent, int slide)
{
    float f;
    (void)accent;                        /* no separate accent shaping yet: kept simple, honest */
    if (note < 0) note = 0;
    if (note > 127) note = 127;
    f = b->tuning * fm_exp2f((float)(note - 69) * (1.0f / 12.0f));
    b->target_hz = f;
    if (!(slide && b->gate)) {           /* retrigger: snap pitch, restart envelope and the click */
        b->freq = f;
        b->env_y = 1.0f;
        b->click = 1.0f;
    }
    b->gate = 1;
    b->idle = 0;
    b->quiet = 0;
    b->started = 1;
}

void m808_note_off(m808_t *b) { b->gate = 0; }

void m808_render(m808_t *b, float *out, int n)
{
    int i;
    float phase, freq, target, env_y, click, tone_z, glide_c, env_c, drive;
    if (b->idle) {
        for (i = 0; i < n; i++)
            out[i] = 0.0f;
        return;
    }
    phase = b->phase; freq = b->freq; target = b->target_hz; env_y = b->env_y;
    click = b->click; tone_z = b->tone_z; glide_c = b->glide_c; env_c = b->decay_c;
    drive = b->drive_amt;
    for (i = 0; i < n; i++) {
        float played, s, g, y;
        freq += (target - freq) * glide_c;
        played = freq * fm_exp2f(click * M808_CLICK_DEPTH);
        phase += played * (1.0f / M808_SR);
        if (phase >= 1.0f) phase -= 1.0f;
        s = fm_sinf(FM_TWO_PI * phase);
        click *= M808_CLICK_C;
        env_y *= env_c;
        g = 1.0f + 3.0f * drive;                           /* drive: pre-gain into tanh, make-up after */
        y = fm_tanhf(s * env_y * g) * (1.0f / (g > 1.0f ? fm_tanhf(g) : 1.0f));
        tone_z += (y - tone_z) * 0.35f;                     /* a little warmth, one-pole */
        out[i] = tone_z;
    }
    b->phase = phase; b->freq = fm_flush(freq); b->env_y = fm_flush(env_y);
    b->click = fm_flush(click); b->tone_z = fm_flush(tone_z);

    if (!b->gate && b->env_y < 1e-4f) {
        b->quiet += n;
        if (b->quiet >= M808_IDLE_HOLD) {
            b->tone_z = 0.0f;
            b->idle = 1;
        }
    } else {
        b->quiet = 0;
    }
}

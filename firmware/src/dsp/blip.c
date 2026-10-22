/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Jeff Webb */
/* See blip.h. */
#include "blip.h"
#include "fastmath.h"

#define BLIP_FM_RATIO 1.41f               /* modulator : carrier, close to root 2 - an INHARMONIC ratio
                                           * on purpose: 2:1 (a plain octave) was tried first and heard
                                           * on real hardware as tonal/pitched, like a tom rather than a
                                           * blip - a non-integer ratio spreads the attack's sidebands
                                           * unevenly, which is what reads as a metallic "blip" edge
                                           * instead of just a brighter version of the same pitched note */
#define BLIP_IDLE_HOLD 4410               /* 100 ms of near-silence before render idles (m808's pattern) */
#define BLIP_MOD_FRAC 0.22f               /* the modulator's decay is this fraction of the carrier's -
                                           * always the faster of the two, which is the whole "blip" */
#define BLIP_MOD_FLOOR_MS 4.0f

static const x0x_param_t params[BLIP_NPARAMS] = {
    { "Tune", 127, 64, 0 },       /* A4 = 440 + (pot - 64) * 0.625 Hz, same range as bass303/m808 */
    { "Decay", 127, 18, 0 },      /* 15 .. 900 ms, exponential; 18 default = ~140 ms, a tight blip
                                   * rather than a tom-length ring */
    { "Bright", 127, 85, 0 },     /* modulator peak index, 0 .. ~8; raised for a hotter default burst */
    { "Glide", 127, 0, 0 },       /* 2 .. 500 ms portamento; 0 is effectively instant */
    { "LRate", 127, 50, 0 },      /* 0.05 .. 8 Hz, exponential */
    { "LDepth", 127, 20, 0 },     /* 0 .. 1.5 octaves of cutoff swing */
    { "Cutoff", 127, 100, 0 },    /* 300 .. 8000 Hz, exponential */
};

int blip_nparams(void) { return BLIP_NPARAMS; }
const x0x_param_t *blip_param(int i) { return (i >= 0 && i < BLIP_NPARAMS) ? &params[i] : 0; }
int blip_get(const blip_t *b, int i) { return (i >= 0 && i < BLIP_NPARAMS) ? b->pot[i] : 0; }

static float rc_coeff(float tau_ms, float fs)
{
    return tau_ms > 0.0f ? fm_expf(-1000.0f / (tau_ms * fs)) : 0.0f;
}

void blip_set(blip_t *b, int i, int v)
{
    float n;
    if (i < 0 || i >= BLIP_NPARAMS)
        return;
    if (v < 0) v = 0;
    if (v > params[i].max) v = params[i].max;
    b->pot[i] = (uint8_t)v;
    n = (float)v * (1.0f / 127.0f);
    switch (i) {
    case BLIP_TUNE:
        b->tuning = 440.0f + (float)(v - 64) * 0.625f;
        break;
    case BLIP_DECAY: {
        float tau = 15.0f + 885.0f * n;
        b->decay_c = rc_coeff(tau, BLIP_SR);
        b->mdecay_c = rc_coeff(fm_maxf(BLIP_MOD_FLOOR_MS, tau * BLIP_MOD_FRAC), BLIP_SR);
        break;
    }
    case BLIP_BRIGHT:
        b->fm_depth = n * 8.0f;
        break;
    case BLIP_GLIDE:
        b->glide_c = rc_coeff(2.0f + 498.0f * n, BLIP_SR);
        break;
    case BLIP_LRATE:
        b->lfo_rate = (0.05f * fm_exp2f(n * 7.3219f)) * (1.0f / BLIP_SR);   /* 0.05 .. 8 Hz, in turns/sample */
        break;
    case BLIP_LDEPTH:
        b->lfo_depth_oct = n * 1.5f;
        break;
    case BLIP_CUTOFF:
        b->base_cutoff = 300.0f * fm_exp2f(n * 4.7370f);                    /* 300 .. 8000 Hz */
        break;
    }
}

void blip_init(blip_t *b)
{
    uint8_t *p = (uint8_t *)b;
    unsigned k;
    int i;
    for (k = 0; k < sizeof(*b); k++)
        p[k] = 0;
    for (i = 0; i < BLIP_NPARAMS; i++)
        blip_set(b, i, params[i].def);
    b->target_hz = b->freq = b->tuning;
    b->idle = 1;
}

void blip_note_on(blip_t *b, int note, int accent, int slide)
{
    (void)accent;                        /* no separate accent shaping yet: kept simple, honest */
    (void)slide;                         /* spart_t carries no tie bit (section pattern.h), so unlike
                                           * m808's slide-gated legato, GLIDE here always slews pitch
                                           * toward the new note - a monosynth portamento, not a tie */
    if (note < 0) note = 0;
    if (note > 127) note = 127;
    b->target_hz = b->tuning * fm_exp2f((float)(note - 69) * (1.0f / 12.0f));
    if (!b->started)
        b->freq = b->target_hz;          /* the very first note: no glide in from silence */
    b->env_y = 1.0f;                     /* always hard-retrigger: instant attack, no rising stage */
    b->menv_y = 1.0f;
    b->gate = 1;
    b->idle = 0;
    b->quiet = 0;
    b->started = 1;
}

void blip_note_off(blip_t *b) { b->gate = 0; }

void blip_render(blip_t *b, float *out, int n)
{
    int i;
    float phase, phase_m, freq, target, env_y, menv_y, filt_z;
    float glide_c, decay_c, mdecay_c, fm_depth, lfo_a;
    if (b->idle) {
        for (i = 0; i < n; i++)
            out[i] = 0.0f;
        return;
    }

    /* the filter LFO and its one-pole coefficient are updated once per block, not per sample -
     * the LFO is slow by design (a "slight" wobble, BLIP_LDEPTH/BLIP_LRATE), so a block-rate
     * update is inaudibly different from a per-sample one and spares a transcendental call per
     * sample (the same reasoning as x0x_param.h's x0x_glide_block) */
    {
        float ph = b->lfo_phase + b->lfo_rate * (float)n;
        float lfo_val, cutoff_hz;
        ph -= fm_floorf(ph);
        b->lfo_phase = ph;
        lfo_val = fm_sin_turns(ph);
        cutoff_hz = fm_clampf(b->base_cutoff * fm_exp2f(lfo_val * b->lfo_depth_oct), 60.0f, 16000.0f);
        lfo_a = fm_expf(-FM_TWO_PI * cutoff_hz * (1.0f / BLIP_SR));
    }

    phase = b->phase; phase_m = b->phase_m; freq = b->freq; target = b->target_hz;
    env_y = b->env_y; menv_y = b->menv_y; filt_z = b->filt_z;
    glide_c = b->glide_c; decay_c = b->decay_c; mdecay_c = b->mdecay_c; fm_depth = b->fm_depth;
    for (i = 0; i < n; i++) {
        float fm_, mod, y;
        freq += (target - freq) * glide_c;
        fm_ = freq * BLIP_FM_RATIO;
        phase_m += fm_ * (1.0f / BLIP_SR);
        if (phase_m >= 1.0f) phase_m -= 1.0f;
        mod = fm_sinf(FM_TWO_PI * phase_m) * (fm_depth * menv_y);
        phase += freq * (1.0f / BLIP_SR);
        if (phase >= 1.0f) phase -= 1.0f;
        y = fm_sinf(FM_TWO_PI * phase + mod) * env_y;
        filt_z += (y - filt_z) * (1.0f - lfo_a);         /* one-pole lowpass, also the output smoothing */
        out[i] = filt_z;
        env_y *= decay_c;
        menv_y *= mdecay_c;
    }
    b->phase = phase; b->phase_m = phase_m; b->freq = fm_flush(freq);
    b->env_y = fm_flush(env_y); b->menv_y = fm_flush(menv_y); b->filt_z = fm_flush(filt_z);

    if (!b->gate && b->env_y < 1e-4f) {
        b->quiet += n;
        if (b->quiet >= BLIP_IDLE_HOLD) {
            b->filt_z = 0.0f;
            b->idle = 1;
        }
    } else {
        b->quiet = 0;
    }
}

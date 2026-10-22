/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Jeff Webb */
/* DWNLO blip: a short, bright, percussive synth pluck for fast melodic lines - the
 * "dit-dit-dit-dit" arpeggio character (Planet Rock's riff is the reference), not a
 * chord pad. Replaces BREAK's part slot (engine.c) and its pattern slot (pattern.h: the
 * old brkpart_t bytes are now bass[2], a third bpart_t - this engine is driven exactly
 * like bass303/m808: one note per step).
 *
 * Monophonic, always hard-retriggered: a new note instantly resets both envelopes below,
 * so fast repeated notes stay crisp instead of smearing together. One 2-operator FM pair
 * (carrier + modulator at a fixed ratio) makes the "blip": the modulator's own envelope
 * decays faster than the carrier's, so each note opens with a bright, slightly metallic
 * burst that collapses into a near-pure tone as it dies - the classic cheap FM bell/pluck
 * trick, no filter sweep needed for that part. A separate, genuinely optional one-pole
 * lowpass with a slow sine LFO on its cutoff (BLIP_LRATE/BLIP_LDEPTH) adds a slight wobble
 * on top, scoped to this one voice only - nothing global, nothing touching fxbus.c or
 * master.c. No sample buffers, no per-voice envelope banks: still far smaller than the
 * break player this slot originally held (6296 B). */
#pragma once
#include <stdint.h>
#include "x0x_param.h"

#define BLIP_SR 44100.0f

enum { BLIP_TUNE, BLIP_DECAY, BLIP_BRIGHT, BLIP_GLIDE,
       BLIP_LRATE, BLIP_LDEPTH, BLIP_CUTOFF, BLIP_NPARAMS };

typedef struct blip {
    uint8_t pot[BLIP_NPARAMS];

    /* derived from the pots */
    float tuning;                    /* A4, Hz (BLIP_TUNE) */
    float decay_c, mdecay_c;         /* carrier / modulator envelope coefficients (mdecay_c is
                                       * always the faster of the two - the "blip" burst) */
    float fm_depth;                  /* modulator's peak index, 0..~8 (BLIP_BRIGHT) */
    float glide_c;                   /* portamento coefficient (BLIP_GLIDE) */
    float lfo_rate;                  /* turns per sample (BLIP_LRATE) */
    float lfo_depth_oct;             /* cutoff LFO swing, +/- octaves (BLIP_LDEPTH) */
    float base_cutoff;               /* filter cutoff before the LFO, Hz (BLIP_CUTOFF) */

    /* voice */
    int gate, idle, started, quiet;
    float target_hz, freq;           /* portamento target and the slewed, currently-sounding pitch */
    float phase, phase_m;            /* carrier / modulator phase, turns, 0..1 */
    float env_y, menv_y;             /* carrier amplitude / modulator depth envelopes, 1 at trigger */
    float lfo_phase;                 /* filter LFO phase, turns, 0..1 - advanced once per render() block */
    float filt_z;                    /* one-pole lowpass state (also the output smoothing) */
} blip_t;

void blip_init(blip_t *b);
void blip_note_on(blip_t *b, int note, int accent, int slide);
void blip_note_off(blip_t *b);
void blip_render(blip_t *b, float *out, int n);
int blip_nparams(void);
const x0x_param_t *blip_param(int i);
void blip_set(blip_t *b, int i, int value);
int blip_get(const blip_t *b, int i);

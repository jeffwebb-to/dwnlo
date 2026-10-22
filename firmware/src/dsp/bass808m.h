/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Jeff Webb */
/* DWNLO melodic 808 bass: a tuned sub voice played as notes with portamento, the way
 * trap/electro producers play an 808 kick melodically. Not a port of drum808.c's d8_bd_t
 * (an analog bandpass-resonator circuit model built for one-shot hits at a near-fixed
 * pitch) - continuously bending that circuit's resonant frequency under glide risked
 * instability, so this is a from-scratch lean voice in the same spirit: oscillator,
 * decay envelope, a pitch "click" on attack (the classic 808 punch), drive, one-pole tone.
 * Float, per-sample (no control-rate chunking - a single voice doesn't need bass303's
 * interpolation machinery). Replaces 303B's slot (engine.c), not drum808.c's kit kick. */
#pragma once
#include <stdint.h>
#include "x0x_param.h"

#define M808_SR 44100.0f

enum { M808_TUNE, M808_DECAY, M808_DRIVE, M808_GLIDE, M808_NPARAMS };

typedef struct m808 {
    uint8_t pot[M808_NPARAMS];

    /* derived from the pots */
    float tuning;             /* A4, Hz (m808_set TUNE) */
    float decay_c;            /* envelope coefficient */
    float drive_amt;          /* 0..1 */
    float glide_c;            /* portamento coefficient */

    /* voice */
    int gate;                 /* a note is held (slide decides legato vs retrigger, as bass303) */
    int idle;                 /* nothing sounding: render writes zeros */
    int started;
    int quiet;                /* samples of near-silence so far (idle hold-off) */

    float target_hz, freq;    /* portamento target and the slewed, currently-sounding pitch */
    float phase;              /* oscillator phase, turns, 0..1 */
    float env_y;              /* decay envelope, 1 at trigger */
    float click;              /* the attack "punch": decays fast, bends pitch up briefly */
    float tone_z;             /* one-pole tone/drive output smoothing */
} m808_t;

void m808_init(m808_t *b);
void m808_note_on(m808_t *b, int note, int accent, int slide);
void m808_note_off(m808_t *b);
void m808_render(m808_t *b, float *out, int n);
int m808_nparams(void);
const x0x_param_t *m808_param(int i);
void m808_set(m808_t *b, int i, int value);
int m808_get(const m808_t *b, int i);

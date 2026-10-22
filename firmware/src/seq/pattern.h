/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X pattern model: 32 patterns, each holding the 909, the 808, two bass-like parts
 * (303 A and the melodic 808 bass), and blip (DWNLO: BREAK's old slot - its
 * own lean per-pattern data, NOT a third bpart_t; a full bpart_t per pattern (144 B)
 * for all three cost 112 B/pattern x 32 patterns = 3584 B more static RAM than this
 * device's hard 98304 B budget allows for. spart_t below is sized for what blip
 * actually needs - a root note and an accent per step, no slide/tie/direction/
 * transpose/TB-3PO machinery - at 34 B/pattern, close to BREAK's original 32 B).
 *
 * Plain data, no pointers, so a pattern copies with an assignment and the project
 * stores the bytes. The UI edits these in place from the main loop while the
 * sequencer reads them in the audio ISR: every field the sequencer reads is a
 * single byte or an aligned 32-bit word, so a read never sees half a write. A drum row is
 * two words (steps 1-32, 33-64): sm_get / sm_flip below. */
#pragma once
#include <stdint.h>

#define NPAT 32
#define NSTEPS 64
#define NSW (NSTEPS / 32)            /* 32-bit words in a row of steps */
#define NDRUM 11                     /* tracks per drum machine: the 11 black keys */
#define NKIT 2                       /* 0 = 909 (drum909.h order), 1 = 808 (drum808.h order) */
#define NBASS 2                      /* 303 A, melodic 808 bass (DWNLO: blip is spart_t, not a 3rd bpart_t) */

enum { RATE_16, RATE_16T, RATE_32, RATE_8T, NRATES };      /* step length */
enum { DIR_FWD, DIR_REV, DIR_PINGPONG, DIR_RANDOM, NDIRS };  /* 303 playback direction */
enum { G_REST, G_NOTE, G_TIE };                             /* 303 step gate */

/* 303 step: one byte of note, one of flags */
#define BS_GATE_MASK 0x03u
#define BS_ACCENT 0x04u
#define BS_SLIDE 0x08u                /* slide INTO the next step (303 semantics) */
typedef struct {
    uint8_t note;                     /* MIDI note */
    uint8_t flags;                    /* gate | BS_ACCENT | BS_SLIDE */
} bstep_t;

/* TB-3PO settings of one 303 line: the line is reproducible from these and the seed */
typedef struct {
    uint8_t density, accent, slide;   /* probabilities 0..100 % */
    uint8_t oct_range;                /* 1..3 */
    uint8_t root;                     /* 0 = C .. 11 = B */
    uint8_t scale;                    /* tb3po.h SCALES */
    uint8_t base_oct;                 /* octave of the root: note = 12 * (base_oct + 1) + root */
    uint8_t mutate_bars;              /* auto-mutate every N bars, 0 = off */
    uint32_t seed;
} tb3po_cfg_t;

typedef struct {
    bstep_t step[NSTEPS];
    uint8_t len;                      /* 1..64 */
    uint8_t rate;                     /* RATE_* */
    uint8_t dir;                      /* DIR_* */
    uint8_t transpose;                /* semitones + 24 (24 = none), applied when played */
    tb3po_cfg_t gen;
} bpart_t;

typedef struct {
    uint32_t hit[NDRUM][NSW];         /* step s (sm_get): the instrument plays */
    uint32_t accent[NSW];             /* the total-accent row */
    uint8_t len;                      /* 1..64 */
    uint8_t rate;
    uint8_t rsv[2];
} dpart_t;

/* blip step: one packed byte - bit 7 gate, bit 6 accent, bits 0-5 the note
 * (biased by SB_NOTE_BASE; 64 values is 5+ octaves, plenty for a melodic blip line) */
#define SB_GATE 0x80u
#define SB_ACCENT 0x40u
#define SB_NOTE_MASK 0x3Fu
#define SB_NOTE_BASE 36                   /* C2 */
#define BLIP_NSTEPS 32                    /* half of NSTEPS: no bass-line-length pattern needed */
typedef struct {
    uint8_t step[BLIP_NSTEPS];
    uint8_t len;                          /* 1..32 */
    uint8_t rate;                         /* RATE_* */
} spart_t;
static inline int sb_gate(uint8_t s) { return (s & SB_GATE) != 0; }
static inline int sb_note(uint8_t s) { return (s & SB_NOTE_MASK) + SB_NOTE_BASE; }

typedef struct {
    dpart_t drum[NKIT];
    bpart_t bass[NBASS];
    spart_t blip;                     /* DWNLO: BREAK's old slot */
    uint8_t swing;                    /* 0..100: 0 = straight (50 %), 100 = 75 % shuffle */
    uint8_t rsv[3];
} pattern_t;

static inline int bstep_gate(const bstep_t *s) { return s->flags & BS_GATE_MASK; }

/* a row of steps (NSW words): step s */
static inline int sm_get(const uint32_t *m, int s) { return (int)((m[s >> 5] >> (s & 31)) & 1u); }
static inline void sm_flip(uint32_t *m, int s) { m[s >> 5] ^= 1u << (s & 31); }
static inline void sm_set(uint32_t *m, int s) { m[s >> 5] |= 1u << (s & 31); }
static inline int sm_any(const uint32_t *m)
{
    uint32_t a = 0;
    int k;
    for (k = 0; k < NSW; k++)
        a |= m[k];
    return a != 0;
}

/* default empty pattern: 16 steps, straight, 303 lines empty with TB-3PO defaults */
void pattern_init(pattern_t *p, uint32_t seed_a, uint32_t seed_b);

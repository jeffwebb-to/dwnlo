/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Jeff Webb */
/* DWNLO's boot splash: a small outside-the-club scene. The night, the street, the club
 * building, the sidewalk crowd and the streetlamp are baked offline (tools/gen_dwnlo_splash.py
 * -> build/gen/dwnlo_splash_art.h, 16-colour RLE), the same technique and run-length scheme as
 * SLOOP's splash.c / tools/gen_logo.py (credited in LICENSING.md) - that part is not copied
 * code, just the same convention. The car, the flashing windows and the "DWNLO" neon sign
 * (the only place that name appears) are drawn live, with this canvas's real primitives:
 * cv_rect, cv_text, no circles, no alpha blending. There is no persistent framebuffer (gfx.c's
 * cv_px is at most 240x144), so every change here is a full reload of the affected band from
 * the baked art, redrawn, and reblit - not a sprite over a saved background.
 *
 * SIMPLIFICATION, stated plainly: the two patrons are baked into the crowd, already standing
 * by the door - there's no live walk-in or fade for them. Only the car's drive-in/park/drive-off
 * is a genuine runtime animation. A real walk-in would need several more discrete redraws plus
 * a hand-rolled colour fade (cv_rect has no alpha), which was cut for this pass; it's a
 * reasonable follow-up if wanted. */
#include "dwnlo_splash_art.h"

/* set only by the host/web branch below; ui_frame()/ui_input_only() (ui.c) hold at this plat_ms()
 * deadline before clearing to black and running ui_init() - see x0x.h for the full contract */
uint32_t dwnlo_splash_hold_until;

/* must match tools/gen_dwnlo_splash.py's WINDOWS / CAR_BAND exactly: both are re-blit targets,
 * not just drawing hints, so a mismatch here would tear the baked art */
typedef struct { int32_t x, y, w, h; } dwnlo_rect_t;
static const dwnlo_rect_t DWNLO_WINDOWS[6] = {
    {115, 54, 22, 14}, {153, 54, 22, 14}, {191, 54, 22, 14},
    {115, 76, 22, 14}, {153, 76, 22, 14}, {191, 76, 22, 14},
};
#define DWNLO_WIN_BAND_Y 50
#define DWNLO_WIN_BAND_H 40              /* rows 50-89: both window rows, clear of the sign at 92 */
#define DWNLO_CAR_BAND_Y 148
#define DWNLO_CAR_BAND_H 24
#define DWNLO_CAR_PARK_X 95                     /* where it stops: just clear of the bouncer */

/* loads rows y0..y0+h-1 of the baked scene into the canvas (no blit: for drawing more on top
 * first, same RLE scan SLOOP's loader uses, generalised to an arbitrary band instead of only
 * sequential chunks from row 0 - this is not a hot path, a splash redraws a handful of times) */
static void dwnlo_splash_load_band(int32_t y0, int32_t h)
{
    uint32_t lo = (uint32_t)y0 * DWNLO_SPLASH_W, hi = lo + (uint32_t)DWNLO_SPLASH_W * (uint32_t)h,
             pos = 0, i;
    cv_begin(DWNLO_SPLASH_W, (uint32_t)h, C_BLACK);
    for (i = 0; i < sizeof DWNLO_SPLASH_RLE && pos < hi; i++) {
        uint32_t run = (DWNLO_SPLASH_RLE[i] >> 4) + 1u, c = DWNLO_SPLASH_RLE[i] & 15u, p;
        if (c && pos + run > lo)
            for (p = pos < lo ? lo : pos; p < pos + run && p < hi; p++)
                cv_px[p - lo] = swap16(DWNLO_SPLASH_PAL[c]);
        pos += run;
    }
}

static void dwnlo_splash_bg_full(void)
{
    int32_t y0;
    for (y0 = 0; y0 < DWNLO_SPLASH_H; y0 += 144) {
        int32_t h = DWNLO_SPLASH_H - y0 > 144 ? 144 : DWNLO_SPLASH_H - y0;
        dwnlo_splash_load_band(y0, h);
        cv_blit((240u - DWNLO_SPLASH_W) / 2u, (uint32_t)y0);
    }
}

/* the club's own marquee carries the name - it's drawn nowhere else. No glow primitive on
 * this canvas: a dim pass one pixel off, then a crisp pass on top, is the cheap approximation */
static void dwnlo_splash_sign(void)
{
    /* the gap between the two window rows (bottom at 90) and the awning (top at 108) */
    cv_begin(120, 14, RGB(21, 18, 31));          /* matches the baked wall colour behind it */
    cv_text(11, 0, &FONT_B, "DWNLO", RGB(120, 20, 70));
    cv_text(10, 0, &FONT_B, "DWNLO", RGB(255, 63, 150));
    cv_blit(110, 92);
}

static void dwnlo_splash_windows(uint32_t lit_mask)
{
    uint32_t i;
    dwnlo_splash_load_band(DWNLO_WIN_BAND_Y, DWNLO_WIN_BAND_H);
    for (i = 0; i < 6u; i++)
        if (lit_mask & (1u << i)) {
            const dwnlo_rect_t *w = &DWNLO_WINDOWS[i];
            cv_rect(w->x, w->y - DWNLO_WIN_BAND_Y, w->w, w->h, RGB(255, 207, 122));
        }
    cv_blit(0, DWNLO_WIN_BAND_Y);
}

/* x is the car's left edge in scene columns; off either edge, this just leaves the band erased */
static void dwnlo_splash_car(int32_t x)
{
    dwnlo_splash_load_band(DWNLO_CAR_BAND_Y, DWNLO_CAR_BAND_H);
    cv_rect(x + 4, 6, 32, 10, RGB(156, 61, 240));          /* body */
    cv_rect(x + 11, 1, 16, 6, RGB(156, 61, 240));          /* roof */
    cv_rect(x + 13, 2, 12, 4, RGB(31, 24, 48));            /* glass */
    cv_rect(x + 2, 14, 6, 6, RGB(10, 10, 13));             /* wheels */
    cv_rect(x + 34, 14, 6, 6, RGB(10, 10, 13));
    cv_rect(x + 36, 9, 3, 2, RGB(255, 242, 194));          /* headlight: front, the way it's going */
    cv_rect(x + 2, 9, 2, 2, RGB(255, 63, 150));            /* taillight */
    cv_blit(0, DWNLO_CAR_BAND_Y);
}

static void dwnlo_splash_foot(void)
{
    cv_begin(220, 16, C_BLACK);
    cv_text(0, 0, &FONT_S, "v0.1 - by Jeff Webb", RGB(201, 205, 216));
    cv_blit(10, 196);
}

static void dwnlo_splash(void)
{
    dwnlo_splash_bg_full();
    dwnlo_splash_sign();
    dwnlo_splash_foot();
#ifndef X0X_HOST                              /* the device: the real drive-in/park/drive-off */
    {
        int32_t x;
        for (x = -44; x <= DWNLO_CAR_PARK_X; x += 14) {
            dwnlo_splash_car(x);
            dwnlo_splash_windows(((uint32_t)x & 2u) ? 0x15u : 0x2Au);
            fm1_delay_ms(130);                      /* half speed: every delay here is doubled */
        }
        dwnlo_splash_windows(0x3Fu);
        fm1_delay_ms(560);
        for (x = DWNLO_CAR_PARK_X; x <= 280; x += 16) {
            dwnlo_splash_car(x);
            fm1_delay_ms(110);
        }
        dwnlo_splash_windows(0u);
        fm1_delay_ms(500);
    }
#else                                            /* host/web: one representative frame, held for
                                                    * real simulated time via dwnlo_splash_hold_until
                                                    * (no fm1_delay_ms here - it's a blocking hardware
                                                    * timer, wrong for a tick-driven audio render loop;
                                                    * see x0x.h for how ui.c honours the hold) */
    {
        extern uint32_t plat_ms(void);          /* declared ahead of plat.h's own include here */
        dwnlo_splash_car(DWNLO_CAR_PARK_X);
        dwnlo_splash_windows(0x3Fu);
        dwnlo_splash_hold_until = plat_ms() + 3600u;    /* half speed: double the device path's total */
    }
#endif
}

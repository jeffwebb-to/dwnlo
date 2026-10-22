#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Jeff Webb
"""Build an animated GIF of DWNLO's boot splash (car driving in, parking, windows flashing,
driving off) for the README, by replaying the drawing math from firmware/src/dwnlo_splash.c
on top of the baked background from tools/gen_dwnlo_splash.py. This is not a device capture
(the real screen can't be recorded to a GIF): it's a from-scratch redraw of the same layout
and timing, with a system font standing in for the device's bitmap font. Read-only against
the firmware tree: writes only .github/splash.gif.

  tools/gen_splash_gif.py
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_dwnlo_splash import scene, PAL, WINDOWS, W  # noqa: E402
from PIL import Image, ImageDraw, ImageFont  # noqa: E402

OUT = Path(__file__).resolve().parent.parent / ".github" / "splash.gif"

CAR_PARK_X = 95
WIN_LIT = (255, 207, 122)
FOOT_TEXT = "v0.1 - by Jeff Webb"


def load_font(size):
    for path in ("/System/Library/Fonts/Monaco.ttf", "/System/Library/Fonts/Menlo.ttc"):
        try:
            return ImageFont.truetype(path, size)
        except OSError:
            continue
    return ImageFont.load_default()


FONT_SIGN = load_font(13)
FONT_FOOT = load_font(10)


def draw_windows(im, lit_mask):
    d = ImageDraw.Draw(im)
    for i, (x, y, w, h) in enumerate(WINDOWS):
        if lit_mask & (1 << i):
            d.rectangle([x, y, x + w - 1, y + h - 1], fill=WIN_LIT)


def draw_sign(im):
    d = ImageDraw.Draw(im)
    d.text((121, 92), "DWNLO", font=FONT_SIGN, fill=(120, 20, 70))
    d.text((120, 92), "DWNLO", font=FONT_SIGN, fill=(255, 63, 150))


def draw_car(im, x):
    d = ImageDraw.Draw(im)
    y0 = 148
    d.rectangle([x + 4, y0 + 6, x + 4 + 32 - 1, y0 + 6 + 10 - 1], fill=(156, 61, 240))
    d.rectangle([x + 11, y0 + 1, x + 11 + 16 - 1, y0 + 1 + 6 - 1], fill=(156, 61, 240))
    d.rectangle([x + 13, y0 + 2, x + 13 + 12 - 1, y0 + 2 + 4 - 1], fill=(31, 24, 48))
    d.rectangle([x + 2, y0 + 14, x + 2 + 6 - 1, y0 + 14 + 6 - 1], fill=(10, 10, 13))
    d.rectangle([x + 34, y0 + 14, x + 34 + 6 - 1, y0 + 14 + 6 - 1], fill=(10, 10, 13))
    d.rectangle([x + 36, y0 + 9, x + 36 + 3 - 1, y0 + 9 + 2 - 1], fill=(255, 242, 194))
    d.rectangle([x + 2, y0 + 9, x + 2 + 2 - 1, y0 + 9 + 2 - 1], fill=(255, 63, 150))


def draw_foot(im):
    d = ImageDraw.Draw(im)
    d.text((10, 196), FOOT_TEXT, font=FONT_FOOT, fill=(201, 205, 216))


def frame(car_x, lit_mask, with_foot=True):
    im = scene().convert("RGB")
    draw_sign(im)
    draw_windows(im, lit_mask)
    if car_x is not None:
        draw_car(im, car_x)
    canvas = Image.new("RGB", (W, 210), PAL["sky_top"])
    canvas.paste(im, (0, 0))
    if with_foot:
        draw_foot(canvas)
    return canvas


def main():
    frames, delays = [], []

    def add(car_x, lit_mask, ms):
        frames.append(frame(car_x, lit_mask))
        delays.append(ms)

    xs_in = list(range(-44, CAR_PARK_X + 1, 14))
    for i, x in enumerate(xs_in):
        add(x, 0x15 if (x & 2) else 0x2A, 130)
    add(CAR_PARK_X, 0, 1400)                    # dark while it's parked

    xs_out = list(range(CAR_PARK_X, 281, 16))   # unison flash starts the moment it pulls away,
    for i, x in enumerate(xs_out):              # not the drive-in's independent alternating one
        add(x, 0x3F if i % 2 == 0 else 0, 220)
    for _ in range(2):                          # a couple more cycles once it's off-screen
        add(None, 0x3F, 220)
        add(None, 0, 220)
    add(None, 0, 900)

    scale = 3
    up = [f.resize((f.width * scale, f.height * scale), Image.NEAREST) for f in frames]
    up[0].save(OUT, save_all=True, append_images=up[1:], duration=delays, loop=0,
               optimize=True, disposal=2)
    print(f"{OUT}: {len(up)} frames, {up[0].size}")


if __name__ == "__main__":
    main()

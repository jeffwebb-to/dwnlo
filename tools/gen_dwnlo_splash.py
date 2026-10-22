#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Jeff Webb
"""DWNLO's boot splash art: the static part of the scene (sky, street, the club building,
the sidewalk crowd, the streetlamp, the bouncer) rendered once offline with Pillow and
RLE-compressed into a C header, the same technique SLOOP's tools/gen_logo.py uses for its
boot logo. The car, the window flashes and the "DWNLO" neon sign are NOT in this image —
those are drawn live at runtime (firmware/src/dwnlo_splash.c) with the device's actual
drawing primitives (cv_rect / cv_text: no circles, no gradients, no alpha), so this script
only needs to produce the parts that don't move.

The 4-bit RLE scheme (run-length nibble, colour-index nibble) allows at most 15 colours
plus index 0 ("no pixel"/black), so this scene is drawn from an explicit, exact palette
below — never PIL's automatic quantizer, which spent its whole budget smoothing a sky
gradient the first time this was tried and quietly merged the window colour into the wall.
Every fill in scene() must be one of PAL's values, by identity; write_header() asserts that.

  tools/gen_dwnlo_splash.py OUT.h
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw

W, H = 240, 190                      # the scene; firmware/src/dwnlo_splash.c reserves the
                                       # rows below H for the live "v0.1 - by Jeff Webb" line

PAL = {                                # index 0 is reserved (black / "no pixel"); 1-15 only
    "sky_top":    (13, 8, 24),
    "sky_mid":    (58, 26, 74),
    "sky_glow":   (106, 31, 74),       # also one crowd outfit colour
    "white":      (231, 227, 255),     # stars, the lamp's hot core
    "sidewalk":   (43, 44, 58),
    "dark":       (18, 19, 25),        # curb, street, door, shoes, hair, the bouncer
    "wall":       (21, 18, 31),        # the club's front
    "awning":     (90, 16, 48),
    "amber":      (255, 184, 77),      # door light, lamp glow core
    "window_off": (35, 42, 58),        # deliberately distinct from "wall"
    "lamp_mid":   (132, 94, 52),
    "outfit_a":   (58, 74, 106),       # crowd
    "skin_pale":  (236, 202, 173),     # crowd + bouncer: pale/"white" looking
    "skin":       (202, 162, 122),     # crowd: light brown
    "skin_dark":  (120, 82, 54),       # crowd: dark brown
}
assert len(PAL) == 15, len(PAL)

# Window and streetlamp positions are load-bearing: dwnlo_splash.c re-blits these exact
# bands at runtime (to clear a flash, or to erase the car's last position) by re-decoding
# this same baked image, not by redrawing plain rectangles, so a live overlay always lands
# on the real baked background instead of a flat guess at its colour. Keep both files' copies
# of WINDOWS / CAR_BAND in sync by hand; there is no shared source for them.
WINDOWS = [(115, 54, 22, 14), (153, 54, 22, 14), (191, 54, 22, 14),
           (115, 76, 22, 14), (153, 76, 22, 14), (191, 76, 22, 14)]
CAR_BAND = (148, 24)                  # (y0, h): the street-level strip the car animates in


def scene():
    c = PAL
    im = Image.new("RGB", (W, H), c["sky_top"])
    d = ImageDraw.Draw(im)

    d.rectangle([0, 0, W, 25], fill=c["sky_top"])                  # sky: three flat bands,
    d.rectangle([0, 25, W, 48], fill=c["sky_mid"])                  # not a gradient - every
    d.rectangle([0, 48, W, 70], fill=c["sky_glow"])                 # pixel must hit the palette
    for x, y in [(8, 8), (20, 14), (34, 6), (50, 10), (70, 16), (90, 5), (14, 22), (44, 18),
                 (62, 4), (6, 30), (76, 12)]:
        d.point((x, y), fill=c["white"])

    d.rectangle([0, 150, W, 168], fill=c["sidewalk"])
    d.rectangle([0, 168, W, 170], fill=c["dark"])
    d.rectangle([0, 170, W, H], fill=c["dark"])

    d.rectangle([100, 40, 240, 170], fill=c["wall"])                # the club's front
    d.rectangle([120, 108, 210, 118], fill=c["awning"])
    d.rectangle([145, 130, 185, 170], fill=c["dark"])                # door
    d.rectangle([180, 136, 182, 146], fill=c["amber"])               # door light strip
    for (wx, wy, ww, wh) in WINDOWS:                                 # unlit by default; the
        d.rectangle([wx, wy, wx + ww, wy + wh], fill=c["window_off"])  # runtime flash lights these

    for r, color in ((13, c["lamp_mid"]), (8, c["amber"])):
        d.ellipse([22 - r, 92 - r, 22 + r, 92 + r], fill=color)     # the streetlamp's glow, as
                                                                      # a few solid rings (no real
                                                                      # alpha blend on this canvas)
    d.line([(22, 92), (22, 170)], fill=c["dark"], width=2)
    d.rectangle([19, 88, 25, 94], fill=c["white"])

    d.rectangle([137, 152, 142, 170], fill=c["sidewalk"])            # the bouncer, by the door -
    d.ellipse([135, 142, 145, 152], fill=c["skin_pale"])              # pale/"white" looking head,
    d.rectangle([133, 140, 147, 143], fill=c["sidewalk"])             # and a hat (sidewalk colour,
    d.rectangle([136, 135, 144, 140], fill=c["sidewalk"])             # not "dark": the door is dark,
                                                                        # so a dark hat would vanish
                                                                        # against it - kept clear of
                                                                        # the door's x>=145 for the
                                                                        # same reason)

    crowd = [                                                        # mixed sidewalk crowd, a mix
        (30, "outfit_a", "m", "skin_pale"), (42, "sky_glow", "w", "skin"),
        (54, "outfit_a", "m", "skin_dark"), (66, "outfit_a", "w", "skin_pale"),
        (78, "sky_glow", "m", "skin_dark"), (210, "outfit_a", "w", "skin"),
        (222, "outfit_a", "m", "skin_dark"), (234, "sky_glow", "m", "skin_pale"),
        (114, "outfit_a", "m", "skin"), (124, "sky_glow", "w", "skin_dark"),  # the two who
    ]                                                                  # arrived by car, baked in
                                                                         # by the door - see
                                                                         # dwnlo_splash.c: only the
                                                                         # car animates

    for (x, color_name, kind, skin_name) in crowd:
        color = c[color_name]
        skin_color = c[skin_name]
        if kind == "w":
            d.rectangle([x - 1, 148, x + 7, 154], fill=c["dark"])        # hair, behind the head
            d.polygon([(x + 1, 154), (x + 5, 154), (x + 6, 168), (x, 168)], fill=color)  # skirt
        else:
            d.rectangle([x, 154, x + 6, 168], fill=color)                 # body
        d.rectangle([x, 168, x + 1, 169], fill=c["dark"])
        d.rectangle([x + 4, 168, x + 5, 169], fill=c["dark"])
        d.ellipse([x + 1, 146, x + 7, 152], fill=skin_color)               # head

    return im


def rgb565(c):
    r, g, b = c
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def write_header(path):
    im = scene()
    names = list(PAL)                                # fixed order: index i+1 = PAL[names[i]]
    index_of = {PAL[n]: i + 1 for i, n in enumerate(names)}
    cols = [(0, 0, 0)] + [PAL[n] for n in names]

    w, h = im.size
    src = im.load()
    px = []
    for y in range(h):
        for x in range(w):
            rgb = src[x, y]
            if rgb not in index_of:
                sys.exit(f"gen_dwnlo_splash: pixel ({x},{y}) is {rgb}, not in PAL - "
                          f"every fill in scene() must use an exact PAL colour")
            px.append(index_of[rgb])

    rle = bytearray()
    i = 0
    while i < len(px):
        j = i
        while j < len(px) and px[j] == px[i] and j - i < 16:
            j += 1
        rle.append(((j - i - 1) << 4) | px[i])
        i = j

    lines = [
        "/* generated by tools/gen_dwnlo_splash.py: the DWNLO boot splash background */",
        "#pragma once", "#include <stdint.h>",
        f"#define DWNLO_SPLASH_W {W}", f"#define DWNLO_SPLASH_H {H}",
        "static const uint16_t DWNLO_SPLASH_PAL[16] = {" + ", ".join(f"0x{rgb565(c):04X}" for c in cols) + "};",
        f"static const uint8_t DWNLO_SPLASH_RLE[{len(rle)}] = {{",
    ]
    for k in range(0, len(rle), 24):
        lines.append("    " + ", ".join(str(b) for b in rle[k:k + 24]) + ",")
    lines.append("};")
    Path(path).write_text("\n".join(lines) + "\n")
    print(f"dwnlo splash: {W}x{H}, {len(rle)} B RLE, {len(cols)} colours -> {path}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    write_header(sys.argv[1])

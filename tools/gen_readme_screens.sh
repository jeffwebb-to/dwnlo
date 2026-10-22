#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Jeff Webb
# Re-generate the README's screenshots (.github/screens/*.gif) from the real host simulator:
# builds host/x0x_host if needed, expands tools/gen_readme_screens.x0x's "### BURST name N" lines
# into N "wait 120 / shot name_NN.png" pairs, runs it, then assembles each name's frames into a
# looping GIF (2x nearest-neighbor: 240x240 is the FM-1's real LCD, GitHub would show it tiny).
#   tools/gen_readme_screens.sh
set -e
cd "$(dirname "$0")/.."
[ -x build/host/x0x_host ] || sh host/build_host.sh
WORK="$(mktemp -d)"
OUT="$WORK/out"
mkdir -p "$OUT"

awk '
/^### BURST / {
    name = $3; n = $4 + 0
    for (i = 0; i < n; i++) {
        printf "wait 120\n"
        printf "shot %s_%02d.png\n", name, i
    }
    next
}
{ print }
' tools/gen_readme_screens.x0x > "$WORK/script.x0x"

build/host/x0x_host "$WORK/script.x0x" "$OUT"

python3 - "$OUT" <<'PY'
import re
import sys
from collections import defaultdict
from pathlib import Path
from PIL import Image

out = Path(sys.argv[1])
dst = Path(".github/screens")
dst.mkdir(parents=True, exist_ok=True)

frames = defaultdict(list)
for png in out.glob("*.png"):
    m = re.match(r"(.+)_(\d+)\.png$", png.name)
    if not m:
        continue
    frames[m.group(1)].append((int(m.group(2)), png))

for name, items in frames.items():
    items.sort()
    ims = [Image.open(p) for _, p in items]
    up = [im.resize((im.width * 2, im.height * 2), Image.NEAREST) for im in ims]
    target = dst / f"{name}.gif"
    up[0].save(target, save_all=True, append_images=up[1:], duration=120, loop=0,
               optimize=True, disposal=2)
    print(f"{target}: {len(up)} frames, {up[0].size}")
PY
rm -rf "$WORK"

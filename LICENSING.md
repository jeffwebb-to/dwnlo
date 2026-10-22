# DWNLO licensing

DWNLO is free software under the GNU General Public License, version 3 only
(`GPL-3.0-only`, full text in `LICENSE`). It is a fork of X0X, itself a fork of Felucca, and
keeps Felucca's licence. If you distribute DWNLO, or firmware derived from it, you must give
your recipients its complete corresponding source under the same licence.

## Where the code comes from

| What | Origin | Licence |
| --- | --- | --- |
| Platform: `firmware/hal/`, `firmware/loader/`, `firmware/src/{libc,lcd,gfx,usb,storage,ota}.c`, `tools/` (build, package, install, upload), `web/` (installer) | [Felucca](https://github.com/hugelton/Felucca), Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments; `usb.c` and `storage.c` changed for X0X | GPL-3.0-only |
| `firmware/src/app/{slots,editor,panel,main_fm1}.c` | adapted from Felucca's `eng_sample.c`, `editor.c`, `panel.c`, `main.c` / `audio.c` | GPL-3.0-only |
| 909 (`dsp/drum909*`, `dsp/fxbus*`) | ported from [9W9](https://github.com/athousanddetails/schwung-9W9) (athousanddetails), itself after [ER-99](https://github.com/matthewcieplak/er-99) (Matthew Cieplak) | GPL-3.0 |
| 909 hi-hat, ride and crash samples (`assets/909/`) | ER-99 via 9W9 | GPL-3.0 |
| 808 (`dsp/drum808*`) | ported from [8W8](https://github.com/athousanddetails/schwung-8W8) (athousanddetails); circuit models after the 808's service notes and Werner / Abel / Smith; rim shot after sc808 (Yoshinosuke Horiuchi / Sam Aaron) | GPL-3.0 |
| 303 (`dsp/bass303*`) | ported from [schwung-303](https://github.com/charlesvestal/schwung-303): Open303 by Robin Schmidt (MIT), Devilfish extensions after jc303 (midilab), RAT drive after dm-Rat (Dave Mollen) | GPL-3.0 (Open303 parts MIT) |
| TB-3PO (`seq/tb3po*`) | ported from [schwung-tb3po](https://github.com/charlesvestal/schwung-tb3po), itself a port of the Phazerville Hemisphere Suite `TB_3PO` applet (djphazer and contributors) | GPL-3.0 |
| The second core's start-up and job hand-off (`firmware/hal/fm1_cpu1.*`) | X0X, after what [Melodee](https://github.com/keremimo/melodee) (keremimo)'s dual-core audio found, with [Jangada](https://github.com/zednaked/jangada) (zednaked)'s measurements and mailbox layout | GPL-3.0-only |
| The idea of a browser emulator for FM-1 firmware (`web/emu/`) | [Groove OS](https://www.groove-os.com/) by Peter Gombos, whose site runs its firmware in the browser; no code from it | credit only |
| The boot splash (`firmware/src/dwnlo_splash.c`, `tools/gen_dwnlo_splash.py`) | DWNLO, Jeff Webb; the baked-RLE-bitmap technique (run-length nibble + 16-colour palette, scanned and reblit in bands) follows SLOOP's `firmware/src/splash.c` / `tools/gen_logo.py` (Chance Roth) convention - no code copied from it | GPL-3.0-only |
| The 808 Bassline (`dsp/bass808m*`), in the second acid-voice's old slot | DWNLO, Jeff Webb; a from-scratch lean voice (not a port of `drum808.c`'s kick circuit - see the file's own header for why), same spirit as the kit's 808 sound | GPL-3.0-only |
| Blip, the synth pluck engine (`dsp/blip*`), in BREAK's old slot | DWNLO, Jeff Webb; a from-scratch monophonic 2-op FM voice for fast melodic lines (Planet Rock's riff was the reference) - a modulator envelope that decays faster than the carrier's for the bright attack "blip", plus a small one-pole lowpass with its own slow sine LFO, scoped to this one voice | GPL-3.0-only |
| The 909/808 drum grid's recolouring and step-page dots (`firmware/src/app/ui.c`'s `draw_drum`) | DWNLO, Jeff Webb; one fixed colour per track row and a distinct accent row, on top of X0X's existing grid | GPL-3.0-only |
| The knob page's per-part colour themes (`firmware/src/app/ui.c`'s `arc`/`draw_knobs`) | DWNLO, Jeff Webb; a themed pointer colour (white, except the 808's own real orange line) added to X0X's existing knob-arc drawing - the panel itself stays black on every part, since the existing palette's dim tones are calibrated for contrast against black and a grey "303 silver box" panel would wash them out on the real screen | GPL-3.0-only |
| Everything else in `firmware/src/{app,dsp,seq}`, `host/`, `tests/` | X0X | GPL-3.0-only |

DWNLO removed the BREAK part entirely (its generator, loop playback, and user sample-upload
slots) to make static RAM room for blip above - there is no break engine, built-in
loop, or loop-upload tooling left in this tree.

## Felucca Assets

Felucca's non-GPL assets — the icon atlas, the panel photo and the Hügelton drum pack —
are **not** part of this fork and are not used by it. Felucca's section 7 additional
permission concerns those assets only, so it is not needed here.

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| Barlow Semi Condensed (The Barlow Project Authors), the default UI face | SIL OFL 1.1 | `assets/fonts/BarlowSemiCondensed-*.ttf`, `assets/fonts/Barlow-OFL.txt` |
| Terminus (Dimitar Toshkov Zhekov), the `terminus` font set | SIL OFL 1.1 | `assets/fonts/ter-u*.bdf`, `assets/fonts/Terminus-LICENSE.txt` |
| Fukiai icon font (Hügelton Instruments), web pages only | MIT | `web/fukiai.ttf`, `web/FUKIAI-LICENSE.txt` |
| JieLi AC79 SDK: `uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin` are read from your SDK checkout at build time and placed in the package; no SDK files are in this tree | Apache-2.0 | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK> |

## Radio

Neither X0X nor DWNLO ever enables the Bluetooth / Wi-Fi radio of the hardware.

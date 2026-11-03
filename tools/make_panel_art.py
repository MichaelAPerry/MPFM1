#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Draw docs/img/panel.svg: a schematic of the FM-1 panel with each control's role in Stamp Composer.
Not to scale; the printed names are the panel's own. Plain SVG (no fonts embedded), written as text.
  tools/make_panel_art.py [OUT.svg]"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "docs" / "img" / "panel.svg"
INK, HOT, COOL, SHEET, PAPER, RULE, MUTED = "#1e2b5a", "#c23a22", "#d9a21b", "#efe9dc", "#fbf8f1", "#ddd4c2", "#4d5577"
W, H = 760, 548
o = []
add = o.append


def text(x, y, s, size=11, weight=400, fill=INK, anchor="middle"):
    s = s.replace("&", "&amp;")
    add(f'<text x="{x}" y="{y}" font-size="{size}" font-weight="{weight}" fill="{fill}" text-anchor="{anchor}">{s}</text>')


def knob(cx, cy, name, role, role2="", r=21, hot=False):
    add(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{PAPER}" stroke="{INK}" stroke-width="2"/>')
    add(f'<line x1="{cx}" y1="{cy - r + 5}" x2="{cx}" y2="{cy - 6}" stroke="{HOT if hot else INK}" stroke-width="3" stroke-linecap="round"/>')
    text(cx, cy + r + 15, name, 12, 700)
    text(cx, cy + r + 29, role, 10.5, 400, MUTED)
    if role2:
        text(cx, cy + r + 42, role2, 10.5, 400, MUTED)


def button(x, y, w, name, role, hot=False):
    add(f'<rect x="{x}" y="{y}" width="{w}" height="44" rx="7" fill="{PAPER}" stroke="{HOT if hot else INK}" stroke-width="2"/>')
    text(x + w / 2, y + 19, name, 13, 700)
    text(x + w / 2, y + 35, role, 10.5, 400, MUTED)


add(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" role="img" '
    f'font-family="Barlow, \'Helvetica Neue\', Helvetica, Arial, sans-serif">')
add('<title>The FM-1 panel with each control\'s job in Stamp Composer</title>')
add(f'<rect x="1" y="1" width="{W - 2}" height="{H - 2}" rx="16" fill="{SHEET}" stroke="{RULE}" stroke-width="2"/>')

# left: MASTER, SELECT, PRESETS, ALGORITHM
for (cx, cy, n, r1, r2) in ((70, 66, "MASTER", "volume", "(the unit's own)"), (180, 66, "SELECT", "move the cursor", ""),
                            (70, 168, "PRESETS", "pick the stamp", "(the sound)"), (180, 168, "ALGORITHM", "pick the layer", "(1 to 8)")):
    knob(cx, cy, n, r1, r2)

# centre: the screen
add(f'<rect x="268" y="26" width="224" height="224" rx="8" fill="#14172b" stroke="{INK}" stroke-width="3"/>')
text(284, 46, "STAMP", 12, 700, "#f4efe3", "start")
text(476, 46, "EDIT", 11, 700, "#f0b64a", "end")
for i in range(5):
    add(f'<rect x="{282 + i * 30}" y="54" width="24" height="24" rx="3" fill="{"#e5482e" if i == 2 else "#3a4066"}"/>')
for i in range(5):
    add(f'<line x1="282" y1="{104 + i * 9}" x2="478" y2="{104 + i * 9}" stroke="#8d93b5" stroke-width="1"/>')
for (x, y) in ((312, 131), (346, 122), (380, 113), (414, 122), (448, 131)):
    add(f'<circle cx="{x}" cy="{y}" r="5" fill="#f0b64a"/>')
text(380, 168, "the staff page", 10.5, 400, "#c9cde6")
for i in range(4):
    x = 280 + i * 49
    add(f'<rect x="{x}" y="186" width="45" height="50" rx="4" fill="#232850" stroke="#5a628f"/>')
    text(x + 22.5, 205, f"K{i + 1}", 12, 700, "#f4efe3")
    text(x + 22.5, 224, "value", 9.5, 400, "#aeb4d6")
text(380, 268, "240 x 240 screen", 11, 600, INK)

# right: KNOB1-4
for (cx, cy, i) in ((590, 66, 1), (700, 66, 2), (590, 168, 3), (700, 168, 4)):
    knob(cx, cy, f"KNOB{i}", f"K{i}: tile {i} of 4", "", hot=True)
text(645, 256, "K1 to K4 follow the four tiles", 10.5, 600, HOT)
text(645, 270, "on the screen, left to right", 10.5, 600, HOT)

# buttons
add(f'<line x1="30" y1="282" x2="730" y2="282" stroke="{RULE}" stroke-width="2"/>')
row1 = (("FX", "mute sound"), ("SCL", "show all / one"), ("ENV", "undo"), ("LFO", "redo"), ("EDIT", "tap: next page"), ("GLO", "song book"))
row2 = (("HOME", "staff, hold: help"), ("SAVE", "save to slot"), ("ARP", "loop on/off"), ("SEQ", "grid step"), ("PLAY", "play / stop"), ("REC", "copy bar"))
for r, (y, row) in enumerate(((296, row1), (350, row2))):
    for i, (n, d) in enumerate(row):
        button(36 + i * 116, y, 108, n, d, hot=n in ("EDIT", "PLAY"))

# octave buttons and the keys
add(f'<line x1="30" y1="410" x2="730" y2="410" stroke="{RULE}" stroke-width="2"/>')
button(36, 462, 96, "OCT−", "octave down")
button(628, 462, 96, "OCT+", "octave up")
text(380, 432, "27 keys, F3 to G5 (OCT− and OCT+ move them)", 11, 600)
WHITE = (0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26)
BLACK = (1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25)
x0, kw = 140, 30
pos = {k: x0 + i * kw for i, k in enumerate(WHITE)}
for k in WHITE:
    add(f'<rect x="{pos[k]}" y="444" width="{kw - 2}" height="76" rx="3" fill="#fffdf8" stroke="{INK}" stroke-width="1.5"/>')
for k in BLACK:
    add(f'<rect x="{pos[k - 1] + kw - 9}" y="444" width="16" height="46" rx="2" fill="{INK}"/>')
text(pos[7] + 14, 514, "C4", 10.5, 700)
text(pos[19] + 14, 514, "C5", 10.5, 700)
text(pos[0] + 14, 514, "F3", 10.5, 400, MUTED)
text(pos[26] + 14, 514, "G5", 10.5, 400, MUTED)
add("</svg>")
OUT.write_text("\n".join(o) + "\n", encoding="utf-8")
print(f"{OUT}: {OUT.stat().st_size} bytes")

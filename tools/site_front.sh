#!/bin/sh
# SPDX-License-Identifier: MIT
# Install the project's own front page (site/index.html: tiles, stamp gallery, audio previews, screenshots) over the
# plain one that base/tools/make_site.py wrote, and copy what it needs into SITE/assets/ (stamps, screenshots, audio).
# install/, manual/, emu/, fm1-emu/, backup/, mirror/ and firmware/ are left alone.
#   tools/site_front.sh SITE_DIR VERSION [release|dev] [COMMIT]
# release: built by the attested release workflow, SHA256SUMS linked. dev: built by the Pages workflow from a commit.
set -eu
SITE="${1:?usage: tools/site_front.sh SITE_DIR VERSION [release|dev] [COMMIT]}"; V="${2:?version}"
KIND="${3:-release}"; COMMIT="${4:-}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PKG="stamp-$V.fwsc"
[ -f "$SITE/firmware/$PKG" ] || { echo "site_front.sh: no $SITE/firmware/$PKG"; exit 1; }
mkdir -p "$SITE/assets/stamps" "$SITE/assets/shots" "$SITE/assets/audio"
cp "$ROOT"/assets/stamps/*.png "$SITE/assets/stamps/"
cp "$ROOT"/site/audio/*.mp3 "$SITE/assets/audio/"
for s in compose mixer sound midi groove bars songbook help flats playback-hop; do
    cp "$ROOT/docs/screens/$s.png" "$SITE/assets/shots/$s.png"
done
python3 - "$ROOT" "$SITE" "$V" "$PKG" "$KIND" "$COMMIT" <<'PY'
import hashlib, html, re, sys
from pathlib import Path
root, site, v, pkg, kind, commit = Path(sys.argv[1]), Path(sys.argv[2]), *sys.argv[3:7]
repo = "https://github.com/MichaelAPerry/MPFM1"
files = "mario mushroom yoshi star flower gameboy dog cat pig swan face plane boat car heart piranha coin shyguy boo luigi peach feather bulletbill goomba bobomb spiny fruit oneup moon egg gnome".split()
chars = re.search(r"character_names\[SC_SOUNDS\]=\{(.*?)\};", (root / "app.c").read_text()).group(1)
sounds = re.search(r"sc_names\[SC_SOUNDS\]=\{(.*?)\};", (root / "dsp" / "stamp.c").read_text()).group(1)
chars, sounds = re.findall(r'"([^"]+)"', chars), re.findall(r'"([^"]+)"', sounds)
assert len(chars) == len(sounds) == len(files) == 31, (len(chars), len(sounds), len(files))
for f in files:
    assert (site / "assets" / "stamps" / f"{f}.png").exists(), f
t = lambda s: s.title().replace("Bob-Omb", "Bob-omb").replace("1-Up", "1-Up").replace("Game Boy", "Game Boy")
stamps = "\n".join(f'      <li><img src="assets/stamps/{f}.png" width="64" height="72" alt="" loading="lazy">'
                   f"<b>{html.escape(t(c))}</b><span>{html.escape(t(s))}</span></li>" for f, c, s in zip(files, chars, sounds))
pills = "\n".join(f'      <li><a class="pill" href="assets/audio/{i:02d}.mp3"><img src="assets/stamps/{f}.png" alt="" loading="lazy">'
                  f"{html.escape(t(s))}<i>&#9654;</i></a></li>" for i, (f, s) in enumerate(zip(files, sounds)))
shots = [("compose", "Staff", "Write notes: keys place them, SELECT moves the cursor, K1 to K4 set tempo, velocity, length and song end."),
         ("mixer", "Mix", "Beats per bar, and the level of the selected sound and layer."),
         ("sound", "Sound", "Tune, tone, decay and pan for each sound."),
         ("midi", "MIDI", "All 31 sounds with their channels; mode, clock and MIDI in."),
         ("groove", "Groove", "Swing and human feel, saved with the song."),
         ("bars", "Bars", "Copy, move, delete, insert and transpose runs of bars."),
         ("songbook", "Song book", "Eight slots and the Twinkle Twinkle demo."),
         ("help", "Help", "Hold HOME for this page's controls."),
         ("flats", "Flats and sharps", "Black keys spelled as you like, drawn on the right line."),
         ("playback-hop", "Playing", "Your stamp hops along the staff in time.")]
shot_html = "\n".join(f'      <li><figure><img src="assets/shots/{n}.png" width="240" height="240" loading="lazy" '
                      f'alt="The {html.escape(h)} page on the FM-1 screen"><figcaption><b>{html.escape(h)}</b>{html.escape(d)}</figcaption></figure></li>'
                      for n, h, d in shots)
raw = (site / "firmware" / pkg).read_bytes()
if kind == "release":
    note = (f"Release v{v}, built and attested by the project's GitHub workflow (signed build provenance). "
            "Check the file against SHA256SUMS before you install.")
    sums = f'<a href="{repo}/releases/download/v{v}/SHA256SUMS">SHA256SUMS</a>'
    zipu = f"{repo}/archive/refs/tags/v{v}.zip"
else:
    c = f" from commit {commit[:7]}" if commit else ""
    note = (f"Development build{c}, made by this site's workflow. It is <strong>not attested</strong>: it has no signed "
            "build provenance and no release checksums. Releases are attested.")
    sums = "SHA256SUMS"
    zipu = f"{repo}/archive/{commit or 'main'}.zip" if commit else f"{repo}/archive/refs/heads/main.zip"
    sums = '<span class="muted">SHA256SUMS</span>'
vals = {"VERSION": html.escape(v), "PKG": pkg, "SIZE": f"{len(raw):,}", "SHA": hashlib.sha256(raw).hexdigest(),
        "BUILDNOTE": note, "SUMS": sums, "ZIP": zipu, "REPO": repo, "STAMPS": stamps, "PILLS": pills, "SHOTS": shot_html}
page = (root / "site" / "index.html").read_text()
for k, val in vals.items():
    page = page.replace("{{" + k + "}}", val)
left = sorted(set(re.findall(r"\{\{[A-Z_]+\}\}", page)))
if left:
    sys.exit(f"site_front.sh: {' '.join(left)} not filled in")
(site / "index.html").write_text(page)
print(f"site_front: {site}/index.html ({kind}, v{v}, {len(raw)} B), assets/ stamps, shots, audio")
PY

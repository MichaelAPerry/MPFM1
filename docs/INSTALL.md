# Install, back up, go back

> **Not yet tested on a physical FM-1.** Read this whole page before writing anything. The FM-1 has one flash
> bank and no recovery button; recovery from a failed boot uses the chip's own USB update mode (below).

## Before you install

1. **Know what you are installing.** The installer page checks the package identity: Stamp Composer releases are
   `FM-1_56XXYYZZ` (0.1 → `FM-1_56000100`). Compare the file's SHA-256 with the release's `SHA256SUMS`.
2. **Keep the stock firmware file.** Download M-VAVE's FM-1 firmware (M-VAVE Downloads → PC Software →
   M-UPGRADE, or its `.fwsc`) before you start, so you can always go back.
3. **Back up what is on the FM-1.** Installing over stock may erase stock presets and settings you saved on the
   unit; save them with M-VAVE's own tools first. Panel calibration is the unit's and fm1-base keeps it.
4. **Use a data USB cable, Chrome or Edge on a computer,** and leave the FM-1 powered from USB.

## Install

1. Open the project's site → **Install** (fm1-base's web installer, Web MIDI).
2. Connect the FM-1, allow MIDI access, and pick it.
3. Leave **"also write m-bootloader" unticked.** That option replaces the FM-1's boot loader; this project has not
   tested it.
4. Press install and do not unplug until the page says it is done. The FM-1 restarts into Stamp Composer.

Command line alternative: `python3 base/tools/fm1_install.py stamp-X.Y.fwsc`.

## Back up and restore songs

Stamp Composer enables fm1-base's backup: the site's **Backup** page (or `python3 base/tools/fm1_backup.py`)
saves all eight song slots to one file and restores them all at once. Stop playback first.

## Back to stock

On the installer page, **Back to the stock firmware** writes M-VAVE's firmware over the same cable. M-VAVE's
M-UPGRADE also works. Songs saved by Stamp Composer may be lost: back them up first.

## If it does not start

After two failed boots the chip enters its mask-ROM update mode (USB `4C4A:8057`, "WL80UBOOT1.00"); holding REC
at power-on is reported to do the same. From a Mac or Linux, `base/bsp/tools/fm1_rescue.py` waits for that mode,
**backs up the whole flash first**, then writes stock firmware into the app area only. It does not repair a damaged
boot area.

## About the boot area

The package carries JieLi SDK V1.2.1's boot loader (SPL), as every fm1-base app does. fm1-base's own updater never
writes the boot area. M-VAVE's stock updater, used for the first install from stock, writes the package's boot area,
replacing the unit's V1.1.9 SPL with V1.2.1's — the same as installing any fm1-base or Felucca firmware. One project
(Lunar Modulator) advises shipping only V1.1.9's SPL; its stated risk is inferred, not observed. We follow fm1-base
until hardware testing says otherwise.

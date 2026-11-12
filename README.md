# Universal Console Disc Reader

An app that makes use of of PS1 and PS2 game discs from your optical drive. It's the disc reading part of the Universal Console project: it turns your physical games into files that the rest of the project can load.

When you insert a disc, the app works out whether it's a PS1 or PS2 game and identifies the title from its serial number, using the PCSX2 and DuckStation game databases. It then dumps the disc into a folder named after the game. The dumping itself is done by [redumper](https://github.com/superg/redumper), a tool built for accurate, preservation quality copies; this app is a simple Qt front end on top of it.

## Usage

### Starting the app
The app looks for redumper and the game databases relative to where it's launched, so launching it from somewhere else will break the disc check and dump. The log at the bottom of the window should say how many titles were loaded; if it says 0, the databases weren't found.

### Checking a disc
1. Insert a PS1 or PS2 disc and wait for the drive to finish spinning up.
2. Choose your drive letter from the **Drive** list (defaults to `E:`).
3. Click **Check Disc**. The status line will show one of:
   - **Ready to Dump** (green): the game was identified and hasn't been dumped yet, e.g. `[PS2] Gran Turismo 4 (SCUS-97328) — Ready to Dump`
   - **Already Dumped!** (orange): a dump for this game already exists in `dumps/`.
   - **No PS1/PS2 disc found** (red): no disc in that drive, the wrong drive letter, or not a PlayStation disc.

If a game isn't in the datbases, its serial number is shown in place of the title.

### Dumping
1. Click **Start Dump**. The app checks the disc again automatically, so you can skip **Check Disc** if you want.
2. redumper's output appears in the log window and the progress bar tracks it. A dump runs through several stages, so the bar resets a few times. That's normal.
3. Dumps can take anywhere from several minutes to over an hour, depending on the disc, your drive and how scratched the disc is. redumper retries bad sectors up to 50 times.
4. When it finishes, the log shows **[SUCCESS]** and the status changes to **Already Dumped!**. Any other ending shows redumper's exit code; check the log for the reason.
5. Click **Cancel** to stop a dump at any time. A cancelled dump is incomplete and shouldn't be used.

### Where dumps go
Dumps are saved to `dumps/<Game Title>/` in the folder you launched the app from:
- PS2 DVD games produce an `.iso`.
- PS1 and PS2 CD games produce a `.cue` with `.bin` files.
- redumper also writes logs and other data files alongside them.

Discs that aren't recognized as PlayStation are saved to `dumps/disc_dump/`, and each new one replaces the last.

**Dumping a game again overwrites its existing files**, so move or back up a good dump before re dumping.

## Credits
[redumper](https://github.com/superg/redumper), [PCSX2](https://github.com/PCSX2/pcsx2) and [DuckStation](https://github.com/stenzek/duckstation) (game databases).

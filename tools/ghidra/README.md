# Studying the original executable with Ghidra

OpenHogs is a new engine, but some of the original game's behaviour
(which map each mission uses, which video plays when, which sky a level
has...) is only written down in the game's code. These notes explain how
to look at that code with [Ghidra](https://github.com/NationalSecurityAgency/ghidra),
the free reverse-engineering suite (Apache-2.0), the way it was done for
`docs/formats/executable.md`.

> **Never commit what Ghidra produces.** Projects, listings and
> decompiled text are derived from the game's executable, which belongs
> to its owners. Keep them outside the repository (the commands below use
> `/tmp`).

## 1. Install Ghidra

1. Install a Java 21 JDK (for example `sudo apt install openjdk-21-jdk`,
   or Eclipse Temurin on Windows).
2. Download a release zip from the
   [Ghidra releases page](https://github.com/NationalSecurityAgency/ghidra/releases)
   (OpenHogs used 11.4.2) and unzip it anywhere, e.g. `/tmp/ghidra`.

## 2. Import and analyse the game

The files to analyse are in your own installation of the game:

| File | What it is |
|------|------------|
| `warhogs_.exe` | the game itself: rules, tables, menus, videos, sound choices |
| `Data/_d3d.dll` | the Direct3D renderer, with exported `af...` functions (`afLoadTims`, `afDrawText`...) |
| `wh32LIB.DLL` | a copy-protection loader (LaserLok), not interesting for the game's logic |

```sh
GHIDRA=/tmp/ghidra/ghidra_11.4.2_PUBLIC
GAME="$HOME/.steam/steam/steamapps/common/HogsofWar"
mkdir -p /tmp/ghproj
"$GHIDRA/support/analyzeHeadless" /tmp/ghproj hogs -import "$GAME/warhogs_.exe"
"$GHIDRA/support/analyzeHeadless" /tmp/ghproj hogs -import "$GAME/Data/_d3d.dll"
```

The analysis takes a few minutes. Afterwards the project can also be
opened in the Ghidra window (`ghidraRun`).

## 3. Export the decompiled code as text

`export_decompiled.py` writes every function's decompiled C to one file,
with the strings each function uses listed above it, so you can search it
with `grep` (for example for `"fmv"` or `"sfxnight"`):

```sh
"$GHIDRA/support/analyzeHeadless" /tmp/ghproj hogs \
    -process warhogs_.exe -noanalysis \
    -scriptPath tools/ghidra -postScript export_decompiled.py /tmp/hogs_decompiled.c
```

## 4. Reading tables

Function addresses in Ghidra are virtual addresses (the executable is
loaded at `0x400000`). Data tables are easiest to read straight from the
file: for `warhogs_.exe` (Steam v1.2b) a virtual address in the `.data`
section (from `0x4C0000`) maps to the file offset *VA* − `0x401200`, one
in `.text` or `.rdata` to *VA* − `0x400C00`. The offsets in
`docs/formats/executable.md` are given both ways.

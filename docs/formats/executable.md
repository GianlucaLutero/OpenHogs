# The original executable

What OpenHogs has learned by reading the code of the PC game
(`warhogs_.exe`, Steam release v1.2b) with Ghidra (`tools/ghidra/README.md`).
Nothing from the executable is copied into OpenHogs: the findings below
are facts (which file, which number, which order) that the new engine
reproduces with its own code.

Addresses are given as file offset / virtual address. Function names are
Ghidra's automatic names (`FUN_<address>`), so they can be found again in
a fresh analysis.

## The files

| File | What it is |
|------|------------|
| `warhogs_.exe` | The game: menus, rules, campaign, AI, sound and video choices. Sections: `.text` from VA 0x401000 (file 0x400), `.rdata` 0x4BC000 (0xBB400), `.data` 0x4C0000 (0xBEE00). |
| `Data/_d3d.dll` | The renderer. Exports `af...` functions (`afLoadTims`, `afDrawText`, ...) that the game calls for textures, models and text. It only draws: no game data is chosen there. |
| `wh32LIB.DLL` | A LaserLok copy-protection loader that starts the game. Nothing for OpenHogs. |

## Levels

### Names: 0xD0790 / VA 0x4D1990

59 pointers to map names; index *n* is level *n*:

| # | Map | # | Map | # | Map | # | Map |
|---|-----|---|-----|---|-----|---|-----|
| 0 | ROAD | 15 | BRIDGE | 30 | DVAL | 45 | PLAY2 |
| 1 | TRENCH | 16 | BAY | 31 | ICE | 46 | ICEFLOW |
| 2 | RUMBLE | 17 | DESVAL | 32 | BUTE | 47 | RIDGE |
| 3 | DEVI | 18 | SNAKE | 33 | MAZE | 48 | ARCHI |
| 4 | TWIN | 19 | EMPLACE | 34 | SEPIA1 | 49 | DEMO2 |
| 5 | ZULUS | 20 | KEEP | 35 | DBOWL | 50 | ISLAND |
| 6 | SNIPER | 21 | SUPLINE | 36 | MLAKE | 51 | LAKE |
| 7 | GUNS | 22 | TESTER | 37 | CMASS | 52 | ONEWAY |
| 8 | OASIS | 23 | FOOT | 38 | ARTGUN | 53 | GENSNOW |
| 9 | MASHED | 24 | FINAL | 39 | DVAL2 | 54 | GENDESRT |
| 10 | CAMP | 25 | ESTU | 40 | HELL3 | 55 | GENBRACK |
| 11 | LIBERATE | 26 | DEMO | 41 | HELL2 | 56 | GENMUD |
| 12 | MEDIX | 27 | BOOM | 42 | LUNAR1 | 57 | GENCHALK |
| 13 | FJORDS | 28 | BHILL | 43 | CREEPY2 | 58 | GENLAVA |
| 14 | EYRIE | 29 | LECPROD | 44 | PLAY1 | | |

The displayed name of level *n* is entry 11 + *n* of `gtext`. Levels
53–58 are the types of generated map. OpenHogs: `game::allLevels()` in
`src/game/src/Levels.cpp`.

### Properties: 0xD4010 / VA 0x4D5210

53 records of 15 int32 (60 bytes), one per fixed level. Field 0 is the
**sky**; the other fields look like colours (values such as `0xB0B0B0`),
probably fog and light, and are not decoded yet.

| Sky | Dome texture (`Chars/<name>.mad` on `Chars/skydome.mad`) | Weather | Sound bank |
|-----|------|---------|------------|
| 0 | coldsky | snow (`Chars/snow.mtd`) | day |
| 1 | desert | – | day |
| 2, 3, 4 | night1 | – | night |
| 5 | ominous | rain (`Chars/rain.mtd`) | day |
| 6 | sunny | – | day |
| 7 | sunrise | – | day |
| 8 | sunset | – | night |
| 9 | toy | – | day |
| 10 | space | – | day |

- The weather is drawn by `FUN_004a9ed0`, called only for skies 0 and 5.
- `FUN_00485550` loads `audio\sfxnight.srl` for skies 2, 3, 4 and 8 and
  `audio\sfxday.srl` otherwise. In the PC release the two banks are
  byte-for-byte identical, so the night ambience files `AMB_1N.wav` and
  `AMB_2N.wav` are never heard (OpenHogs plays them at night:
  `docs/open-decisions.md`).

## Campaign

| Table | Offset / VA | Content |
|-------|-------------|---------|
| Mission order | 0xD05F0 / 0x4D17F0 | 26 level numbers, mission 0 (Boot Camp) to 25 |
| Turn times | 0xD0660 / 0x4D1860 | 26 values in seconds (99 = no hurry) |

Details and the resulting mission list: `docs/formats/campaign.md`.

## Videos

The PC videos are Bink files (`BIKf`, 320 × 240, about 25 frames per
second) in `Language/Streams/`. `FUN_0043ee10` plays one; a key press
skips it (flag at VA 0x520848).

**Start-up** (`FUN_0047de90`): `infologo.bik`, `sheff.bik`, then
`fmv 01` and `fmv 02`.

**Between missions** (`FUN_004821c0`), with the number of the next
mission held at VA 0x51F17B:

| Next mission | Video |
|--------------|-------|
| 6, 11, 16, 21 (a new island) | `fmv 03`, `fmv 04`, `fmv 05`, `fmv 06` |
| 25 (Isle of Swill) | `fmv 07` |
| campaign complete (in hard mode: back to 1) | `fmv 08`, then `fmv 09` |

The names `fmv03`...`fmv08` are an array of pointers at VA 0x4D18D0.
On PC, `fmv 02` and `fmv 09` are 1-byte placeholders: the second intro
film and the hard-mode ending exist only on PlayStation. OpenHogs:
`game::introCutscenes()` and `game::campaignCutscenes()`.

## Sound

- **Speech**: `FUN_0043af70` builds paths such as `speech\sku1\pig...`;
  other sections are `sku1\train` (Boot Camp) and `sku1\sarge\` (the
  sergeant). Which line goes with which event is still to be mapped
  (block `original-audio`).
- **Music**: `FUN_0043b4c0` plays clip *index* + 3, up to 0x1D.

## Resources loaded for a match (HUD and effects)

| File | Entries |
|------|---------|
| `Language/Tims/dashtims.mad` | 46: power bar (`newpow1`–`7`, `powg1`), clock (`clock01`–`04`, `timlit`, `timer0`–`9`), `sights`, `target`, `pcpie4`, `score1`–`2`, pause (`pause1`–`8`), angle (`ang1`–`4`)... |
| `Language/Tims/tboxtims.mad` | 4: text box frame (`npro1`–`4`) |
| `Language/Tims/mapicons.mtd` | 8: radar map (`map1`, `iconpig`, `iconhart`, `iconpkup`, `iconprop`, `bomb`, `rain`, `snow`) |
| `Language/Tims/facetims.mad` | 25: `aburst`, `croshair`, `point01`–`02`, `arrow000`, weapon icons `wepn01`–`20` |
| `Language/Tims/flagtims.mad` | 1: `cross` |
| `Language/Tims/expltims.mad` | 27: damage numbers (`num0`–`9`), `heart`, smoke and fire frames (`pcfx003`–`013`), `splodge`, `Shad_org` (shadow) |
| `Chars/sight.mtd`, `Chars/propoint.mtd` | aiming sight, promotion point marker |
| `Maps/<map>.MTD` | the map's own textures |

Their exact positions on screen are the subject of block `original-hud`.

## Differences between versions

Collected from guides, reviews and fan sites (see the README's Credits);
they decide what block `version-variants` has to offer.

| Version | Notes |
|---------|-------|
| PlayStation PAL, SLES-01041 | Text in English, French, German, Spanish and Italian; English voices (Rik Mayall). |
| PlayStation PAL localised, SLES-02766/02767/02768/02769 | French, German, Spanish and Italian editions with local voice actors (Italian: Giovanni Battezzato, December 2001). |
| PlayStation NTSC-U, SLUS-01195 | English. |
| PC (1.2b, Steam/GOG) | Computer hogs do not walk between shots; no looping turn theme (team theme, then random tracks); no New Game+ "hard mode" (enemies one rank higher, extra ending `fmv 09`); more objects on the maps; generally harder; partial localisations. |

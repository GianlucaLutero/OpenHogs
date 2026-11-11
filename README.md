# OpenHogs

[![Build and test](https://github.com/GianlucaLutero/OpenHogs/actions/workflows/build.yml/badge.svg?event=pull_request)](https://github.com/GianlucaLutero/OpenHogs/actions/workflows/build.yml)

**OpenHogs** is an open-source reimplementation of *Hogs of War*
(Infogrames / Gremlin, 2000), the turn-based artillery game with pigs in the
trenches of a World War I parody.

The goal is to play the original game natively on modern systems
(Windows 10/11 first, Linux later) with:

- **no compatibility hacks**: a new engine instead of the 2000 executable;
- **your own game files**: OpenHogs ships no original assets, it reads them
  from your Steam or GOG installation;
- **replaceable content**: language packs (e.g. Italian text and speech),
  music, sounds, textures and maps that you can **switch on and off** from a
  single config file, without ever modifying the original installation;
- **improvements** over the original, added as optional features.

> OpenHogs is a fan project, not affiliated with the rights holders of
> *Hogs of War*. You need a legal copy of the game to use it.

## Project status

OpenHogs is in early development. The game starts, loads a map and puts
the hogs on it; teams take turns, and the active hog runs, jumps, swims,
picks one of its weapons, aims and fires: fists and blades, pistols,
rifles and machine guns, grenades, mortars and bazookas, TNT and mines.
Hogs get hurt, thrown about and killed, and the last team standing wins.
A main menu leads to the level choice and the volume options, and the
match has its HUD and pause menu, all in the game's own fonts and words
(so in Italian with the Italian pack). Any team can be played by the
computer, which picks its target and weapon, gets closer when it has to,
aims and fires like a player would. The one-player campaign takes a
squad of eight hogs through the 25 missions of Saustralasia, with the
game's briefings, promotions along four careers and automatic saving.
Besides that, the original data can be
extracted, every map explored in 3D, an Italian (or French, German,
Spanish) language pack built from your PlayStation disc, every sound,
voice and music track played, and the hogs watched in all their
animations.

| Block | Status | What it delivers |
|-------|--------|------------------|
| `project-foundation` | ✅ done | Build system, core library, content-pack system, unit tests |
| `asset-extractor`    | ✅ done | Reads archives, textures and text; `openhogs-extract` tool |
| `ci-build`           | ✅ done | Automatic build and tests on Windows and Linux |
| `map-viewer`         | ✅ done | Reads terrain, models and objects; `openhogs-mapviewer` 3D viewer |
| `italian-pack`       | ✅ done | Italian text, menus and briefings from the PlayStation disc; `openhogs-langpack`, `openhogs-text` |
| `audio`              | ✅ done | Plays effects, speech and music with separate volumes; `openhogs-soundtest` |
| `characters`         | ✅ done | Hog skeleton, 93 animations, models of every class and team; `openhogs-characterviewer` |
| `character-expressions` | ✅ done | Faces that follow what the hog is doing, blinking |
| `game-framework`     | ✅ done | The `openhogs` game: fixed-rate game loop, hogs placed on the map, chase camera, ambient sound |
| `hog-movement`       | ✅ done | Turns (time limit, movement allowance), running, jumping, falling, swimming, collisions with the scenery, footsteps |
| `weapons`            | ✅ done | 20 weapons (melee, bullets, grenades, shells, TNT, mines), health, explosions, knockback, fall damage, death, victory |
| `menus-and-hud`      | ✅ done | Main menu, level choice, volume options, pause menu, HUD (team, class, health, weapon icon, time, movement, power), match messages; the game's bitmap fonts with accented letters |
| `ai`                 | ✅ done | Computer opponents: shot planning with the real flight rules, approach, aiming errors; ONE PLAYER skirmish against the computer, PLAYER/CPU choice per team in multiplayer |
| `campaign`           | ✅ done | The 25 missions in the original order with their maps, names and turn times; squad of eight, ranks and careers, promotion points, losses; briefings; mission results; saving |
| `local-multiplayer`  | 🔍 in review | Nation and PLAYER/CPU/OFF for every team, field conditions (hogs per team, turn time, health, sudden death), gamepads in menus and matches |
| gameplay blocks      | planned | Movement, weapons, turns, AI, campaign |

## Requirements

- A copy of **Hogs of War** from Steam or GOG (or a retail CD installation).
- Optional, for the Italian language: your own **European PlayStation disc**
  (SLES-01041) as a disc image (`.bin` or `.iso`).
- To build from source:
  - CMake 3.24 or newer
  - A C++20 compiler: Visual Studio 2022 or newer (Windows), GCC 12+ or Clang 15+
  - Git and an internet connection on first configure (CMake downloads
    SDL3, GLM and the test framework)
  - Linux only: the X11/Wayland development packages needed by SDL3, e.g. on
    Ubuntu/Debian:
    `sudo apt install libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxtst-dev libxfixes-dev libxkbcommon-dev libwayland-dev libegl-dev libgl-dev`
- To run the map viewer: a graphics card with OpenGL 3.3 (any GPU from the
  last 15 years).

## Installation

There is no release yet, so OpenHogs has to be built from source.

### 1. Get the source

```sh
git clone https://github.com/GianlucaLutero/OpenHogs.git
cd OpenHogs
```

### 2. Build

**Windows (Visual Studio 2022 or newer)**: open the folder with *File → Open → Folder*
and Visual Studio configures CMake automatically. Or, from a command
prompt (the Visual Studio generator finds the compiler by itself):

```sh
cmake -S . -B build
cmake --build build --config Release
```

If CMake answers *"CMake 3.24 or higher is required"*, an older CMake
installed by some other program comes first in your `PATH`. Visual Studio
ships a recent CMake: open the **Developer PowerShell for VS 2022** from the
Start menu and run the commands there, or call Visual Studio's CMake
directly from any PowerShell:

```powershell
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
$cmake = "$vs\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -S . -B build
& $cmake --build build --config Release
```

**Linux**

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The first build takes a few minutes because SDL3 is compiled from source.
To build only the command-line tools, without SDL3 and the viewer, add
`-DOPENHOGS_BUILD_VIEWER=OFF` to the first command.

### 3. Run the tests (optional)

```sh
ctest --test-dir build --output-on-failure -C Release
```

Every pull request is also built and tested automatically on
Windows (Visual Studio), with warnings in OpenHogs code treated as errors
(the Linux jobs with GCC and Clang are temporarily switched off). See `.github/workflows/build.yml`.

### 4. Configure

Copy `config/openhogs.default.ini` to `config/openhogs.ini` and set the
folder of your installation:

```ini
[Game]
Directory = C:\Program Files (x86)\Steam\steamapps\common\HogsofWar
```

## Usage

### Starting the game

`openhogs` is the game itself. It opens on the **main menu**:

- **ONE PLAYER** → **NEW GAME** → **SELECT TEAM**: start the campaign for
  one of the six nations (see below); **LOAD GAME** continues it;
  **DEATH MATCH** → pick a map: you play the team that starts, the
  computer plays all the others;
- **MULTI-PLAYER** → pick a map → **PLAYERS**: two rows per team of the
  map: its nation, and who plays it, *PLAYER* (a person at this PC),
  *CPU* (the computer) or *OFF* (left out), changed with left/right; at
  least two teams must play. **FIELD CONDITIONS** sets the number of
  hogs per team, the turn time (10–99 s), starting health (normal, half,
  double) and sudden death (every hog left with one point of health when
  round 10, 20 or 30 begins). Then **START GAME**;
- **OPTIONS** → **VOLUME CONTROL**: master, effects, voices and music
  volume (for this session; the starting values come from
  `config/openhogs.ini`);
- **QUIT APPLICATION** asks for confirmation.

In the menus the arrow keys (or `W` `A` `S` `D`) move and change the
sliders and choices, `Enter` picks and `Esc` goes back.

In a match the hogs stand where the map places them, each in its team's
uniform, and the teams take turns: each turn one hog moves, then fires one
of its weapons. The turn ends when the shot is over (or when time runs
out, or when you press Enter); the match ends when only one team has hogs
left, and `Enter` then goes back to the menu.

**The campaign.** Your squad has eight hogs; the five marked `*` on the
squad page go on the next mission (three in mission 1), the others wait
in reserve. Pick a hog to **PROMOTE** it (a grunt first chooses one of
four careers: heavy weapons, medic, espionage, engineer; every career
leads to Commando and Hero) or to **SWAP POSITION** with another. A
mission earns one promotion point, one more if nobody died, five more for
conquering an island; a promotion costs its rank's level (1 to 5). Losing
three hogs in a mission costs the first to die for good (four cost two),
losing all of them fails the mission. **START MISSION** shows the game's
briefing (Enter to go, Esc to go back), then the battle: defeat the enemy
to complete it. The results page offers **RETRY** after a defeat. The
campaign is saved automatically in `saves/campaign.ini`.

Teams played by people share the keyboard (hot seat). During a computer
team's turn ("THE ENEMY RETALIATES...") the hog keys do nothing; watch it
think, turn, pick a weapon, aim and fire. The HUD shows the team and class of the active hog and
its health (top left), the seconds left in the turn (top right), the
selected weapon with its icon and the movement left (bottom left; the
team name says *CPU* for a computer team) and,
while charging a throw, its power. "GET READY ..." announces each turn
and "VICTORY TO ...!!" the winner. Health bars float above the hogs. All
texts come from the game's own tables, so with the Italian pack they are
in Italian.

`--map` skips the menus and starts a match on that map directly:

```sh
# Windows
build\bin\Release\openhogs.exe --map ESTU

# Linux
./build/bin/openhogs --map ESTU
```

| Option | Effect |
|--------|--------|
| `--map <name>` | skip the menus and play this map; `--list-maps` prints the maps |
| `--hog <n>` | hog to start on (1 = the first hog of the map) |
| `--computer others` / `--computer all` | with `--map`: the computer plays every team but the first, or every team (to watch a match) |
| `--no-sound` | play without sound |
| `--screenshot <file>` | save the match (or, without `--map`, the menu) as an image and exit; `--seconds <s>` lets it run for a while first, `--auto-walk` makes the active hog run meanwhile, `--weapon <n>` and `--fire-at <s>` make it pick its n-th weapon and fire, `--pause` shows the pause menu, `--menu-keys down,enter` presses menu keys first |
| `--game <folder>` / `--config <file>` / `--size <W>x<H>` | as for the other programs |

| Key | Action |
|-----|--------|
| `W` / `↑` | run forward |
| `S` / `↓` | walk backwards |
| `A` / `←`, `D` / `→` | turn left / right |
| `Space` | jump (hold `W` to jump forward) |
| `Q` / `E` | previous / next weapon |
| `R` / `F` | aim up / down (the white dots show where the shot goes) |
| `Ctrl` or `X` | fire; for grenades, mortars and bazookas hold it to throw further and release to fire |
| `Tab` / `Shift+Tab` | another hog of the same team (only before moving) |
| `Enter` | end the turn (after the match: back to the menu) |
| left mouse button + drag | turn the camera around the hog |
| mouse wheel | zoom |
| `Esc` or `P` | pause menu: CONTINUE, or ABORT SKIRMISH (asks for confirmation) |
| `F12` | save a screenshot (`openhogs-<MAP>.png`) |

**Gamepads** (any controller SDL knows, with Xbox button names) work in
the menus and in matches alongside the keyboard. The players share the
PC: whoever's turn it is plays with the keyboard or any pad.

| Pad | Menus | Match |
|-----|-------|-------|
| D-pad / left stick | move, change | run, walk back, turn |
| A | pick | jump |
| B | back | – |
| X or right trigger | – | fire (hold to throw further) |
| right stick | – | up/down aim, left/right camera |
| LB / RB | – | previous / next weapon |
| Y | – | end the turn |
| Back | – | another hog (before moving) |
| Start | pick | pause menu |

Without the menus' choices, teams wear the nations' uniforms in a fixed
order (team 1 British, team 2 American, team 3 French...).
A turn lasts 60 seconds and allows about 7 seconds of running. These values,
and the other choices still to be compared with the original game, are
listed in [`docs/open-decisions.md`](docs/open-decisions.md).
The weapons, their names, models and sounds are described in
[`docs/formats/weapons.md`](docs/formats/weapons.md), the fonts and menu
pictures in [`docs/formats/fonts.md`](docs/formats/fonts.md), the campaign
in [`docs/formats/campaign.md`](docs/formats/campaign.md).

### Extracting the game data

`openhogs-extract` unpacks the original data into ordinary files you can
open with any image viewer or text editor. Your installation is only read,
never modified.

```sh
# Windows (the executable is in build\bin\Release or build\bin)
openhogs-extract "C:\Program Files (x86)\Steam\steamapps\common\HogsofWar" extracted

# Linux
./build/bin/openhogs-extract ~/Games/HogsofWar extracted
```

Result (all names lower case):

| Original | Extracted |
|----------|-----------|
| `Maps/CAMP.MAD` (archive) | folder `maps/camp.mad/` with every file inside |
| `.TIM` textures | `.png` images next to the original |
| `Language/Text/gtext.ofs` + `.bin` | `language/text/gtext.txt`, one `[index] TEXT` line per string |

On the Steam release this produces about 17,800 files and 9,300 PNG
textures (around 130 MB). A summary is printed at the end; the only
expected warning is `chars/mcap.mad: not a package archive` (that file
holds animation data, not an archive).

To look inside a single archive without extracting:

```sh
openhogs-extract --list "C:\...\HogsofWar\Chars\WEAPONS.MTD"
```

Do not share the extracted files: they are copyrighted game data.

### Italian language (from your PlayStation disc)

The PC release is English only. The European PlayStation release
(SLES-01041) has the game in English, French, German, **Italian** and
Spanish text, with English voices. `openhogs-langpack` takes the
translated text and mission briefings from **your own** disc image and
builds a content pack in the PC layout. The disc and the PC installation
are only read.

1. **Make an image of your disc** with any CD dumping tool. Only the first
   track (the data track, the largest `.bin` file) is needed; a plain
   `.iso` works too. Keep it outside the repository or in a folder named
   `HogsOfWar/`, which git ignores.

2. **Build the pack** from the OpenHogs folder:

   ```sh
   # Windows
   build\bin\Release\openhogs-langpack.exe --disc "D:\HogsOfWar\Hogs of War (Europe) (Track 1).bin" --game "C:\Program Files (x86)\Steam\steamapps\common\HogsofWar"

   # Linux
   ./build/bin/openhogs-langpack --disc ~/HogsOfWar/track1.bin --game ~/Games/HogsofWar
   ```

   This creates `content/packs/lang-it/` and prints a report:

   ```
   Game text: 258 of 272 strings from the disc, 14 from the overrides, 0 left in English
   Menu text: 751 of 786 strings from the disc, 13 from the overrides, 22 left in English
   Briefings: 31
   ```

   Strings the PlayStation version does not have (e.g. tutorial hints that
   name PC keys instead of pad buttons) come from
   [`content/translations/it.ini`](content/translations/it.ini), written for
   OpenHogs. What stays in English is mostly people's names in the credits.
   Use `--language fr`, `de` or `es` for the other languages (they have no
   hand-written overrides yet).

3. **Select it** in `config/openhogs.ini`:

   ```ini
   [Content]
   Language = it
   ```

   Set `Language = en` to go back to English at any time.

The pack is used by every OpenHogs program that reads game files; the
menus and briefing screens that show it are part of the next blocks. To
read or edit the text, convert a table with `openhogs-text`:

```sh
openhogs-text export content/packs/lang-it/data/Language/Text/gtext gtext-it.txt
openhogs-text import gtext-it.txt content/packs/lang-it/data/Language/Text/gtext
```

The listing has one `[index] TEXT` line per string, in UTF-8 with accented
capitals (À, È, Ì…). **Do not share the generated pack**: it contains data
from your disc. Improvements to `content/translations/it.ini` are welcome,
since that file is original text.

Not converted yet: newspapers, cards and title screens (their `.MGL`
picture format is still unknown, see
[`docs/formats/psx-disc.md`](docs/formats/psx-disc.md)).

### Exploring the maps in 3D

`openhogs-mapviewer` shows any map of the game: terrain, textures, trees,
buildings and the other objects, plus yellow markers on the hogs' start
positions. Run it from the OpenHogs folder after the configuration step:

```sh
# Windows
build\bin\Release\openhogs-mapviewer.exe CAMP

# Linux
./build/bin/openhogs-mapviewer CAMP
```

Without a map name the first map is opened. Useful options:

| Option | Effect |
|--------|--------|
| `--list` | print the names of all maps (about 70) |
| `--game <folder>` | read this game folder directly, ignoring `config/openhogs.ini` and content packs |
| `--config <file>` | use another configuration file |
| `--size 1920x1080` | window size |
| `--screenshot camp.png` | save the map as an image and exit, without opening a window |
| `--camera x,y,z,yaw,pitch` | start from a given view (the viewer prints the current one when a map loads) |

Controls:

| Key | Action |
|-----|--------|
| `W` `A` `S` `D` | move forward, left, back, right |
| `Q` / `E` | move down / up |
| `Shift` | move faster |
| right mouse button + drag, or arrow keys | look around |
| `Page Up` / `Page Down` | previous / next map |
| `O` | show / hide objects |
| `M` | show / hide the start-position markers |
| `F` | view the whole map again |
| `F12` | save a screenshot (`openhogs-<MAP>.png`) |
| `Esc` | quit |

Because the viewer reads files through the content system, a texture or
map pack enabled in `config/openhogs.ini` shows up in the viewer too.

### Watching the hogs

`openhogs-characterviewer` shows one hog, animated, on a ground grid: any
class, team uniform, level of detail and any of the 93 animations. The
hog's face changes with what it is doing (happy when celebrating, angry
when aiming, shocked when falling...) and it blinks.

```sh
# Windows
build\bin\Release\openhogs-characterviewer.exe --class ace --team american

# Linux
./build/bin/openhogs-characterviewer --class sniper --animation 44
```

| Option | Effect |
|--------|--------|
| `--list` | print the classes, teams and animations (with their names from the game) |
| `--class <id>` | `ace`, `grunt`, `heavy`, `legend`, `medic`, `sapper`, `saboteur`, `sniper`, `spy` |
| `--team <name>` | `british`, `american`, `french`, `german`, `japanese`, `russian`, `teamlard` |
| `--detail <level>` | `high`, `medium` or `low` |
| `--animation <n>` / `--frame <n>` | animation and frame to start from |
| `--fps <n>` | playback speed (default 25 frames per second) |
| `--expression <name>` | force a face: `neutral`, `happy`, `sad`, `shocked`, `angry`, `eyes-closed`, `dying`, `gormless` (default `auto`: follows the animation) |
| `--no-blink` | do not blink |
| `--view <yaw,pitch>` | camera angles around the hog (-90 = from the front) |
| `--distance <units>` | camera distance (default 650) |
| `--screenshot <file>` | save the chosen frame as an image and exit |
| `--game <folder>` / `--config <file>` / `--size <W>x<H>` | as for the map viewer |

Controls:

| Key | Action |
|-----|--------|
| `←` / `→` | previous / next animation |
| `↑` / `↓` | previous / next class |
| `T` | next team |
| `L` | next level of detail |
| `E` | next face (automatic, then each expression by hand) |
| `B` | blinking on / off |
| `Space` | pause / play |
| `,` / `.` | one frame back / forward |
| `-` / `+` | slower / faster |
| left mouse button + drag | orbit around the hog |
| mouse wheel | zoom |
| `F12` | save a screenshot |
| `Esc` | quit |

How the skeleton, animations and expressions work is described in
[`docs/formats/characters.md`](docs/formats/characters.md); content packs
can change which face goes with which animation (see
[`content/README.md`](content/README.md)).

### Listening to the game's sounds

`openhogs-soundtest` plays any sound of the game, alone or several at
once, like the sound test of old console games. It is also the quickest
way to check a pack that replaces sounds or music.

```sh
# Windows
build\bin\Release\openhogs-soundtest.exe --music 2
build\bin\Release\openhogs-soundtest.exe --list-banks

# Linux
./build/bin/openhogs-soundtest --play Speech/Sku1/Sarge/SGEN0101.wav
./build/bin/openhogs-soundtest --bank Audio/sfxday.srl --sound 2
```

| Option | Effect |
|--------|--------|
| `--play <file>` | play a sound file (`.wav` or `.ogg`), path relative to the game folder; repeat to play several at once |
| `--bank <file.srl> --sound <n>` | play sound number `n` of a sound bank (see `--list-banks`) |
| `--music <n>` | play music track `n` (2 to 32) |
| `--list-banks` | list the sound banks and their numbered sounds |
| `--list-music` | list the music tracks |
| `--seconds <s>` | stop after `s` seconds (default: when every sound has ended) |
| `--render <out.wav>` | write the mix to a WAV file instead of playing it |
| `--game <folder>` / `--config <file>` | as for the map viewer |

Volumes come from the `[Audio]` section of the configuration (see below).
The audio files are described in
[`docs/formats/audio.md`](docs/formats/audio.md).

### Game settings

The engine will use the configuration described below; the content-pack
system and the audio settings are already in place.

### Choosing the language

```ini
[Content]
Language = it
```

`en` uses the original English files. Any other language needs a language
pack in `content/packs/` with the same `Language` code.

### Volume

```ini
[Audio]
MasterVolume = 100
EffectsVolume = 100
SpeechVolume = 80
MusicVolume = 60
```

Each value goes from 0 (silent) to 100 (as recorded). A sound's final
volume is its category volume multiplied by the master volume.

### Switching content packs on and off

```ini
[Content]
EnabledPacks = hd-music, ps1-difficulty
```

List the Ids of the packs you want; remove an Id to switch that pack off.
The original game files come back automatically. See
[`content/README.md`](content/README.md) to create your own pack.

## Project layout

```
OpenHogs/
├── CMakeLists.txt        top-level build script
├── cmake/                CMake helpers
├── config/               user configuration (openhogs.default.ini)
├── content/              replaceable content: packs, languages (see content/README.md)
│   ├── packs/_template/  starting point for a new pack
│   └── translations/     hand-written translations used by openhogs-langpack
├── docs/                 architecture notes
│   └── formats/          one page per original file format
├── src/
│   ├── core/             binary reading, INI files, virtual file system
│   ├── content/          config, content packs, pack selection
│   ├── formats/          parsers: archives, text, fonts, textures, models, maps, sounds, skeleton,
│   │                     animations, PlayStation disc
│   ├── audio/            sound effects, speech, music: decoding, mixing, sound card
│   ├── scene/            maps and posed hogs turned into triangles (no GPU code)
│   ├── game/             match state and rules: world, hogs, camera, fixed timestep
│   ├── ui/               menus, HUD and text layout logic (no drawing code)
│   ├── render/           OpenGL 3.3 renderer and camera
│   ├── tools/extractor/  openhogs-extract command-line tool
│   ├── tools/text/       openhogs-text: text tables <-> editable text
│   ├── tools/langpack/   openhogs-langpack: language packs from the PlayStation disc
│   ├── apps/game/        openhogs: the game
│   ├── apps/mapviewer/   openhogs-mapviewer application
│   ├── apps/common/      window, screenshots and game-file opening shared by the apps
│   ├── apps/characterviewer/ openhogs-characterviewer application
│   └── apps/soundtest/   openhogs-soundtest application
├── tests/                unit tests (see tests/README.md)
└── third_party/          code not written for OpenHogs (glad, stb_vorbis)
```

More details in [`docs/architecture.md`](docs/architecture.md).

## Contributing

Development happens in blocks: each block has its own branch, named after
the block, and is merged into `main` after review. Code is C++20, written
to be readable and commented in English. See [`CLAUDE.md`](CLAUDE.md) for
the full rules and [`tests/README.md`](tests/README.md) for the testing
guidelines.

## Credits

OpenHogs builds on the work of others. Thank you!

| Project | Authors | License | Used for |
|---------|---------|---------|----------|
| [OpenHoW](https://github.com/TalonBraveInfo/OpenHoW) | Mark Sowden (hogsy) and contributors | GPL-3.0-or-later | Its [file-format documentation](https://github.com/TalonBraveInfo/OpenHoW/tree/master/doc/file-formats) was the reference for the model (VTX, NO2, FAC) and map (PMG, POG, PTG) formats and confirmed the MAD/MTD layout. No code was copied; every layout was re-checked on the game data and some POG offsets were corrected (see `docs/formats/maps.md`). |
| [SDL3](https://github.com/libsdl-org/SDL) | Sam Lantinga and SDL contributors | zlib | Window, input and OpenGL context of the viewers, sound output of the sound test (downloaded at build time). |
| [GLM](https://github.com/g-truc/glm) | G-Truc Creation (Christophe Riccio) and contributors | MIT | Vector and matrix maths (downloaded at build time). |
| [glad](https://github.com/Dav1dde/glad) | David Herberth | generated code: WTFPL or CC0-1.0; `khrplatform.h`: Apache-2.0 (Khronos Group) | OpenGL 3.3 function loader, generated into `third_party/glad`. |
| [stb_vorbis](https://github.com/nothings/stb) | Sean Barrett and contributors | public domain (Unlicense) or MIT, at your choice | Decodes the Ogg Vorbis music. Copied unmodified into `third_party/stb_vorbis` (v1.22). |
| [Hogs of War walkthrough (SuperCheats)](https://www.supercheats.com/playstation/walkthroughs/hogsofwar-walkthrough02.txt) | its anonymous author | all rights reserved (consulted, not copied) | Reference for the ranks' health and weapons and the order and names of the 25 missions, checked against the tables in the game's executable. |
| [Hogs of War (Wikipedia)](https://en.wikipedia.org/wiki/Hogs_of_War) | Wikipedia contributors | CC BY-SA 4.0 (consulted, not copied) | The islands and the medals for conquering a region. |
| [Catch2](https://github.com/catchorg/Catch2) | Catch2 authors (Martin Hořeňovský, Phil Nash and contributors) | BSL-1.0 | Unit test framework (downloaded at build time). |

*Hogs of War* was developed by Infogrames Sheffield House (formerly Gremlin
Interactive) and published by Infogrames in 2000. The PlayStation TIM image
format, the CD sector layout and the ISO 9660 file system (ECMA-119) are
publicly documented; OpenHogs' readers were written from that
documentation. The Italian overrides in `content/translations/it.ini` were
written for OpenHogs and follow the wording of the original Italian
translation. The campaign rules follow the game's own manual (`manual.pdf`
in the game folder).

## License

OpenHogs is released under the [GNU GPL v3](LICENSE).
*Hogs of War* and its assets belong to their respective owners and are not
part of this project.

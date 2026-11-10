# OpenHogs

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

OpenHogs is in early development. Nothing is playable yet, but the
original data can already be read and extracted.

| Block | Status | What it delivers |
|-------|--------|------------------|
| `project-foundation` | ✅ done | Build system, core library, content-pack system, unit tests |
| `asset-extractor`    | ✅ done | Reads archives, textures and text; `openhogs-extract` tool |
| `map-viewer`         | ⏳ next | Reads models and maps and shows them in 3D |
| gameplay blocks      | planned | Movement, weapons, turns, AI, campaign |

## Requirements

- A copy of **Hogs of War** from Steam or GOG (or a retail CD installation).
- To build from source:
  - CMake 3.24 or newer
  - A C++20 compiler: Visual Studio 2022 (Windows), GCC 12+ or Clang 15+
  - Git and an internet connection on first configure (CMake downloads the
    test framework)

## Installation

There is no release yet, so OpenHogs has to be built from source.

### 1. Get the source

```sh
git clone https://github.com/GianlucaLutero/OpenHogs.git
cd OpenHogs
```

### 2. Build

**Windows (Visual Studio 2022)**: open the folder with *File → Open → Folder*
and Visual Studio configures CMake automatically. Or, from a
*Developer Command Prompt*:

```sh
cmake -S . -B build
cmake --build build --config Release
```

**Linux**

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 3. Run the tests (optional)

```sh
ctest --test-dir build --output-on-failure -C Release
```

### 4. Configure

Copy `config/openhogs.default.ini` to `config/openhogs.ini` and set the
folder of your installation:

```ini
[Game]
Directory = C:\Program Files (x86)\Steam\steamapps\common\HogsofWar
```

## Usage

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

### Game settings

The engine will use the configuration described below; the content-pack
system is already in place.

### Choosing the language

```ini
[Content]
Language = it
```

`en` uses the original English files. Any other language needs a language
pack in `content/packs/` with the same `Language` code.

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
│   └── packs/_template/  starting point for a new pack
├── docs/                 architecture notes
│   └── formats/          one page per original file format
├── src/
│   ├── core/             binary reading, INI files, virtual file system
│   ├── content/          config, content packs, pack selection
│   ├── formats/          parsers: archives, text tables, TIM textures
│   └── tools/extractor/  openhogs-extract command-line tool
└── tests/                unit tests (see tests/README.md)
```

More details in [`docs/architecture.md`](docs/architecture.md).

## Contributing

Development happens in blocks: each block has its own branch, named after
the block, and is merged into `main` after review. Code is C++20, written
to be readable and commented in English. See [`CLAUDE.md`](CLAUDE.md) for
the full rules and [`tests/README.md`](tests/README.md) for the testing
guidelines.

## License

OpenHogs is released under the [GNU GPL v3](LICENSE).
*Hogs of War* and its assets belong to their respective owners and are not
part of this project.

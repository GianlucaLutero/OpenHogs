# Architecture

OpenHogs is split into small modules. Each module is a static library with
a public interface in `src/<module>/include/openhogs/<module>/` and a private
implementation in `src/<module>/src/`. A module may only depend on modules
listed before it below.

| Module | Status | Responsibility |
|--------|--------|----------------|
| `core`    | done    | Shared building blocks: `BinaryReader`, `IniFile`, path helpers, `VirtualFileSystem`. No dependencies. |
| `content` | done    | Replaceable content: user config, content packs, language selection. Builds the virtual file system the rest of the game reads from. |
| `formats` | planned | Parsers for the original file formats (MAD/MTD archives, text tables, models, maps, textures). |
| `tools`   | planned | Command-line tools, starting with the asset extractor. |
| `engine`  | planned | Window, rendering, audio, input (SDL3 + OpenGL). |
| `game`    | planned | Game rules: turns, hogs, weapons, AI, campaign. |

## How files are found

No module opens game files directly. They ask the `VirtualFileSystem`
built by the `content` module:

```
config/openhogs.ini ──► ContentConfig
content/packs/*     ──► discoverPacks ──► selectActivePacks ──► buildFileSystem ──► VirtualFileSystem
original game folder ───────────────────────────────────────────┘
```

This gives three guarantees:

1. The original installation is read-only for OpenHogs.
2. Any file can be replaced by a pack, and the replacement can be switched
   off from the config.
3. File names are case-insensitive and accept `\` or `/`, as in the original
   Windows game.

## Original game layout (Steam / GOG release, v1.2b)

| Folder | Content | Language-dependent |
|--------|---------|--------------------|
| `Audio/`, `FESounds/` | sound effects (`.wav`, 16-bit mono 22 kHz) | no |
| `Chars/` | hogs, hats, weapons, sky models (`.mad`/`.mtd`) | no |
| `Language/Text/` | text tables (`.bin` + `.ofs`) | **yes** |
| `Language/Tims/` | briefing and debrief images | **yes** |
| `Language/Streams/` | FMV cut-scenes (`.bik`) | **yes** |
| `Maps/` | per map: `.pmg` terrain, `.pog` objects, `.ptg` textures, `.mad` models | no |
| `MUSIC/` | soundtrack (`.ogg`) | no |
| `Skys/` | sky domes | no |
| `Speech/Sku1/` | voice lines, one folder per character (`.wav`) | **yes** |

The language-dependent folders are exactly what a language pack replaces.

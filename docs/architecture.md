# Architecture

OpenHogs is split into small modules. Each module is a static library with
a public interface in `src/<module>/include/openhogs/<module>/` and a private
implementation in `src/<module>/src/`. A module may only depend on modules
listed before it below.

| Module | Status | Responsibility |
|--------|--------|----------------|
| `core`    | done    | Shared building blocks: `BinaryReader`, `IniFile`, path helpers, `VirtualFileSystem`. No dependencies. |
| `content` | done    | Replaceable content: user config, content packs, language selection. Builds the virtual file system the rest of the game reads from. |
| `formats` | done | Parsers for the original file formats: MAD/MTD archives, text tables and encoding (with accents), TIM textures, models (VTX/NO2/FAC), maps (PMG/POG/PTG), sounds (WAV, `.srl` banks), the hogs' skeleton (HIR) and animations (mcap), PlayStation disc images and raw pictures. See [formats/](formats/). |
| `tools/text` | done | `openhogs-text`: converts text tables to editable UTF-8 listings and back. |
| `tools/langpack` | done | `openhogs-langpack`: builds a language pack from the PlayStation disc (text alignment, overrides, briefing conversion). |
| `tools/extractor` | done | `openhogs-extract`: unpacks archives, converts textures to PNG and text tables to text. |
| `scene`   | done | Turns map data into triangles grouped by texture, places objects. Poses the hogs (`CharacterPose`: bone transforms, blending between frames) and loads them (`CharacterLibrary`: classes, teams, animation names); facial expressions and blinking (`CharacterExpressions`). Pure CPU code, unit tested. |
| `render`  | done | OpenGL 3.3 renderers for maps, characters (and any textured model) and effects (glad), RAII GPU objects, free camera. No window code: the application creates the context. |
| `apps/common` | done | Shared by the applications: opening the game files (`--game` or configuration + packs), the SDL3/OpenGL window, PNG screenshots. |
| `apps/mapviewer` | done | `openhogs-mapviewer`: input, map switching, screenshots. |
| `apps/characterviewer` | done | `openhogs-characterviewer`: one animated hog, class/team/detail/animation switching, orbit camera. |
| `audio`   | done | Sound effects, speech and music: `SoundLibrary` loads and caches WAV/Ogg files and `.srl` banks through the VFS, `Mixer` mixes any number of sounds at any rate with per-category volumes, `AudioDevice` (SDL3, built with the applications) feeds the mixer to the sound card. |
| `apps/soundtest` | done | `openhogs-soundtest`: plays or renders any sound of the game. |
| `game`    | in progress | Match state and rules, with no drawing, sound or window code. Done: `FixedTimestep` (simulation at 50 steps per second whatever the display rate), `TerrainHeightField`, `MapCollision` (terrain rules and the objects' collision boxes), `Hog` and `spawnHogs`, `AnimationPlayer` and `IdleAnimator`, `HogMovement`, `TurnSystem`, `Weapons` (the weapon table and loadouts), `Combat` (damage, death, explosions, bullets), `Projectiles`, `World` (turn phases: playing, resolving the shot, match over), `ChaseCamera`. Planned: AI, campaign. |
| `apps/game` | in progress | `openhogs`, the game: connects the `game` module to the window, the input, the renderers and the sound. |

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

## Open decisions

Values and behaviours still to be compared with the original game are
collected in [open-decisions.md](open-decisions.md), to be settled in the
final refinement phase.

## The game loop

`openhogs` runs three things each frame, in this order:

1. **Input**: the keys held down become a `game::MovementInput` for the
   active hog; other events change the camera or end the turn.
2. **Simulation**: `game::FixedTimestep` turns the real time since the
   last frame into a whole number of steps of 1/50 s; `World::update` runs
   once per step. The match therefore behaves the same at any frame rate,
   and a stall never makes it jump ahead.
   Each step returns its events (footsteps, shots, explosions, hogs hurt
   or killed, a new turn...), which the application turns into sounds.
3. **Drawing**: the map, each hog posed by its animation player (the
   active one with its weapon), the projectiles, then the effects
   (aiming line, tracers, explosions, health bars). Drawing only reads the
   world.

The `game` module does not know about OpenGL, SDL or the sound card, which
is what makes its rules unit-testable.

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

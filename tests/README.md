# Unit tests

The `tests/` module contains the unit tests of every OpenHogs module. It is
built with [Catch2 v3](https://github.com/catchorg/Catch2), which CMake
downloads automatically.

## Running

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

To run only one area, filter by tag, e.g. `build/bin/openhogs_tests "[content]"`.

## Rules for tests

We keep **only meaningful tests**. Each test must protect a behaviour that
a user, a pack author or another module relies on.

1. **Justify every test.** Each `TEST_CASE` starts with a short comment that
   says what behaviour it protects and why breaking it would matter.
2. **Test behaviour, not implementation.** Check what the code promises
   (e.g. "a pack file replaces the game file"), not how it does it.
3. **No trivial tests.** Do not test getters, the standard library, or code
   that has no decisions in it.
4. **Never use real game files.** Tests build the data they need (in memory or
   with `support/TempDirectory`) so they run anywhere, with no copyrighted
   assets. Format parsers are tested with small hand-made samples.
5. **One topic per test case**, named as a sentence describing the behaviour.

## Layout

```
tests/
├── support/    helpers shared by tests (TempDirectory, ByteBuilder, IsoBuilder)
├── core/       tests for src/core
├── content/    tests for src/content
├── formats/    tests for src/formats
├── game/       tests for src/game (GameTests, MovementTests, WeaponTests)
├── audio/      tests for src/audio
├── extractor/  tests for src/tools/extractor
├── langpack/   tests for src/tools/langpack
└── scene/      tests for src/scene
```

Each new module gets its own folder, mirroring `src/`.

## What is covered

| Area | Protected behaviour |
|------|---------------------|
| `BinaryReader` | little-endian decoding, signed values, no reads past the end, fixed-size name fields |
| `IniFile` | case-insensitive keys, Windows paths kept intact, errors with line numbers, strict numbers, lists, booleans |
| `PathUtils` | every spelling of a game path maps to the same lookup key |
| `VirtualFileSystem` | top layer wins, fallback to the game folder, case-insensitive overrides, merged listings |
| `ContentPack` | manifests read completely; invalid ids, types and language packs rejected |
| `ContentConfig` | relative paths resolved from the config folder, required game folder, normalised ids |
| `ContentManager` | packs on/off via `EnabledPacks`, language selection, priority order, discovery, original files restored when a pack is off |
| `PackageArchive` | entries and data read correctly, case-insensitive lookup, out-of-file entries rejected, non-archives detected |
| `GameText` | font-code mapping, accented capitals as UTF-8 (both directions, lower case accepted), lossless `{xx}` escapes for icons, undisplayable text rejected |
| `TextTable` | strings in index order, byte-identical re-encoding, offsets recomputed for longer translations, missing terminators rejected, readable listings round-trip and reject missing/duplicated lines |
| `PsxDisc` / `decodeRaw15` | files read from raw and ISO images (sub-folders, multi-sector files), path spellings, non-disc files rejected, 15-bit colour layout |
| `TimImage` | 4-bit nibble order and width, PS1 transparency, out-of-palette pixels and non-TIM data rejected |
| `PngWriter` | checksums against reference values, whole file against an independent encoder |
| `AssetExtractor` | end-to-end extraction, bad files reported without stopping, entry names cannot escape the output folder, repeated names kept |
| `Model` | triangle and quad layout (fields grouped by kind), optional normals, out-of-range vertex indices rejected |
| `MapTerrain` | blocks, row-major heights, tile surface and flag bits, wrong file size rejected |
| `MapObjects` | 94-byte records, trailing bytes ignored, team/script/flags at the offsets verified on the game data, truncated files rejected |
| `WaveFile` | 16-bit and 8-bit samples, stereo interleaving, unknown chunks skipped, compressed files rejected, written files read back identical |
| `SoundBank` | numbered sound files in order, missing or out-of-order entries rejected |
| `Mixer` | mono on both channels, rate conversion keeps duration and interpolates, sound/category/master volumes, looping, finished sounds removed, clamping, empty sounds refused |
| `OggDecoder` | rate, channels, length and content of a generated Ogg Vorbis file; invalid data rejected |
| `AudioSettings` | volumes as percentages, defaults, out-of-range values rejected |
| `SoundLibrary` | music track numbering and discovery, bank paths resolved case-insensitively, decoded sounds cached |
| `Skeleton` (HIR) | parent and offset of each bone, parents that do not come first rejected |
| `MotionCapture` | table without count, root positions, bone angles (fourth float skipped), animations outside the file or with partial frames rejected |
| `CharacterPose` | the game's rotation convention (transpose of Rz·Ry·Rx, arms down), rest pose, children following parents, blending between frames, the ±180° wrap blended the short way, looping, mismatched frames rejected |
| `buildPosedModelMesh` | each vertex moved by its own bone, unknown bones rejected |
| `CharacterLibrary` | class catalogue and model names, animation names, loading through the VFS in any letter case, team textures in archive order followed by the face textures found by name, expressions.ini from a pack, wrong skeleton rejected |
| `CharacterExpressions` | only eye and mouth textures swapped, blinking, the saboteur's balaclava faces, fallback to the default face, rules per animation and their overrides, blinking brief, regular and different per hog |
| `blendPoses` | halfway pose, bones kept rigid, mismatched poses rejected |
| `FixedTimestep` | same steps for the same time however it is cut into frames, stalls capped and their backlog dropped |
| `TerrainHeightField` | samples at their world position, heights equal to every drawn terrain triangle, edge height outside the map, tile lookup |
| `spawnHogs` / `Hog` | class from the object name (commando included), team, player flag, facing, feet on the ground, hogs on towers keep their height, model matrix agrees with the facing |
| `AnimationPlayer` | looping and one-shot playback, frames passed across the loop's end (footsteps), no restart when asked again, cross-faded changes |
| `IdleAnimator` | reproducible per seed, mostly standing, fidgets followed by standing, any other animation gives way to standing |
| `MapCollision` | boxes from the map's objects (hogs and invisible objects left out), boxes turn with their object, low boxes are floors, tall ones obstacles, high ones overhead |
| `HogMovement` | running speed and direction, footsteps per cycle, turning without movement left, gentle slopes followed and cliffs stopping, wall tiles not climbable, obstacles stopping with sliding, hogs blocking but able to step apart, jump height and length, falls with their height, swimming, map edge, footstep sound names |
| `TurnSystem` | teams in order starting with the player's, hogs in rotation, time limit, movement allowance, changing hog only before moving |
| `Weapons` | every weapon has the data its kind needs, unique text ids, class loadouts made of real weapons |
| `Combat` | fall damage by height, health never below zero, one death only, explosions falling off with distance and throwing hogs away, bullets stopping at the first hog, wall or ground |
| `Projectiles` | shells exploding on contact but not on their owner, grenades bouncing, settling and exploding on their fuse, water and map edge, mines arming and triggering, TNT fuse, the aiming arc landing where physics says |
| `World` (weapons) | charge, throw, no control while the shot resolves, next turn; bullets and bursts; melee only in front and within reach; victory when the last enemy dies; no instant victory with one team; cluster bomblets; chain reactions; weapon changes and moving while charging; fall damage |
| `World` | only the active hog follows the input, ending the turn hands control over, a map without hogs has no turns |
| `ChaseCamera` | looks at the target from its distance, glides independently of the step length, swings round the short way, stays above the terrain, zoom and pitch limits |
| `TextureTable` | equal-size image slices, uneven files rejected |
| `scene` | terrain rows go towards -Z, tile texture flip/rotation, two triangles per tile grouped by texture, model UV normalisation and quad split, object scale/yaw/Y-flip, objects without a model become markers |
| `langpack` | translations follow the English text to the PC indices, differently worded and missing strings keep the English, overrides apply and reject foreign indices, the shipped Italian overrides encode, bottom-up BMPs, the OS/2 BMP header, bilinear scaling, a complete pack built from a miniature disc, clear errors for unusable discs |

### Not covered by unit tests

The OpenGL renderers (`src/render`), the sound-card output (`AudioDevice`)
and the applications need a GPU, a window or speakers, so they are not unit
tested. They are kept thin on purpose:
all decisions about geometry and posing live in `src/scene`, which is tested. The
renderer is checked by rendering real maps and hogs (`openhogs-mapviewer
--screenshot`, `openhogs-characterviewer --screenshot`) and comparing with
the original game; the audio output by
rendering real sounds with `openhogs-soundtest --render` and checking
their length and level.

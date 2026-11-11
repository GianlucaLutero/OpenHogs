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
| `TextureTable` | equal-size image slices, uneven files rejected |
| `scene` | terrain rows go towards -Z, tile texture flip/rotation, two triangles per tile grouped by texture, model UV normalisation and quad split, object scale/yaw/Y-flip, objects without a model become markers |

| `langpack` | translations follow the English text to the PC indices, differently worded and missing strings keep the English, overrides apply and reject foreign indices, the shipped Italian overrides encode, bottom-up BMPs, the OS/2 BMP header, bilinear scaling, a complete pack built from a miniature disc, clear errors for unusable discs |

### Not covered by unit tests

The OpenGL renderer (`src/render`) and the viewer application need a GPU
and a window, so they are not unit tested. They are kept thin on purpose:
all decisions about geometry live in `src/scene`, which is tested. The
renderer is checked by rendering real maps (`openhogs-mapviewer
--screenshot`) and comparing with the original game.

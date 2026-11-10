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
├── support/    helpers shared by tests (TempDirectory)
├── core/       tests for src/core
└── content/    tests for src/content
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

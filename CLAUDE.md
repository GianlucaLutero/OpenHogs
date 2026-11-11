# OpenHogs – development rules

These rules were set by the project owner and apply to every change.

## Workflow

- Work is split into **blocks**. Each block gets its **own branch**, named after
  the block (e.g. `project-foundation`, `asset-extractor`, `map-viewer`).
- A block branch is merged into `main` only after the owner has validated it,
  so every block ends with a pull request against `main` (or against the
  previous block's branch when blocks are stacked).
- Decisions that do not block progress (a value to tune, an unclear
  behaviour of the original, a design choice) are not asked one by one:
  pick a reasonable default, implement it, and record it in
  `docs/open-decisions.md`. They are settled together in the final
  refinement phase, once a complete prototype exists.
- Never commit original game files (audio, textures, maps, executables...).
  The engine reads them from the user's own installation at runtime.

## Code

- Language: **C++20**, built with CMake.
- Code must be written to be **as readable as possible**: clear names, small
  functions, no clever tricks when a plain version exists.
- Comment code so that someone new can understand and modify it: every header
  explains what the component is for, every non-obvious decision explains *why*.
- Comments, documentation and the README are written in **English**.

## Structure

- The project is modular; each module lives in `src/<module>/` with a public
  `include/openhogs/<module>/` folder and a private `src/` folder.
- Everything that can be replaced by the user (audio, speech, text, textures,
  language packs...) belongs to the **content** module: data in `content/`,
  configuration in `config/`, loading logic in `src/content/`.

## Tests

- Unit tests live in the separate `tests/` module (Catch2).
- Only meaningful tests: each test case starts with a short comment that
  states **what behaviour it protects and why it matters**. Do not add tests
  that only restate the implementation or test the standard library.

## README

- Always keep `README.md` up to date, especially the description and the
  steps to install and use the mod.
- **Credit the authors** of any code, library or documentation that is reused
  or used as a reference, in the `Credits` section of the README: name, link,
  license and what it was used for. Reused code also keeps its original
  license header.

# Changelog

All notable changes of OpenHogs, newest first. Version numbers follow
the `project()` call in `CMakeLists.txt`.

## Unreleased — refinement

- **Controls**: the keys of a match can be changed on the CONTROLS screen,
  as in the original (the row blinks until a key is pressed), and are
  saved in `[Controls]` of `config/openhogs.ini`; `Esc` always pauses.
- **Camera**: it stays behind the active hog; the mouse wheel zooms.
- **Rifle sight**: the mouse turns the hog and aims while it is held; the
  pad's left trigger holds it.
- **Volumes** changed in VOLUME CONTROL are saved.

## 0.1.0 — first prototype release

The first packaged build: the whole game can be played from start to
finish with the files of a Hogs of War installation (Steam or GOG,
v1.2b). What is still OpenHogs' own guess rather than the original's
behaviour is listed in [`docs/open-decisions.md`](docs/open-decisions.md),
to be settled in the final refinement phase.

- **Game**: the 61 maps of the PC release; turns, movement, swimming and
  falling; all the weapons and actions of the original, including the
  rifle sight, gases, missiles, airstrikes, jetpack and special ops;
  buildings and vehicles to get into, drive and destroy; crates,
  mission objectives and destructible scenery.
- **Campaign**: the Boot Camp and the 25 missions in order, with the
  original's briefings, ranks, promotions, medals, losses and saving;
  enemies with the ranks their maps give them; the films between the
  islands.
- **Skirmish and multiplayer**: DEATH MATCH against the computer and
  local multiplayer with FIELD CONDITIONS; the computer opponent.
- **Faithful presentation**: the original's front end and menus, HUD,
  sounds, voices, music and Bink cutscenes, decoded from the game files.
- **Versions**: the PC's rules by default, the PlayStation's with
  `[Game] Version = ps1` (walking computer hogs, looping turn tunes, the
  hard mode).
- **Content**: content packs, and an Italian language pack built from the
  European PlayStation disc (`openhogs-langpack`).
- **Tools**: map viewer, character viewer, sound test, file extractor and
  text table tool.

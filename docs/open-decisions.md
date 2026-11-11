# Open decisions

Choices made during development that do not block progress, collected
here to be settled together in the final **refinement** phase, once a
complete prototype can be played and compared with the original game.

Each entry says what OpenHogs does now, why, and where to change it. When
a decision is settled, move it to the *Settled* list at the end with the
answer.

## Fidelity: how does the original do it?

| # | Area | Open question | What OpenHogs does now | Where |
|---|------|---------------|------------------------|-------|
| 1 | Characters | What does the commando (`CO_ME`, 13 in the maps) look like? The PC files have no commando model, only unused `COLO` textures. | Drawn with the saboteur's model. | `src/game/src/Hog.cpp` (`kCommandoStandIn`) |
| 2 | Maps | Multiplayer maps (e.g. `BOOM`) place several hogs of different classes on the same spot. Which ones does the game keep? Probably chosen by the game mode, maybe via the type id (POG offset 46) or the flags (offset 88). | All of them are created; overlapping hogs can step apart. | `spawnHogs` in `src/game/src/Hog.cpp` |
| 3 | Expressions | Which face does the original show in which situation? The code that picks it was not found. | Rules per animation based on the animation names; replaceable with `Chars/expressions.ini`. | `src/scene/src/CharacterExpressions.cpp` |
| 4 | Idle | How does the original choose idle animations and how often? | Standing cycle, 20% chance of a fidget (animations 28, 31, 34–36) at the end of each cycle. | `src/game/src/IdleAnimator.cpp` |
| 5 | Sound | Which ambient sound plays during a match, and does it depend on the map or time of day (`sfxday`/`sfxnight`)? | `AMB_1D.wav` (first sound of `sfxday.srl`) in a loop, at 60% volume. | `src/apps/game/Game.cpp` |
| 6 | Movement | Turning speed, backward walking speed, swimming speed. | Turning 150°/s; backwards 180 units/s (measured from the walk-back stride); swimming at half speed. Running (480 units/s) is measured from the run cycle's stride and is not in question. | `src/game/include/openhogs/game/HogMovement.h` |
| 7 | Movement | Jump height and length; does a standing jump go straight up? | Up at 900 units/s with gravity 2400 units/s² (about 170 units high); forward at running speed only if *forward* is held. | `HogMovement.h` |
| 8 | Movement | Which slopes can a hog climb? What exactly does the tiles' wall flag mean (and the "slip" byte next to it)? | Wall tiles cannot be walked up (only down); other slopes up to 2:1; steps up to 96 units. The slip byte is ignored. | `src/game/include/openhogs/game/MapCollision.h` |
| 9 | Movement | Is water deep (drowning) or shallow (swimming)? Where is the water surface? | Water tiles are swum through at half speed on the bottom of the terrain; no drowning; no jumping in water. | `HogMovement.cpp` |
| 10 | Turns | Turn time limit and movement allowance; do they depend on the class or on options? | 60 seconds and 3500 units (about 7 s of running) for every hog. | `TurnRules` in `src/game/include/openhogs/game/TurnSystem.h` |
| 11 | Turns | How is the hog of a turn chosen? Can the player change it, and until when? | Each team uses its hogs in rotation; the player may pick another hog of the team with Tab until the hog has moved. | `src/game/src/TurnSystem.cpp` |
| 12 | Turns | Which team starts? | The team of the hog marked as the player's in the map; otherwise the lowest team number. | `TurnSystem` constructor |
| 20 | Weapons | Damage, blast radius, range, speed, fuse and spread of every weapon. | OpenHogs' own values (e.g. rifle 25, grenade 40 within 400 units, 3 s fuse). | `src/game/src/Weapons.cpp` |
| 21 | Weapons | Which weapons does each class carry, and with how much ammunition? (In the original it depends on rank and on what the team bought.) | A fixed loadout per class (grunt: fist, knife, pistol, rifle, grenade...); unlimited ammunition. | `kLoadouts` in `Weapons.cpp` |
| 22 | Weapons | Weapons not implemented yet: cattle prod, super shotgun, flame thrower, guided and homing missiles, medicine dart, tranquiliser, roller and shrapnel grenades, gases, airburst, medicine ball, anti-personnel mine, super TNT, the shells (long range, mine, gas, fire rain, 1000 lbs, shock), jetpack, suicide, healing, pick pocket, hide, shockwave, special ops, airstrikes, vehicles. | Not available. | `docs/formats/weapons.md` |
| 23 | Health | Starting health of each class; does it grow with rank? | 100 for every hog. | `kMaxHealth` in `src/game/include/openhogs/game/Hog.h` |
| 24 | Turns | After firing, can the hog still move (retreat)? How long does the turn wait after the shot? | The turn ends 1.5 s after everything is still; no control after firing. | `kSettleSeconds` in `src/game/src/World.cpp` |
| 25 | Weapons | Can a hog fire while swimming or in the air? | No. | `World::handleAiming` |
| 26 | Weapons | Are belt and back weapons (grenades, launchers) moved into the hands when used? | Drawn where their model's bones place them (belt, back). | `docs/formats/weapons.md` |
| 27 | Combat | Does an explosion leave a crater in the terrain? | No. | – |
| 13 | Collisions | The tile flags and object boxes are used as found; the meaning of the 16-bit field after the box (1 for some bridge parts) and of the bytes at POG offsets 56–58 is unknown. Objects' X/Z tilts are ignored for collisions. | Boxes are upright and flat-topped; low boxes are floors, tall ones obstacles. | `src/game/src/MapCollision.cpp` |

## Design: what do we want?

| # | Area | Open question | What OpenHogs does now | Where |
|---|------|---------------|------------------------|-------|
| 14 | Teams | Which nation's uniform does each team wear? (Decided by the campaign and the multiplayer menus in the original.) | Teams take the nations in a fixed order: British, American, French, German, Japanese, Russian, Team Lard. | `teamNation` in `src/game/src/Hog.cpp` |
| 15 | Camera | How should the camera follow the hog: fixed behind it, or free? | It orbits freely with the mouse, swings round behind the hog while it runs or jumps and for 1.5 s after a change of hog. | `src/apps/game/Game.cpp`, `ChaseCamera` |
| 16 | Controls | Final key bindings (and gamepad support). | W/S or arrows to run/walk back, A/D or arrows to turn, Space to jump, Tab to change hog, Enter to end the turn. | `src/apps/game/Game.cpp` |
| 17 | Movement | Should a hog stop when it runs into an obstacle, or keep the run animation? | It stands (idle animation) as soon as a step is completely blocked; walking at an angle slides along the obstacle. | `HogMovement.cpp` |
| 18 | Damage | Fall damage: from which height, how much? | Falls up to 400 units are harmless, then 1 point per 10 units. | `src/game/include/openhogs/game/Combat.h` |
| 28 | Controls | Weapon keys. | Q/E change weapon, R/F aim up/down, Ctrl or X fire (hold to throw further). | `src/apps/game/Game.cpp` |
| 29 | Effects | Explosions, tracers and the aiming line are plain coloured shapes; the original's effect sprites are still to be found. | Yellow-to-red ball, yellow dots, white dots (always visible); health bars over the hogs. | `src/render/src/EffectsRenderer.cpp` |
| 30 | Sound | Sounds for hurt, death, a new turn and victory. | `P_OWW`, `P_MOAN`, `S_SELECT`, `P_WHOHO`. | `src/apps/game/Game.cpp` |
| 31 | Aiming | What should the aiming arc show when not charging? | The arc at full power; while charging, at the current power. | `World::aimPreview` |
| 19 | CI | Re-enable the Linux jobs (GCC, Clang) once their package installation step no longer hangs. | Only Windows (MSVC) runs on pull requests; GCC and Clang are run locally before each PR. | `.github/workflows/build.yml` |

## Settled

| # | Question | Answer |
|---|----------|--------|
| – | Animation speed | 25 frames per second, confirmed against the original game (Gianluca). |
| – | When does CI run? | On pull requests only, not on every push (Gianluca). |

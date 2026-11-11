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
| 23 | Health | Starting health of the maps' enemy hogs (by class or rank?). | 100 for every map hog; the campaign squad's hogs have their rank's health (50 for a grunt ... 150 for a hero). | `kMaxHealth` in `src/game/include/openhogs/game/Hog.h` |
| 24 | Turns | After firing, can the hog still move (retreat)? How long does the turn wait after the shot? | The turn ends 1.5 s after everything is still; no control after firing. | `kSettleSeconds` in `src/game/src/World.cpp` |
| 25 | Weapons | Can a hog fire while swimming or in the air? | No. | `World::handleAiming` |
| 26 | Weapons | Are belt and back weapons (grenades, launchers) moved into the hands when used? | Drawn where their model's bones place them (belt, back). | `docs/formats/weapons.md` |
| 27 | Combat | Does an explosion leave a crater in the terrain? | No. | – |
| 13 | Collisions | The tile flags and object boxes are used as found; the meaning of the 16-bit field after the box (1 for some bridge parts) and of the bytes at POG offsets 56–58 is unknown. Objects' X/Z tilts are ignored for collisions. | Boxes are upright and flat-topped; low boxes are floors, tall ones obstacles. | `src/game/src/MapCollision.cpp` |
| 32 | Menus | The original's menu backgrounds, title screens, newspapers and cards are the `.mgl` pictures of `FEBmps/FEBMP.MAD`, whose compression is only partly decoded (`docs/formats/fonts.md`). | The menus are drawn over the first map of the list, seen from a slowly circling camera and darkened. | `src/apps/game/MenuBackground.cpp` |
| 33 | Menus | What are the levels called, and which belong to multiplayer? gtext has names such as PLAY PEN or THE LAKE (entries 1–62), but how they map to the map files is not known yet. | The level list shows the map file names (BOOM, BRIDGE, CAMP...), every map of the game. | `ui::FrontEnd` |
| 34 | Fonts | How wide is a space? The font tables give it no size. | A third of the letter height (10 pixels in BIG, 4 in SMALL). | `kSpaceWidthDivisor` in `src/formats/src/BitmapFont.cpp` |
| 35 | Text | Does `>2N` mean a number with two digits ("MISSION 03")? | Yes: a zero-padded number of that width. | `ui::fillPlaceholders` |
| 36 | HUD | What did the original HUD look like (layout, colours, which information)? Its pictures are probably among the undecoded `.mgl` files. | OpenHogs' own layout: team, class and health top left, turn clock top right (red under 10 s), weapon icon and name with the movement bar bottom left, power bar above it while charging; team colours = the uniforms' colours (khaki, light blue, blue, grey, yellow, red, purple). | `Game::drawHud`, `teamColour` in `src/apps/game/Interface.cpp` |
| 37 | HUD | Which rank name does each class show? The game names ranks (GRUNT, GUNNER, BOMBARDIER, ACE...), OpenHogs has one model per class. | The rank the model stands for; the heavy weapons hog is a GUNNER. | `kGtClassNames` in `src/ui/src/UiStrings.cpp` |
| 38 | Match end | The original has a results screen (BATTLE RESULTS, ENEMY GRILLED, FRIENDS FRIED..., gtext 195–199). | "VICTORY TO ...!!" stays on screen; Enter goes back to the main menu. | `Game::handleKey` |
| 44 | AI | How does the original computer opponent choose its target, weapon and movement, and how accurate is it (the PS1 version has difficulty levels)? | It weighs every weapon against every enemy with the real flight rules and picks the most damage to enemies minus 1.5 × the damage to its own team (30 points extra for a kill); without a shot worth 8 points it runs towards the nearest enemy (jumping once if blocked), then plans again. One level: up to 2° off in direction and angle, 4 % in power. | `src/game/src/ComputerPlayer.cpp`, `AiSettings` |
| 47 | Campaign | Which nation is the enemy in each mission? The executable's mission tables do not say (it may depend on the player's nation). | Each island's five missions go through the five other nations in order (`characterTeams` order, starting again on each island); the last mission is against Team Lard; FINAL's five enemy teams are the five nations. | `Campaign::enemyNation` |
| 48 | Campaign | Missions have scripted objectives, pick-ups, bonus medals, reinforcements, buildings; Boot Camp is a scripted tutorial. | Every mission is won by defeating the enemy; no bonus medals; Boot Camp is not offered. | `docs/formats/campaign.md` |
| 49 | Campaign | What does a promotion cost? The manual says one point per rung, the help text speaks of "enough points"; walkthroughs say all medals are needed for a squad of heroes. | The new rank's level: 1 for a first career rank, 2, 3, 4 for Commando, 5 for Hero. | `promotionCost` in `src/game/src/Ranks.cpp` |
| 50 | Campaign | What happens to a squad place when a hog is lost for good? | A new grunt takes its place, with the next unused name of the nation (then PIG 1, PIG 2...). | `Campaign::recordMission` |
| 51 | Ranks | Ranks' health and weapons come from the manual and player guides; missing weapons (flame thrower, airburst, healing, poison gas, jetpack, airstrike...) and ammunition limits are left out. The PC files have one body model per career line: which model does each rank wear? | Gunner–Pyrotechnic: heavy; Orderly–Surgeon: medic; Scout, Sniper: sniper; Spy: spy; Sapper, Engineer: sapper; Saboteur and Commando: saboteur; Hero: legend. | `kRanks` in `src/game/src/Ranks.cpp` |
| 52 | Campaign | The turn-time table has 99 for Boot Camp and missions 1–2: no limit, or 99 seconds? | 99 seconds. | `kMissions` in `src/game/src/Campaign.cpp` |
| 56 | Multiplayer | The original's PIGS setting offers "-1 / NORMAL / +1" or numbers (fetext 314–329 are not labelled); its default "depends on the number of teams". Up to four teams in the original. | PIGS: NORMAL (as on the map) or 1 to 8 per team (fewer if the map has fewer); every team of the map can play (FINAL has six). | `FrontEnd` field conditions, `applyFieldConditions` |
| 57 | Multiplayer | Sudden death "at a turn": counted in turns or in rounds? What does it do besides health 1? | In rounds (every team has played once more): when round 10, 20 or 30 begins every hog is left with 1 health; banner SUDDEN DEATH and the big explosion sound (E_1000P). | `World::turnBegan` |

## Design: what do we want?

| # | Area | Open question | What OpenHogs does now | Where |
|---|------|---------------|------------------------|-------|
| 14 | Teams | Which nation's uniform does each team wear? (Decided by the campaign and the multiplayer menus in the original.) | Teams take the nations in a fixed order: British, American, French, German, Japanese, Russian, Team Lard. | `teamNation` in `src/game/src/Hog.cpp` |
| 15 | Camera | How should the camera follow the hog: fixed behind it, or free? | It orbits freely with the mouse, swings round behind the hog while it runs or jumps and for 1.5 s after a change of hog. | `src/apps/game/Game.cpp`, `ChaseCamera` |
| 16 | Controls | Final key bindings. | W/S or arrows to run/walk back, A/D or arrows to turn, Space to jump, Tab to change hog, Enter to end the turn, Esc or P for the pause menu; in menus arrows/WASD, Enter, Esc. | `src/apps/game/Game.cpp` |
| 17 | Movement | Should a hog stop when it runs into an obstacle, or keep the run animation? | It stands (idle animation) as soon as a step is completely blocked; walking at an angle slides along the obstacle. | `HogMovement.cpp` |
| 18 | Damage | Fall damage: from which height, how much? | Falls up to 400 units are harmless, then 1 point per 10 units. | `src/game/include/openhogs/game/Combat.h` |
| 28 | Controls | Weapon keys. | Q/E change weapon, R/F aim up/down, Ctrl or X fire (hold to throw further). | `src/apps/game/Game.cpp` |
| 29 | Effects | Explosions, tracers and the aiming line are plain coloured shapes; the original's effect sprites are still to be found. | Yellow-to-red ball, yellow dots, white dots (always visible); health bars over the hogs. | `src/render/src/EffectsRenderer.cpp` |
| 30 | Sound | Sounds for hurt, death, a new turn and victory. | `P_OWW`, `P_MOAN`, `S_SELECT`, `P_WHOHO`. | `src/apps/game/Game.cpp` |
| 31 | Aiming | What should the aiming arc show when not charging? | The arc at full power; while charging, at the current power. | `World::aimPreview` |
| 19 | CI | Re-enable the Linux jobs (GCC, Clang) once their package installation step no longer hangs. | Only Windows (MSVC) runs on pull requests; GCC and Clang are run locally before each PR. | `.github/workflows/build.yml` |
| 39 | Menus | The multiplayer setup (game modes such as DEATH MATCH or SURVIVAL, turn time, health, number of players) and what ONE PLAYER offers besides the campaign. | ONE PLAYER → DEATH MATCH is a skirmish on any level: the team that plays first is the player's, the others the computer's. MULTI-PLAYER: the map's own teams, each PLAYER (default) or CPU on the PLAYERS page. | `ui::FrontEnd` |
| 40 | Options | Should volume changes made in the menu be saved? | They last for the session; the starting values come from `[Audio]` in `config/openhogs.ini`. | `Application::handleFrontEndResult` |
| 41 | Sound | Menu sounds and menu music. | `click1` to move, `click5` to pick, `Indu006` to go back (FESounds bank); no music yet. | `src/apps/game/MenuControls.h` |
| 42 | Menus | Behaviour of the menu cursor and of confirmations. | The cursor wraps around and skips greyed-out entries; ARE YOU SURE? starts on NO, so a double Enter never quits or aborts. | `ui::Menu`, `ui::FrontEnd`, `ui::PauseMenu` |
| 43 | Match | Messages and pause. | "GET READY ..." for 2.5 s at each turn (fading out); the match pauses by itself when the window loses focus; the pause menu stops the turn clock and the camera. | `src/apps/game/Game.cpp` |
| 45 | AI | Explosives put down (TNT, mines) are never used by the computer: the hog cannot walk away after using them. Without any worthwhile shot it ends its turn rather than firing at random. | Not used / turn ended. | `ShotPlanner::step`, `ComputerPlayer::decide` |
| 46 | AI | Pace of the computer's turn. | It thinks for 1 s (planning one weapon and enemy per step meanwhile), then turns, picks the weapon and aims at the normal speeds; its random errors are seeded from the clock, so no two matches are the same. | `AiSettings`, `computerSettings()` in `src/apps/game/Game.cpp` |
| 53 | Campaign | Newspapers after missions (`Papers/`), the debriefing pictures (`Debrief/`) and island bonuses ("five medals for every region conquered", from the Wikipedia article). | No newspapers or debriefing pictures yet (a text results page instead); +5 points for each island's last mission. | `ui::CampaignMenu`, `kIslandPoints` |
| 54 | Campaign | Saving: the original has numbered `savearmy` files, SAVE TEAM / LOAD SQUAD and renaming hogs. | One campaign, saved automatically to `saves/campaign.ini` after every change; no renaming yet. | `CampaignSession` |
| 55 | Text | Words the game's tables do not have: "PROMOTION POINTS: n" and the end-of-campaign line. | English only (`UiStrings::promotionPoints`, `campaignComplete`). | `src/ui/include/openhogs/ui/UiStrings.h` |
| 58 | Multiplayer | Not implemented yet: deathmatch limit, SELECT PIG (choose the hog each turn), team ranks (CHANGE RANKS), renaming teams, the generated levels (SURVIVAL / DEATHMATCH, CUSTOM / RANDOM / CODE) and network play. | – | `docs/formats/campaign.md`, `FrontEnd` |
| 59 | Controls | Gamepad layout, and whether each human team should own a controller instead of sharing. | Xbox layout as in the README; any pad (or the keyboard) controls the team whose turn it is. | `src/apps/game/Gamepads.h` |
| 60 | Multiplayer | May two teams fight for the same nation? | Yes: each team's nation is chosen freely (same uniform, same name). | `FrontEnd::handlePlayers` |

## Settled

| # | Question | Answer |
|---|----------|--------|
| – | Animation speed | 25 frames per second, confirmed against the original game (Gianluca). |
| – | When does CI run? | On pull requests only, not on every push (Gianluca). |

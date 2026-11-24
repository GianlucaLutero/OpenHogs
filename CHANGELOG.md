# Changelog

All notable changes of OpenHogs, newest first. Version numbers follow
the `project()` call in `CMakeLists.txt`.

## Unreleased — refinement

- **Campaign map** before every mission but the Boot Camp, as the original
  (`Language/Tims/PigMap/`): the territories in their holders' colours,
  the island zooming out of its territory, flags and the arrows on the
  next mission. The enemy of a mission is the nation holding its
  territory: a new campaign shares them out at random (Team Lard holds the
  last), won ones turn to the player's colour; saves keep the map.
- **Video analysis** (phase 5): `docs/video/index.md` maps a full
  playthrough of the original (the Italian PlayStation release) mission by
  mission; `tools/video` has two browser scripts to skim a YouTube video
  through its preview pictures and to read chosen frames.
- **Controls**: the keys of a match can be changed on the CONTROLS screen,
  as in the original (the row blinks until a key is pressed), and are
  saved in `[Controls]` of `config/openhogs.ini`; `Esc` always pauses.
- **Camera**: it stays behind the active hog; the mouse wheel zooms.
- **Rifle sight**: the mouse turns the hog and aims while it is held; the
  pad's left trigger holds it.
- **Volumes** changed in VOLUME CONTROL are saved.
- **Label over the hog** as on the PlayStation: its name or rank, and its
  health after a heart; no more health bars.
- **FIELD CONDITIONS**: DEATHMATCH LIMIT (killed hogs come back; 2 points
  a kill, -1 a loss) and SELECT PIG (10 seconds to choose the hog).
- **Rules**: the PlayStation's by default (`[Game] Version = ps1`); two
  teams can no longer fight for the same nation; the battle awards keep
  the executable's slip that leaves team 1 out of the kill awards.
- **Swimming** as the wiki says: about 2 points a second, rising to 20 for
  a hog steered into the water in its own turn; drowned hogs explode.
- **Jetpack** as the wiki says: it flies along the aim; fire again drops
  it (it explodes where it lands) and opens the parachute; the turn goes
  on; flying into the ground is a crash landing.
- **Weapon menu** as the original (`C`): the brass panel with its four
  tabs, the weapons' icons and uses left.
- **Explosives** as the wiki says: TNT leaves 5 seconds to get away; the
  first mine of a turn keeps the turn and waits for the turn to end; the
  computer uses both. The mortar and the guided missile aim at 45 degrees
  or more, with the angle gauge's limit mark.
- **Radar** as the original: the map in the bottom-left corner, tilted
  and turned with the camera, with the hogs in their nation's colour and
  the crates.
- **Text box** as the original: crate messages and the Boot Camp's
  instructions come down from the top in a brass tube (long ones scroll);
  the teams' strength comes in the big tube at the start of each round.
- **Deaths** as the original: one of 17 dying animations, then the hog
  blows up and only its boots are left.
- **Crates** that appear during a match come down by parachute.
- **Crates, explosive drums and animals** can be destroyed; a drum hit hard
  enough blows up two seconds later. Walls and houses hold out a little
  longer below two thirds of their health, and a hog standing on an object
  that is destroyed takes the rest of the blow.
- **Effects** with the original pictures: explosions throw fire, smoke and
  earth (foam in water) and leave a scorch mark; hogs have shadows.
- **Level names** from the game's text in the level lists.
- **Sergeant**: a cheer or a groan after some turns, and reminders of the
  promotion points and crates still to get when one enemy is left.
- **Newspaper** after every won campaign mission, with the headline and
  photo the original chooses for the battle.
- **Debriefing page** as the original: the squad's faces (bandaged, or a
  gravestone for hogs lost for good), uniforms, names and rank badges, and
  the mission's medals.
- **Eight save slots** on the original's loading and saving board: LOAD
  GAME and SAVE TEAM, with the saved squad's badges and mission on the
  panel. An old `saves/campaign.ini` becomes slot 1.
- **Names**: a new team is named (PLEASE NAME YOUR TEAM) and hogs can be
  renamed (RENAME), on the original's letter grid or the keyboard; the
  NAUGHTY PIGS cheat works.
- **Sergeant** on the squad page: early in the campaign, with points to
  spend, he says to promote a hog.
- **Menu machines** as the original: the list menu goes back up before
  another screen, the pipes blow steam while the panels move, the dial
  clicks into place and the cogs rattle.
- **3D models in the menus**: the hog on CAREER PATH's pedestal, in the
  body of the career being chosen, and the debriefing's turning medals.

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

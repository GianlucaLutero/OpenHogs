# The campaign

Where the one-player campaign of Hogs of War is defined, and what
OpenHogs takes from each place. The rules themselves (squad, points,
losses) come from the game's manual (`manual.pdf` in the game folder) and
are summarised in `src/game/include/openhogs/game/Campaign.h`.

## Tables in `warhogs_.exe` (Steam v1.2b)

Offsets are file offsets in the executable.

### Level names: `0xD0790`

An array of 59 pointers to map names: index *n* is level *n*. The first
26 are the campaign's levels and Boot Camp, then the demo map, the
multiplayer arenas, two test maps and the generated-map types. The full
list is in `src/game/src/Levels.cpp`; the campaign part is:

| # | Map | # | Map | # | Map | # | Map |
|---|-----|---|-----|---|-----|---|-----|
| 0 | ROAD | 7 | GUNS | 14 | EYRIE | 21 | SUPLINE |
| 1 | TRENCH | 8 | OASIS | 15 | BRIDGE | 22 | TESTER |
| 2 | RUMBLE | 9 | MASHED | 16 | BAY | 23 | FOOT |
| 3 | DEVI | 10 | CAMP | 17 | DESVAL | 24 | FINAL |
| 4 | TWIN | 11 | LIBERATE | 18 | SNAKE | 25 | ESTU |
| 5 | ZULUS | 12 | MEDIX | 19 | EMPLACE | 26 | DEMO |
| 6 | SNIPER | 13 | FJORDS | 20 | KEEP | 27… | BOOM, BHILL, LECPROD, DVAL, ICE… |

The name of level *n* is entry **11 + *n*** of the game text table
`gtext` (11 ROUTINE PATROL = ROAD, 21 BOOT CAMP = CAMP, 31
ASSASSINATION = KEEP, 35 WELL, WELL, WELL! = FINAL, 36 THE WAR
FOUNDATION = ESTU).

> An earlier version of this page (and of OpenHogs) left KEEP out and
> shifted every later level by one, so mission 1 was DEMO and missions
> 21–25 had the wrong maps. The table above is read pointer by pointer.

### Mission order: `0xD05F0`

26 int32 level numbers, mission 0 (Boot Camp) to mission 25:

    10, 25, 0, 1, 3, 2, 5, 4, 6, 9, 7, 11, 8, 13, 14, 16, 12, 15, 17, 18, 19, 21, 20, 22, 23, 24

So mission 1 is ESTU (three player hogs, a short estuary map) and
mission 25 is FINAL (five enemy teams), mission 22 is KEEP
(ASSASSINATION).

### Turn times: `0xD0660`

26 int32 values, again mission 0 to 25, in seconds (99 seems to mean "no
hurry": Boot Camp and the first two missions):

    99, 99, 99, 60, 60, 60, 60, 45 (×11), 30 (×6), 15, 30

Mission 25's 30 seconds agree with the walkthroughs.

Other tables near these (ten 5s, a list 0 1 4 5 8 9 11 10 14 16, a curve
0 … 100) are not understood yet.

## Islands

From the manual and the achievement list: Hogshead 1–5, Saustralia
6–10, Trotsville 11–15, Bellyopolis 16–20, Arstria 21–24, Isle of Swill
25. Which enemy nation each mission is fought against is not stored in
these tables: it is the nation holding the mission's territory on the
campaign map (below).

## The campaign map

Before every campaign mission but the Boot Camp (level 10) the
executable shows the map of Saustralasia (`FUN_00482c30`, called just
before the briefing, `FUN_004821c0`). Its pictures are in
`Language/Tims/PigMap/`:

| File | What |
|------|------|
| `BigMap.bmp` | the map, 640 × 480 |
| `PMAPTIMS.MAD` | entries 0–24: the territories of missions 1–25 (`hog2`, `hog4`, `hog3`, `hog5`, `hog1`, `sau4` …, `ios`); 25–28 the arrows `ar1`–`ar4`; 29 `flag` (drawn 40 × 32); 30 `fpole` (8 × 62) |
| `LANGTIMS.MAD` | the six islands' names |
| `hogphy.mad` … `iosphy.mad` | one picture each: the islands Hogshead, Saustralia, Trotsville, Bellyopolis, Arstria, the Isle of Swill |

**Who holds what.** The team structure has a byte per territory at
+0x55 (`DAT_0051f17d` for the player's team). A new campaign
(`FUN_00482660` → `FUN_00482940`) draws a nation for each territory, five
at a time, never the player's and never one already drawn as often as
the group number: so each group of five goes to the five other nations
in a random order; territory 24 (mission 25) is then given to Team Lard
(6). Winning a mission gives its territory to the player
(`FUN_004829d0(team, mission − 1, nation)`, after the results page).
The nation bytes are the team structure's own order, mapped to the
models' order (British, American, French, German, Russian, Japanese,
Team Lard) by `FUN_004508e0` (0, 2, 1, 4, 5, 3, 6).

**Colours** (VA 0x4D34E0, 16 bytes each, in the models' order): British
(39, 170, 105), American (143, 172, 212), French (110, 125, 195), German
(130, 130, 130), Russian (214, 123, 133), Japanese (205, 187, 98), Team
Lard (158, 46, 158), nobody (109, 56, 32). The islands' names are tinted
(250, 240, 235).

**Positions** (in the 640 × 480 picture). An array of 0x88-byte records
at VA 0x4D3658 — a 128-byte name followed by x and y — gives the
territories' top-left corners (records 0–24), the islands' names (25–30)
and where each island's picture ends up (31–36); the pictures' sizes are
at VA 0x4D4A00 (250 × 322, 250 × 319, 250 × 250, 294 × 291, 150 × 326,
150 × 150) and each mission's place on its island's picture at VA
0x4D4AC0. All the numbers are in `src/ui/include/openhogs/ui/CampaignMap.h`.

**Steps.**

1. `FUN_00483980` builds the map: `BigMap.bmp`, every territory tinted
   with its holder's colour, the names. `FUN_00482cd0` shows it for
   two seconds (a key cuts it short), the next mission's territory
   white whenever bit 4 of the millisecond clock is set (a 16 ms
   flicker).
2. `FUN_00483010`, 32 ticks of 50 ms: the territory's rectangle grows
   into the island picture's along the curve at VA 0x4D4A40 (0, 1, 2, 3,
   5, 7 … 97, 100, held at 100), drawn three times: a black shadow at
   0.75 of the way with alpha 64 × progress, the territory in its
   holder's colour fading out, the island picture fading in.
3. `FUN_004833b0`, until ten seconds have passed or a key is pressed:
   the island picture in place; a flag (tinted with the holder's colour)
   and its pole on each of the island's missions, one every three ticks,
   at the mission's place − (8, 62); once all are up, the four arrows
   in the player's colour round the next mission — `ar2` at (−64 + b,
   −45), `ar3` at (−18, −91 + b), `ar1` at (16 − b, −45), `ar4` at (−18,
   −11 − b) — where b follows 0, 1, 2, 4, 7, 12 (VA 0x4D4B88) up and down,
   a step a tick. An island holds five missions, Arstria four (21–24) and
   the Isle of Swill one (25).

In the PlayStation version filmed in `docs/video/` the same steps show
(an inset of the island with its flags, one arrow); mission 1's enemy
wears the colour of the flag the arrow points at, which confirms that
the territory's holder is the mission's enemy.

## Boot Camp

The campaign starts with the Boot Camp: mission 0 in the executable's
order, level 10 (`CAMP`), 99-second turns, one hog. The executable plays
level 10 in a training mode (`DAT_005207a8 = 1` in `FUN_0041a320`; the
flag also decides which crates are hidden, see maps.md "Objectives"):

- the hog gets no class weapons (the loadout copy in `FUN_00466950` is
  skipped); crates give weapons that never run out and health crates 50
  points (`FUN_00465420`);
- the crates and dummies of the map form a chain of hidden groups: the
  first dummy (or any of the trees of group 1) reveals group 2 (a rifle
  crate and two dummies), whose dummies reveal group 3, and so on up to
  group 6 (a TNT crate, a health crate and the bridge); the wall
  `STW04_D2` (type 22) reveals group 7 (the bazooka crate in the
  building and its two dummies);
- the instructions are gtext 211-234, each with a line of the
  instructor (`Speech/Sku1/Train1/tr1_enNN.wav`, `FUN_0043b530`, NN =
  the line number): 211 / line 2 when the training starts
  (`FUN_0048fca0`); when a crate appears (`FUN_00465ab0`, by weapon:
  rifle 215/6, sniper rifle 219/10, 5 grenades 221/12, 10 grenades
  224/15, TNT 226/17, bazooka 231/22); when one is picked up
  (`FUN_00465cb0`: bayonet 212/3, rifle 217/8, sniper rifle 220/11,
  grenades 222/13 and 225/16, TNT 229/20, bazooka 234/25; health 10
  227/18, 15 228/19, 20 232/23, 25 233/24); when a crate weapon is chosen
  (`FUN_00493020`: bayonet 214/5, rifle 218/9, grenade 223/14).

How the executable decides the Boot Camp is over has not been read;
OpenHogs ends it once every crate of the chain is taken (decision 118).

## Medals and points

The mission results page (`FUN_004845e0`, picture
`Language/Tims/Debrief/pigbkpc1.bmp`) draws medals, and each medal is a
promotion point:

- LEVEL COMPLETE (gtext 190): one medal;
- SURVIVAL BONUS (191): one medal if no hog died, NONE (96) otherwise;
  in FINAL (level 24, the last mission) fewer than three deaths are
  enough;
- SPECIAL BONUS (192), only when the mission offers any: as many medals
  as the table at 0x4D3560 gives for the mission (indexed by mission,
  0 = Boot Camp: `0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 2 2 2 2 2 3 3 4 5 0`),
  filled for each promotion point the squad picked up (the hogs' counts
  at hog + 0x1DE, which objectives 4, 10 and 16 lower). BAY (level 16,
  mission 15) starts with one. The table matches the maps: mission 24
  (FOOT) has five hogs to lead to a point, mission 23 (TESTER) four
  legends to kill;
- after the page, missions 5, 10, 15, 20 and 25 (`mission > 1 &&
  mission % 5 == 0`) add five points, the island bonus, so mission 24,
  which ends Arstria, does not.

The points of a mission are the medals plus that bonus (`_DAT_0051f178`).

## Battle results

After a skirmish or a match between people, `FUN_00483cc0` shows BATTLE
RESULTS (195) on `debrief/multipl1.bmp` (`multipl2.bmp` for the version
with the PS1 frame): per team ENEMY GRILLED (196), FRIENDS FRIED (197),
VICTORY BONUS (198, 3 for the winner), SPECIAL AWARDS (199) and TOTAL
(200); the team with the best total WINS!! (201), none on a tie. Each
hog counts what happens in its turn (`FUN_00467ac0`, `FUN_00467db0`):
enemies killed (hog + 0x1D0), own hogs killed (0x1D1), damage dealt to
enemies (0x1C8) and the most enemies killed in one turn (0x1C6). One
SPECIAL AWARD (202) goes to a hog, the first that fits:

| Award (gtext) | For | Points |
|---------------|-----|--------|
| THE ANNIHLATOR!!! (203) | 4+ kills in a turn | 5 |
| TRIPLE RIP REAPER!!! (204) | 3 kills in a turn | 3 |
| MASTER SADIST!!! (205) | 351+ damage | 3 |
| DOUBLE DEATH DEALER! (206) | 2 kills in a turn | 2 |
| SADISM AWARD! (207) | 176-350 damage | 2 |
| PIG OF THE BATTLE (208) | any damage | 1 |

The code skips the kill and 351-damage awards when the best hog belongs
to the second team (it compares the team with 1, where -1 "nobody" seems
meant); OpenHogs copies that slip, as the owner chose (decision 116):
such a hog can still get SADISM AWARD or PIG OF THE BATTLE.

## The maps' teams

Every campaign map marks the player's hogs (POG flag bit 0): three in
ESTU (mission 1), five everywhere else. The enemy is one other team, except FINAL
(mission 25), which has five enemy teams. CAMP has a single hog.

The enemies' ranks come from the map too: a hog object's type field
(offset 46, see [maps.md](maps.md)) is its rank, so each mission's
enemies have the health and weapons of their rank (OpenHogs:
`game::deploySquad`, `rankFromMapType`).

**Hard mode** (PlayStation only; the Hogs of War Wiki): once the
campaign is won it starts again from mission 1 with the same squad, and
every enemy is one rank higher — grunts become gunners, snipers spies,
commandos heroes (the wiki notes a few missions where this does not
happen, mission 18's spies and mission 20). It ends with `fmv 09`, a
one-byte placeholder on PC. OpenHogs offers it with `[Game] Version =
ps1` (`game::GameVersion`, `Campaign::startHardMode`).

## Pictures

| File | Size | Use |
|------|------|-----|
| `Language/Tims/Briefing/level<N>.bmp` | 640 × 480 | the briefing of mission *N* (0 = Boot Camp) |
| `Language/Tims/Briefing/level1n<K>.bmp` | 254 × 190 | photo for mission 1's empty frame (at 350, 197): a hog in the player's uniform, *K* = British, American, French, German, Russian, Japanese |
| `Language/Tims/Briefing/loadbar.bmp` | | the loading bar under the briefing (not drawn by OpenHogs, which loads the map after the briefing) |
| `Language/Tims/Papers/<Nation>.bmp`, `text<N>.bmp`, `pic0<N>.bmp` | 640 × 480, 314 × 204, 300 × 299 | the newspaper after a won mission (see *Newspapers*) |
| `Language/Tims/Debrief/*` | | the debriefing page (see *Debriefing*) |
| `Language/Tims/PigMap/` | | the campaign map (see *The campaign map*) |

The language packs replace the briefings and newspapers with translated
ones, so OpenHogs shows them as they are.

## Newspapers

After every won campaign mission except the Boot Camp, the executable
(`FUN_0045c700`, called as `FUN_0045c3d0(3)`) prints a newspaper for up to
10 seconds or until a key is pressed. The paper is the nation's
(`British`, `American`, `French`, `German`, `Russian`, `Japan`; `teamlard`
for Team Lard), the story `text<N>.bmp` goes at (23, 144) and the photo
`pic0<N>.bmp` at (309, 111). Red is the see-through colour: (255, 0, 0)
in the photos, (252, 3, 15) in the stories (the same red once the pictures
are 4-bit-per-channel textures).

*lost* counts the first five squad slots whose state is not 2
(`FUN_00450b00`); *b* is the mission number:

| Hogs lost | Photo | Story | Headline |
|-----------|-------|-------|----------|
| 0, more than one medal this mission (`0x51F3C6`) | `pic01` | (*b* & 3) + 1 | VICTORY! NATION ECSTATIC |
| 0 | `pic02` | (*b* & 3) + 5 | ENEMY GIVEN A PROPER SEEING TO |
| 1–2 | `pic03` | (*b* & 3) + 9 | VICTORY ONCE MORE |
| 3 | `pic04` | (*b* & 3) + 13 | THEY DON'T LIKE IT UP 'EM |
| 4–5 | `pic05` | (*b* & 3) + 16 | OUR LADS ROGERED |

Unless all five were lost, the levels that end an island and the final
level (levels 2, 7, 16, 19, 23 and 24: RUMBLE, GUNS, BAY, EMPLACE, FOOT,
FINAL) print stories 21 to 26 instead (21 is HOGSHEAD FALLS!).

## Debriefing

The MISSION RESULTS page (`FUN_004845e0`) is drawn on `pigbkpc1.bmp` after
a victory (keys SPACE CONTINUE, ESCAPE RETRY) or `pigbkpc2.bmp` after a
defeat (SPACE RETRY, ESCAPE EDIT SQUAD). Text positions are 640 × 480
pixels, centred on x:

- the first five hogs of the squad in rows 73 pixels apart: a face
  (`Facepc<n>`, bandaged `facepcw<n>` for a hog that fell, `r_i_p` for one
  lost for good) over the nation's uniform (`unifeng`, `unifusa`,
  `uniffren`, `unifgerm`, `unifruss`, `unifjap`, `uniflard`) in the frame,
  the name at (215, 90 + 73 *row*) and the rank badges. Badges (table
  `0x4D35C8`, two per rank, ranks numbered as gtext 63 + *n*): the career
  (`pcHeavy`, `pcSniper` for espionage, `pcEngine`, `pcMedic`, `pcCmmndo`,
  `pcLegend` for the hero and legend) and `pcPip1` / `pcPip2` for the
  second and third rank of a career. `pcGrenad` and `pcParatr` are loaded
  but not in the table. Faces, uniforms and the gravestone are magenta-keyed,
  badges black (palette entry 0);
- at (320, 5) a title, at (456, 84) MISSION RESULTS (gtext 187); after a
  defeat MISSION FAILED! at (456, 226); after a victory LEVEL COMPLETE at
  (456, 140), SURVIVAL BONUS at (456, 225) and NONE at (456, 260) without
  the medal, SPECIAL BONUS at (456, 305) when the mission offers special
  medals (table `0x4D3560`);
- the medals are the promotion point model (`Chars/PROPOINT.MAD`,
  `propoint.mtd`) turning 25/4096 of a turn every 5 ms at (1950, *y*, 7000)
  with *y* = 700 (level complete), -500 (survival) and -1700 (special
  medals, 750 apart, the ones not picked up at 100/255 opacity).

## Texts

fetext: 24 SELECT TEAM, 50 START MISSION, 54 ONE PLAYER, 55 NEW GAME, 56
LOAD GAME, 71 PROMOTE, 72 SWAP POSITION, 90 CAREER PATH, 91–94 HEAVY
WEAPONS / ENGINEER / ESPIONAGE / MEDIC; each nation's block of a team name
and nine hog names (166 British, 176 French, 186 American, 196 Russian,
206 Japanese, 216 German, 226 Team Lard).

gtext: 11–62 level names, 63–79 ranks, 158 MISSION >2N : >S, 187 MISSION
RESULTS, 188 MISSION FAILED!, 189 MISSION ABORTED, 190 LEVEL COMPLETE, 191
SURVIVAL BONUS, 192 SPECIAL BONUS, 193 RETRY, 194 EDIT SQUAD, 195–208
battle results and special awards, 271 RIP.

## Save files

The original keeps squads in `savearmy<N>` (680 bytes, not decoded).
OpenHogs saves its campaign as a small INI file, `saves/campaign.ini`
next to the `config` folder: nation, next mission, points, names, and
each hog's name and rank.

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
these tables (OpenHogs' choice: `docs/open-decisions.md`).

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
meant); OpenHogs does not copy that (decision 116).

## The maps' teams

Every campaign map marks the player's hogs (POG flag bit 0): three in
ESTU (mission 1), five everywhere else. The enemy is one other team, except FINAL
(mission 25), which has five enemy teams. CAMP has a single hog.

## Pictures

| File | Size | Use |
|------|------|-----|
| `Language/Tims/Briefing/level<N>.bmp` | 640 × 480 | the briefing of mission *N* (0 = Boot Camp) |
| `Language/Tims/Briefing/level1n<K>.bmp` | 254 × 190 | photo for mission 1's empty frame (at 350, 197): a hog in the player's uniform, *K* = British, American, French, German, Russian, Japanese |
| `Language/Tims/Briefing/loadbar.bmp` | | loading bar |
| `Language/Tims/Papers/<Nation>.bmp`, `text<N>.bmp`, `pic0<N>.bmp` | 640 × 480, 314 × 204 | the newspaper after a mission: a nation's front page with a headline and a photo (not shown yet) |
| `Language/Tims/Debrief/*` | | mission results pages: backgrounds, hog faces, rank badges (not used yet) |
| `Language/Tims/PigMap/` | | the campaign map (empty in the Steam release) |

The language packs replace the briefings and newspapers with translated
ones, so OpenHogs shows them as they are.

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

# The campaign in the footage

Findings of block `video-campaign` from the playthrough indexed in
[index.md](index.md) (Italian PlayStation release). Times are *part
number + time in that part*.

## The campaign map (part 1 of the block)

Every mission but the Boot Camp opens on the map of Saustralasia (part 2
0:26–0:37, and at the start of every mission in the index):

- the map with each territory in its holder's colour (0:26);
- the island's picture grows out of the territory (0:30);
- the island with a flag on each of its missions and an arrow pointing
  at the next one (0:34), held for a few seconds or until a button.

The PC executable does the same with its own pictures
(`Language/Tims/PigMap/`, decoded in
[campaign.md](../formats/campaign.md#the-campaign-map)); the PlayStation
draws one arrow from the left, the PC four arrows round the mission.
OpenHogs now shows the PC's version (`ui::CampaignMapAnimation`,
`CampaignSession::drawCampaignMap`).

**The enemy is the territory's holder.** At part 2 0:34 the arrow points
at a light blue flag (the American colour) and mission 1's enemies wear
American light blue (part 2 2:00–3:00). The executable shares the
territories out at random when a campaign starts (each island's five
missions among the five other nations, the last one to Team Lard), so
OpenHogs fights each mission against its holder (decision 47 settled).
The player of the footage is Japanese (yellow): no territory is yellow
at the start and they turn yellow as the missions are won.

## The start of a mission (part 2 of the block)

Measured on part 2 (mission 1), frame by frame:

- 0:52.4 the briefing ends and the match starts at once with the title
  card MISSIONE 01 : GUERRA: EPISODE ONE, centred high on the screen;
- the camera looks up from the ground at the player's first hog coming
  down under its parachute (0:53–0:58) and follows it; it lands at
  about 0:59 (some 6.5 seconds of descent);
- the camera then stays behind that hog while the others land around it
  (their labels appear in the air, 1:00–1:06), the title card still up;
- 1:07 the title card gives way to REDFLAMEFOX PRONTO... (the first
  turn's GET READY); about 14.5 seconds in all.

The PC executable has the same opening: a campaign match (but the Boot
Camp's) starts in state 0 of the game object (`+0x510`, set by
the constructor `FUN_0048efc0` when `DAT_00520744` is 1 or 2), with camera
mode 7, and the message routine `FUN_0045b8b0` writes gtext 158
(MISSION >2N : >S) with the level's name in that state. In mission 15 an
airship brings the squad instead of the parachutes (part 12 0:06).

Part 3 (mission 2, 0:38–0:52) agrees: the title card at once, the
parachute descent to about 0:45.5, then the other hogs' labels appear
round the landed hog.

OpenHogs now opens every campaign mission but the Boot Camp this way
(`World::beginArrival`, decision 171): the squads come down under
`WE_PARA` with the parachuting animation, the camera follows the
player's first hog from behind and below, and GET READY replaces the
title 8 seconds after the last landing; Enter skips the opening. Not
reproduced: mission 15's airship.

Seen on the way, for block `video-ui`: once landed, several hogs show
their name and health labels at the same time (part 3 0:47–0:52, part 12
0:50), where OpenHogs labels only the hog in play (decision 36).

## The missions and their maps (part 3 of the block)

Two frames of every mission, some 15 and 30 seconds after its title card
(the times are in [index.md](index.md)), with the radar enlarged, beside
OpenHogs' screenshot of the same map at the same moment.

**What matches.** The radars agree mission by mission: the squads' and
the enemies' crosses stand where the maps' data puts them, the same
crates (hearts for health, squares for weapons), and the red outlines of
the objectives' areas (missions 6, 9, 11, 12 and others). The original
reads the same `.POG` files as OpenHogs, so the missions are laid out
alike; the turn times agree with the executable's table (see the index).
The colours of the crosses differ because they follow the nations: the
footage's player fights in yellow.

**What did not: each level's look.** The footage's maps are not all lit
alike: the rainy maps (TRENCH, MEDIX, EMPLACE, FINAL) are dark, the
sunrise and sunset ones (TWIN, ZULUS, LIBERATE, SNIPER, OASIS, EYRIE)
orange, BAY is night-blue; OpenHogs drew every map in the same daylight
under a plain blue background. The executable's level record (0x4D5210)
explains it, and OpenHogs now follows the PC version
([executable.md](../formats/executable.md#properties-0xd4010--va-0x4d5210)):

- the textures of the terrain and the objects are recoloured as the
  record's fields 1–4 say (desaturation, brightness, a tint), as
  `_d3d.dll` does when it loads them (`scene::gradeColour`);
- the sky dome (`Chars/skydome.mad`) is drawn with the level's sky
  pictures (sunny, sunrise, sunset, night1, ominous, coldsky...);
- the distance fog of the level's sky fades the scene into its colour
  (decision 172 for its distances).

The footage is the PlayStation version, which lights the scene with the
record's fields 5–14 (an ambient colour and three lights); the PC
executable does not read them, so OpenHogs' rainy maps stay lighter than
the footage's. The weather (rain, snow) is not drawn yet (decision 174).

**Seen on the way.** Mission 14's title card is at 3:10 in part 13, not
2:00 (index corrected), and its squad starts in a wooden pen. In
missions 19 and 22 the air strike's aiming view, seen from above, labels
what lies below (BUNKER, CHIRURGO, ARTIGLIERIA): for block `video-weapons`.
The debriefings' special medals were not compared: the medals' rules come
from the executable (decision 162) and the footage shows only the ones
this run earned.

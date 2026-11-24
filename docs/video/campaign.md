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

## The start of a mission (part 2 of the block, to do)

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

OpenHogs starts a campaign match straight with GET READY: the opening
is the next part of this block.

## Still to compare (part 3 of the block)

For each mission: the enemies (number, ranks, where they stand), the
crates and objectives, the special medals on the debriefing, the turn
time, against the maps' data in OpenHogs; and the maps themselves
(sky, objects, starting places).

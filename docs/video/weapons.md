# Weapons (block `video-weapons`)

Notes from the footage indexed in [index.md](index.md), the PC manual and
the PC executable. Times are *part + time in the part*.

## The airstrike

Seen twice from start to end: part 17, 30:39–31:02 (mission 22) and part
7, 25:38–25:55 (mission 9).

1. The player picks ATTACCO AEREO in the weapon menu and fires (30:39).
2. The camera rises to a **view high over the map**, looking down. A
   crosshair starts on the player's own hog: a ring with a red cross in
   the middle and a long golden arrow through it (30:41). The player moves
   it over the map; the labels stay on the hogs, so the target can be
   picked by name (ARTIGLIERIA 200, 30:45). **The turn's clock keeps
   running** (25 at 30:41, 17 at 30:49).
3. Fire again confirms the spot (30:49). The camera comes back down to the
   hog, which raises its arms (part 7, 25:45; probably animation 56, "Air
   strike").
4. **An airship comes in**: seen from the ground, first small and far, then
   filling the view (30:53–30:58). The clock is stopped meanwhile (13).
5. At 30:58.5 it lets the bombs go from its gondola: small red bombs,
   falling straight down; the camera follows them down to the spot.
6. Five explosions, about a quarter of a second apart, in a line across
   the spot, the first on it (30:59.95–31:00.95). The next turn starts at
   31:01.5.

The PC manual explains the arrow: *"Selecting certain skills (such as the
Air strike) will present you with an aerial view of the map - move your
crosshair with the cursor keys to select your target. With air strikes you
can also use the Page up and Page down keys to rotate the angle that your
transport Airship will come in from."* Every map's archive holds the
airship's model, `AIRSHIP1` (the vehicle table names it AIRSHIP, gtext 89;
health 1).

**In OpenHogs** (decision 129): the first press of fire with an airstrike,
F-rain airstrike or special ops opens the aerial view (3500 units over the
crosshair, 1000 behind it); the movement keys move the crosshair at 2000
units a second, the aim keys turn the airship's way in at 90° a second
(the PC's Page Up and Page Down), and fire again confirms. The airship
(`World::Airship`) starts 5000 units before the spot and flies at 1000
units a second, about five seconds as on the footage, with its gondola
2500 units over the spot; each bomb leaves it as it passes over its
place, so the bombs fall one after the other along its course and reach
the ground in about 1.3 seconds (they start at 300 units a second and fall
with gravity), as on the footage. The camera waits on the ground past the
spot looking up at the airship, then follows the bombs. The turn waits for
the airship to fly off. Special ops use the same crosshair, without the
arrow; the manual's "certain skills" is read as both.

## The labels over buildings

The airstrike's target in part 17 is labelled **ARTIGLIERIA ♥ 200**: an
enemy's artillery, with the artillery's name and its 200 points of health
(`BIG_GUN`, docs/formats/maps.md). The executable explains it: the label
routine `FUN_0045a5c0` takes, for a hog in state 3 (in a vehicle) or 4 (in
a building), the object's health (`object + 0x4C`) and its name from the
tables at VA 0x4CF0B0 (vehicles: gtext 90 DUCK, 91 AQUA-TANK, 92 BOAT, 94
TANK, 95 TRUCK, 89 AIRSHIP, 93 PLANE) and VA 0x4CF098 (buildings: 82
ARTILLERY, 83 MEDIC TENT, 84 MASH TENT, 85 PILLBOX, 87 SHELTER, 88 TENT),
and `FUN_00459b20` labels the first hog inside each object. OpenHogs had
left the hogs inside without a label; it now labels them so.

## Health running down

A hurt hog's label does not drop to its new health at once: the number
runs down, slowly at first, then faster.

| Part 3 | Health shown |
|--------|--------------|
| 1:45.2 – 1:45.4 | 30 → 29 (the bayonet's 10 appears) |
| 1:46.0 / 1:46.4 | 28 / 27 |
| 1:46.6 / 1:46.8 / 1:47.0 | 25 / 24 / 22 |
| 1:47.2 / 1:47.4 / 1:47.6 | 20 / 19 / 17 |
| 2:14.1 / 2:14.4 / 2:14.7 | 29 / 27 / 25 (a mortar's 32) |
| 2:15.0 / 2:15.3 / 2:15.6 | 23 / 20 / 17 |
| 2:15.9 / 2:16.2 | 13 / 10 |

About 2 points a second in the first second, 10 to 13 after two or three:
a speed that grows by about 5 points a second each second fits both (2.5
× t² points after t seconds; 13 points in 2.3 s in the first case). The
executable keeps a hog's damage still to come at `hog + 0x1AC` (a hog
whose health minus it is under one point counts as dead, `FUN_00476d30`;
`FUN_00467ec0` empties it when the turn's damage is settled), and the
label shows the health itself (`hog + 0x4C`); the routine that moves one
into the other was not found. OpenHogs keeps its damage immediate and only
runs the label's number down at that pace (`ui::HealthCountdown`,
decision 67); healing shows at once.

The damage numbers themselves rise slowly from the hurt hogs and fade, in
their team's colour (2:14: 32 and 19 in yellow over two hogs of the same
side), as OpenHogs draws them. The label routine has a second, smaller
number of its own (`hog + 0x1C0`, drawn for 32 frames rising 62 pixels in
a pinkish red, 230, 100, 100), which was not seen on the footage.

## Weapons in the hands

The computer's mortar man (part 3, 2:03–2:10) walks with nothing in his
hands, then holds the mortar's white tube upright in front of him while
the power bar fills (six seconds), and fires from its mouth. It supports
moving launched weapons into the hands while they are used (decision 26,
annotated); it was not checked for the grenades on the belt.

## Not settled by the footage

No frame reviewed in this block showed clearly: the weapon staying out
while the hog turns (decision 87), swimming with a weapon (88), which hand
holds the weapon (91), driving (134) and the vehicles' own actions (22).
They are left for the refinement.

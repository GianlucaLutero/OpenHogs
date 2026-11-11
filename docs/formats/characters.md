# Characters: skeleton, animations, models

Used for: the hogs. Parsers: `src/formats/Skeleton`, `src/formats/MotionCapture`
(models use the normal `Model` parser); posing: `src/scene/CharacterPose`;
loading: `src/scene/CharacterLibrary`.

Everything here was worked out on the Steam release (v1.2b) for OpenHogs;
how each convention was found is explained so it can be checked again.

## Files

| File | Contents |
|------|----------|
| `Chars/pig.HIR` | the skeleton, 15 bones, shared by every hog |
| `Chars/mcap.mad` | 93 animations (not a `.MAD` archive despite the name) |
| `Chars/british.mad` | body models of the 9 classes, 3 levels of detail each (81 files) |
| `Chars/<team>.mtd` | textures of one team: `british`, `american`, `french`, `german`, `japanese`, `russian`, `teamlard` (120 TIMs each, same names in every team) |
| `Chars/FACES.MTD` | 7 eye and 7 mouth textures (`eyes001`–`007`, `gobs001`–`007`) for facial expressions (see below) |
| `Chars/BRITHATS.MAD`, `FHATS.MAD` + `.MTD` | separate hats, skinned to the head bone like the body (not used yet) |
| `Chars/WEAPONS.MAD` + `.MTD` | weapon models (next blocks) |

All teams use the models of `british.mad`; only the textures change.

## Skeleton: `pig.HIR`

15 records of 20 bytes, no header. Little-endian.

| Type | Meaning |
|------|---------|
| `int16` | parent bone (the root, bone 0, is its own parent) |
| `int16` | always 0 |
| `int16` x 3 | offset from the parent, model units (Y down) |
| `int16` x 5 | always 0 |

Parents always come before their children.

```
0 pelvis ── 1 spine ── 2 head
     │          ├── 3 left shoulder ── 4 left elbow ── 5 left hand
     │          └── 6 right shoulder ── 7 right elbow ── 8 right hand
     ├── 9 left hip ── 10 left knee ── 11 left foot
     └── 12 right hip ── 13 right knee ── 14 right foot
```

The names come from looking at which part of the model each bone moves.
The hog faces +X, Y points down and Z points to its left (bone 3 is at
Z = +88). With all angles at zero the arms stick out sideways (a "T pose").

## Animations: `mcap.mad`

```
uint32 offset, uint32 size    one entry per animation (93 entries);
                              the table ends where the first animation starts
frames                        size / 272 frames per animation
```

Frame (272 bytes for 15 bones):

| Type | Meaning |
|------|---------|
| `int32` x 3 | root (pelvis) position, model units, Y down; about (0, −100, 2) when standing |
| `int32` x 5 | unknown; mostly 0, sometimes small values (maybe events such as footsteps) |
| 15 × (`float` x 3, `float`) | Euler angles of each bone in radians, then a float that is always 1.0 |

Frame rate: not stored. 25 frames per second, checked by comparing with the
original game.

### How the angles combine

The engine's matrix is **the transpose of Rz · Ry · Rx**, i.e.
Rx(−x) · Ry(−y) · Rz(−z) for column vectors. That is what you get when
an engine that multiplies row vectors (DirectX style) builds Rz · Ry · Rx.
Three observations led to it:

1. **Y is the middle angle.** In all 67,950 bone records the Y angle stays
   within ±90° while X and Z use the full ±180°. A matrix-to-Euler
   conversion keeps exactly the middle angle within ±90°, so the order is
   X-Y-Z or Z-Y-X.
2. **Continuity.** Where the stored angles wrap from +180° to −180°, the
   right convention still gives a smoothly moving bone. Counting
   frame-to-frame jumps of the bone matrices over every animation, Rz·Ry·Rx
   and its transpose give 152; every other combination gives 600 or more.
3. **Arms down.** Of those two, only the transpose makes idle hogs stand
   with their arms at their sides; the other one makes them hold their
   hands up. (The shoulders of the idle animations store an X angle of
   about +0.9 radians.)

Blending between frames must use quaternions, not the Euler angles: at
the ±180° wrap, blending the numbers would spin the bone the long way.

### Animation names

`warhogs_.exe` contains a table of 59 animation names (an internal debug
list). They match animations 0–58 in order: 0 is a run cycle, 5 swimming,
30 hands on hips ("Lord Flash-Heart Pose"), 33 cowering, 37 a sneeze,
44 a salute, 57 hari-kiri. Animations 59–92 have no name in the executable.

| # | Name | # | Name | # | Name |
|---|------|---|------|---|------|
| 0 | Run cycle (normal) | 20 | Using Punch | 40 | Getting to feet |
| 1 | Run cycle (wounded) | 21 | Sword / Knife | 41 | Celebration #1 |
| 2 | Run cycle (more wounded) | 22 | Bayonet | 42 | Celebration #2 |
| 3 | Walk cycle (backwards) | 23 | Aiming Handgun | 43 | Celebration #3 |
| 4 | Turning on Spot | 24 | Aiming Rifle | 44 | Salute |
| 5 | Swimming | 25 | Aiming Machine gun | 45 | Look back |
| 6 | Swimming like Rick | 26 | Aiming Heavy weapon | 46 | Thinking |
| 7 | Getting into Vehicles | 27 | Standing around cycle 1 | 47 | Dying #1 |
| 8 | Jumping - Start | 28 | Standing around cycle 2 | 48 | Dying #2 |
| 9 | Jumping - Middle | 29 | Very Wounded | 49 | Dying #3 |
| 10 | Jumping - End | 30 | Lord Flash-Heart Pose | 50 | Drowning |
| 11 | Scramble | 31 | Looking around | 51 | Idle Cold |
| 12 | Getting out Handgun | 32 | Looking gormless | 52 | Idle Hot |
| 13 | Getting out Rifle | 33 | Cowering | 53 | Lay Mine |
| 14 | Getting out Machine gun | 34 | Brushoff 1 | 54 | Heal |
| 15 | Getting out Heavy weapon | 35 | Brushoff 2 | 55 | Pick pocket |
| 16 | Getting out Punch | 36 | Brushoff 3 | 56 | Air strike |
| 17 | Getting out Grenade | 37 | Sneeze | 57 | Hari Kiri |
| 18 | Getting out Sword / Knife | 38 | Flying through air/falling | 58 | Parachuting |
| 19 | Using Grenade | 39 | Bouncing on B-Hind | | |

## Body models

Normal VTX/NO2/FAC models (see [models.md](models.md)) with two
character-specific rules:

- The `bone` field of every vertex (and normal) says which bone it
  follows, and **positions are relative to that bone**: the hand vertices
  sit around (0, 0, 42), not at the end of the arm. Posing a vertex is just
  `boneTransform * position`.
- The texture index counts the entries of the team `.mtd` (0–119). The
  last two entries, `eyes000` and `gobs000`, are the default eyes and mouth.

| Class | High detail | Medium | Low | Texture prefix |
|-------|-------------|--------|-----|----------------|
| Ace | `pcace_hi` | `pcace_me` | `ac_hi` | `ACEU`, `AC_H`, `ACLO` |
| Grunt | `pcgru_hi` | `pcgru_me` | `gr_hi` | `GRUN`, `GR_H`, `GRLO` |
| Heavy Weapons | `pchvy_hi` | `pchvy_me` | `hv_hi` | `HEVY` |
| Legend | `pcleg_hi` | `pcleg_me` | `le_hi` | `LEGE`, `LE_H`, `LELO` |
| Medic | `pcmed_hi` | `pcmed_me` | `me_hi` | `MEDI`, `ME_H` |
| Sapper | `pcsap_hi` | `pcsap_me` | `sa_hi` | `SAPP`, `SA_H` |
| Saboteur | `pcsab_hi` | `pcsab_me` | `sb_hi` | `SABO`, `SBLO` |
| Sniper | `pcsni_hi` | `pcsni_me` | `sn_hi` | `SNIP`, `SN_H` |
| Spy | `pcspy_hi` | `pcspy_me` | `sp_hi` | `SPYU`, `SPLO` |

High detail models have about 650 vertices, medium about 475, low about 200.

## Facial expressions

Every body model draws its eyes with texture 118 and its mouth with texture
119 of the team archive (`eyes000`, `gobs000`), on all three levels of
detail, saboteur included. All face textures have the same size (56×21 for
eyes, 56×17 for mouths), so an expression is shown by drawing those
triangles with another texture of the same kind; the texture coordinates do
not change.

| Texture | Eyes | Mouth |
|---------|------|-------|
| `000` (team archive) | open, raised brows | open, tongue out |
| `001` | open | grin |
| `002` | closed | neutral |
| `003` | sad | frown |
| `004` | wide open | open "O" |
| `005` | angry | gritted teeth |
| `006` | open, on black | grin, on black |
| `007` | closed, on black | neutral, on black |

The black variants are drawn on the saboteur's black balaclava.

`warhogs_.exe` loads `chars\faces.mtd` as a texture group but the code that
picks a face was not found by looking for the file name or the texture
numbers, so **the original rules are not known yet**. OpenHogs uses its own,
based on the animation names, and lets content packs replace them
(`Chars/expressions.ini`, see `content/README.md`):

| Expression | Eyes / mouth | Animations |
|------------|--------------|------------|
| neutral | 001 / 002 | everything not listed below |
| happy | 001 / 001 | 30 Lord Flash-Heart pose, 41–43 celebrations, 44 salute |
| sad | 003 / 003 | 1–2 running wounded, 29 very wounded, 40 getting to feet, 51–52 idle cold/hot, 57 hari kiri |
| shocked | 004 / 004 | 8–9 jumping, 11 scramble, 33 cowering, 38 flying, 39 bouncing, 50 drowning, 58 parachuting |
| angry | 005 / 005 | 12–26 weapons (getting out, using, aiming), 34–36 brushing off, 56 air strike |
| eyes-closed | 002 / 002 | 37 sneeze |
| dying | 002 / 004 | 47–49 dying |
| gormless | 000 / 000 | 32 looking gormless |

On top of that the hogs blink: eyes closed for 0.15 s about every 5.5 s, at
irregular moments that differ from hog to hog.

## Movement animations

All animations stay on the spot: the run cycle moves the legs but not the
hips, so the game itself moves the hog. To keep the feet from sliding, the
running speed comes from the stride: in the run cycle (animation 0, 17
frames) each foot travels 326 model units (163 world units) per step, two
steps per cycle, which at 25 frames per second is **480 world units per
second**. The backward walk (animation 3, 20 frames) gives 180 units per
second the same way.

OpenHogs uses: 0 running, 3 walking back, 4 turning on the spot, 5
swimming (its root is 140 units lower, so the body sinks into the water),
8, 9 and 10 for the start, middle and end of a jump, 38 for falling.

## Placing a hog

Model space is converted to world space like map objects: scale by 0.5 and
mirror Y. In the idle animations the lowest point of the body is about
133–150 model units below the origin, so OpenHogs puts the origin 145 model
units above the ground (`scene::kStandingFeetDepth`).

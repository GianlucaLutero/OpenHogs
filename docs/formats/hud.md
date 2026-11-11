# The HUD of the original game

How the PC game (`warhogs_.exe`, Steam v1.2b) builds its match HUD, read
with Ghidra (`tools/ghidra/README.md`). OpenHogs reproduces it in
`ui::HudLayout` (layout and moving parts), `ui::FloatingNumbers` (damage
numbers) and `gameapp::OriginalHud` (drawing).

Addresses are virtual addresses in the executable (file offset = VA −
0x401200 for these `.data` tables, see `docs/formats/executable.md`).

## Pictures

`FUN_004852c0` loads, when a match starts:

| File | Use |
|------|-----|
| `Language/Tims/dashtims.mad` | the instruments (table below) |
| `Language/Tims/facetims.mad` | `aburst` (slot the weapon icon is copied into), `croshair`, `point01/02`, `arrow000`, `wepn01`–`wepn20` (pieces of the weapon menu's frame) |
| `Language/Tims/expltims.mad` | damage digits `num0`–`num9`, `heart`, `ptp0-2`/`ptpa` (markers), smoke and fire frames, `Shad_org` (shadow) |
| `Language/Tims/tboxtims.mad` | `npro1`–`npro4`, a frame for text boxes |
| `Language/Tims/mapicons.mtd` | the radar map: `map1`, icons for hogs, health, pick-ups, props, bombs, rain, snow |
| `Language/Tims/menutims.mad` | weapon icons (56 × 34), loaded by `FUN_00457690` |
| `Chars/sight.mtd`, `Chars/top.mad`, `Chars/sight.mad` | the sniper sight and the 3D radar |

`dashtims.mad` (46 TIMs), by index:

| # | Name | # | Name |
|---|------|---|------|
| 0–4 | `newpow3`–`newpow7` (power gauge, lower row) | 26–27 | `score1`, `score2` |
| 5–6 | `newpow1`, `newpow2` (power gauge, top) | 28–35 | `pause1`–`pause8` |
| 7 | `powg1` (power needle) | 36–40 | `ang1`–`ang5` (angle dial) |
| 8–11 | `clock01`–`clock04` | 41–42 | `divide1`, `divide2` (aim limit marks) |
| 12 | `timlit` (clock lamp) | 43–44 | `wedge1`, `wedge2` (the fan) |
| 13–22 | `timer0`–`timer9` (clock digits, 24 × 25) | 45 | `angpoint` (aim arrow, 64 × 15) |
| 23–25 | `sights`, `target`, `pcpie4` (power lamp) | | |

## Layout

`FUN_004544e0` copies static tables of 40-byte sprite records (x, y as
int16 in a 640 × 480 screen, then size, texture, vertices) and moves each
by the real screen size: by width/2 − 320 for pieces in the middle, by
width − 640 for pieces on the right, by height − 480 for pieces at the
bottom. OpenHogs' canvas is always 480 high, so only x moves.

| Table | Pieces | Position(s) | Tied to |
|-------|--------|-------------|---------|
| 0x4CF468 | `ang1`–`ang5`, `divide1`, `divide2` | (480,11) (544,33) (480,75) (544,75) (608,41); (496,33) (483,71) | right |
| 0x4CF580 | `wedge1`, `wedge2`, the same flipped | (499,16) (482,34) (499,71) (482,71) | right |
| 0x4CF620 | `angpoint` | (486,66), turning about (536, 73.5) | right |
| — | weapon icon | `ang1` + (67, 44) = (547,55) | right |
| 0x4CF300 | `newpow3`–`newpow7`, `newpow1`, `newpow2` | (160…416, 419), (160,399) (224,399) | centre, bottom |
| 0x4CF418 | `powg1` | x = 160 + 95 + power × 151, y = 426 | centre, bottom |
| 0x4CF440 | `pcpie4` | (210,423) | centre, bottom |
| 0x4CF648 | `clock01`–`clock04` | (503,377) (567,377) (503,405) (567,405) | right, bottom |
| 0x4CF6E8 | `timlit` | (553,376) | right, bottom |
| 0x4CF878 | `score1`, `score2` ×3, `score1` mirrored | (160…415, 419) | centre, bottom |

## Behaviour

Per frame, `FUN_00457840` draws the HUD:

- **Angle gauge** (`FUN_004584e0`): the dial is always drawn. While the
  hog can aim, a level rises by 3 per frame up to 50 (and falls the same
  way): the arrow and the limit marks are drawn at alpha 5 × level, the
  fan at alpha level (a faint glow). The arrow's angle is the hog's pitch
  in 4096ths of a turn; flat points left, up points up. Some weapons draw
  `divide1`/`divide2` and clamp the arrow at 45°.
- **Power gauge** (`FUN_00457fb0`): slides in and out in 19 frames from
  200 pixels below its place. The needle follows the power (0–4095).
  `pcpie4` is drawn for all weapons but a few.
- **Clock** (`FUN_00458a80`): slides like the power gauge. The time left
  is counted in hundredths of a second and shown as (hundredths + 100) /
  100, two digits. A digit rolls up during the 12 hundredths before it
  changes, with the offsets of the table at 0x4CF2D0 (0, 2, 4, 6, 8, 10,
  22, 24, 23, 23, 24, 24 pixels); the digits are cut to the clock's
  window. The lamp is tinted from the table at 0x4CF250: green, amber
  under 10 s, red under 5 s, plus a glow computed each frame.
- **Sprites are drawn in reverse order**: what is submitted first ends on
  top (so the needle shows over the tube, the digits behind the clock's
  face, the lamp over the clock).
- **Messages** (`FUN_0045b8b0`): centred, at y = 140 (some at 160 and
  180). "GET READY >S..." and "THE ENEMY RETALIATES..." take the colour of
  the team from the table at 0x4CF1E0: British (70,240,70), American
  (70,240,240), French (70,70,240), German (255,255,255), Russian
  (240,70,70), Japanese (240,240,70), Team Lard (255,32,255). Other
  messages are white.
- **Damage numbers** (`FUN_00489c40`, `FUN_00489e50`): particles of type
  `#` print a number with the digits of `expltims.mad` (character − 0x2C
  = picture index), side by side, sized by the distance to the camera
  (never below 10 pixels), tinted and fading over the particle's life.

Not reproduced yet (`docs/open-decisions.md`): the weapon menu (the frame
of `wepn01`–`20` with a 4 × 4 grid of icons and ammunition counts,
`FUN_00459550`), the sniper sight (`FUN_00457eb0`), the 3D radar
(`FUN_004582f0`, `Chars/top.mad`), text boxes and the limit marks.

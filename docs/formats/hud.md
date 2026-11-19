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
| `Chars/sight.mtd`, `Chars/sight.mad` | the rifle sight (`sight.mad` holds the model `hole`) |
| `Chars/top.mad` | asked for as the radar's object; not in the PC folder (the renderer draws the radar without it) |

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

## Rifle sight

The "rifle cam" (`FUN_00492820`, messages "Changing to rifle cam",
"Changing to barrel cam"): while a button (input bits 0x1100) is held
and the hog stands ready with weapon 6–15, 17 or 18 (PISTOL to ROCKET
LAUNCHER, MEDICINE DART, TRANQUILISER; the weapon number is the text
entry − 96), the camera switches to mode 0x0E; letting go restores the
previous mode. Mode 0x0E (and 0x11, the "barrel cam" of weapons 45 and
46, not reproduced) sets the HUD's sight flag (`FUN_00457830`, HUD
object + 0xC6C). With the flag set, `FUN_00457eb0` draws:

- the black around the hole: 46 polygons built at start-up
  (`FUN_00457140`) from the faces of the model `hole` in
  `Chars/sight.mad` — a ring whose vertices lie on a circle of radius
  239 (hole) and on a 1030 × 1620 rectangle — scaled by 0.8 onto the
  640 × 480 screen, centred, 9 pixels higher than the middle;
- the crosshair, `croshair.tim` of `facetims.mad`, 48 × 48, centred on the
  screen.

The radar map is not drawn meanwhile; the angle gauge and the clock are.
A full PS1 playthrough on YouTube shows the same view (first person along
the gun, the black mask with the round hole; there the crosshair is four
small marks).

## Weapon menu

`FUN_00457690` loads it: `language\tims\menutims.mad` entry 0
(`menu.bmp`, the 257 × 274 panel: four tabs along the top, a grid of
windows, a bar at the bottom; magenta is transparent), entries 1–67 (the
weapons' icons, 56 × 34, one per weapon number), `smlfont.bmp` (digits
6 × 10) and `smlhalft.bmp` (missing on PC; `halftone.bmp` is there).
`FUN_00459550` draws it each frame, sliding in from the right by
309 × a percentage table (0x4D1954): for each of 4 columns (x 16, 88, 157,
229) and 4 rows (y 25, 71, 117, 164) of the open tab (16 slots, a weapon
number and its uses each), the icon, the halftone over a weapon with no
uses, and the uses (capped at 9) in the icon's corner; then the selection
frame at the chosen icon − (15, 11). The PC picture only has 3 columns of
windows. OpenHogs' version: `ui::WeaponMenu`, `OriginalHud::drawWeaponMenu`
(decision 151).

## Aim limit

`FUN_0045c130` sets a flag when the weapon in hand changes: weapon numbers
16 (guided missile), 28 (mortar) and 50 get flag 1, which draws `divide1`
and keeps the arrow at 45° or more; hogs in vehicles and buildings get
flags 2, 3 or 5 (`divide2`, both). OpenHogs keeps the mortar and the
guided missile at 45° or more and draws `divide1` (decision 153).

## Radar

`FUN_004582f0` places the radar each frame: an object made from
`Chars/top.mad` (missing on PC) with `mapicons.mtd` as its textures, whose
screen position is packed in its word +0x4C (x in bits 20–31, y in bits
8–19): x = 110, y = height − 75, plus a slide of 200 pixels in 19 ticks
like the clock (HUD flag +0xC69, set when the match starts). The renderer
(`_d3d.dll`) does the rest:

- `afInitScanner` (called through `DAT_00538000` when the HUD is made)
  paints a 64 × 64 texture into the slot of `map1`, one pixel per tile:
  the tile's surface colour from the table at 0x1002BEB8 (mud 60,50,40;
  grass 40,70,40; metal 128; wood 153,94,34; water 90,90,150; stone and
  rock 50; sand 100,80,30; ice 180,240,240; snow 100; quagmire 60,50,40;
  lava 240,100,53) × a brightness of 64 + 130 × (height of the tile's
  corner − lowest) / (highest − lowest), divided by 512 and capped at 31
  (5-bit channels); a tile with the mine flag (0x40) is plain red. The
  first column repeats the second.
- `FUN_10009810`, at the end of every frame, draws the texture on a 4 × 4
  grid of a square plane (half size 0.151, set by
  `afSetScannerSizeSmall(0)`; 0.121 for the small size), projected with
  the camera's rotation and a focal length of 480, then a 10 × 11 mark
  5 pixels up and left of each thing it collected while drawing the
  scene: every animated model (hogs) with `iconpig`, tinted by the table
  at 0x1002BE70 indexed by the model's nation (green, cyan, blue, grey,
  red, yellow; white for models with flag 2), and objects whose word +0x48
  is 4 — the exe gives that to `CRATE1`, `CRATE2`, `CRATE4` and
  `PROPOINT` (object names 20–23) with kind = name − 20 — as `iconpkup`
  (kinds 0 and 2, tint 169,148,63), `iconhart` (1, tint 255,64,121) or
  `iconprop` (3, tint 219,136,53). Positions are divided by 18884 / 0.151.

OpenHogs: `ui::radarMapPicture`, `ui::radarMarks`, `ui::RadarView`,
`OriginalHud::drawRadar` (decision 154).

## Text box

`FUN_0045e530` makes it: a queue of 8 messages (0x118 bytes each), the
tube's ends at x0 = middle − 120 and x1 = middle + 122, a text window from
middle − 102 to middle + 104 (206 pixels). `FUN_0045eb10` queues a message
(text, duration in hundredths, scroll speed, a flag for the big tube);
callers: crates (`FUN_00465420`: the weapon's name and `X<count>` — just
the name in the Boot Camp — or gtext 172 for health, 171 when the hog
already has 14 weapons; 10 s, 3 pixels a tick), Boot Camp instructions
(10 s, 5 pixels a tick), a hog leaving a network game (gtext 258–270), and
`FUN_0045ec50`, the big tube with the teams' strength (4 s), at the start
of the match and of each round (+0x420 is cleared when the turn count is a
multiple of the number of teams); not in the Boot Camp.

`FUN_0045e6a0` draws it each tick. The tube comes down in 10 ticks along
the table at 0x4D0190 (0, 10, 15, 30, 40, 90, 100, 95, 100, 100 %), stays,
then goes up the same way and the next message starts:

- small tube: `npro4` (32 × 64) at x0 − 14 (turned round) and x1 − 18,
  four `npro3` (64 × 64) at x0 + 18 + 48 i; y = 45 % − 50;
- big tube: `npro2` at x0 − 64 (turned round) and x1, four `npro1` at
  x0 + 64 i; y = 105 % − 110; `FUN_0045efb0` draws the strength bar at
  y + 35 from x0 + 1: 240 pixels shared by the teams' health (+0xE4 >> 7),
  8 tall in the team's message colour fading to it − 70, and a second row
  under it.

`FUN_0045ecb0` measures the text (with `macfont.bmp`, missing on PC): a
text under 207 pixels stays centred for 3 s from the start; a longer one
(`FUN_0045ee30`) enters at the window's right edge and moves left by the
speed each tick, and the tube goes up once it has gone by. The text is cut
to the window, 22 pixels tall, 33 below the tube's top.

OpenHogs: `ui::TextBox`, `OriginalHud::drawTextBox` (decision 155).

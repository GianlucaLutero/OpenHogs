# The front end (menus) in the executable

How the PC executable (`warhogs_.exe`, see [executable.md](executable.md))
builds and animates its menus. Read with Ghidra and capstone; addresses are
virtual addresses. OpenHogs uses this for the original list menu
(`ui::OriginalMenu`, `OriginalMenuView`).

## Screens and screen types

The front end is a table of 52 screens of 0x48 bytes (base pointer
`DAT_0051aae0`, built by `FUN_0042f120`). Field +0x24 is the screen's
**type**, which decides how it is drawn; +0x0C is the selected entry; +0x04
and +0x08 are the text numbers (in `fetext`) of its title and of its first
entry. The types of the 52 screens are at 0x4C1470:

```
12 1 1 2 4 4 4 4 4 4 8 0 5 0 1 0 2 21 0 6 7 9 10 2 12 13 2 2 20 21 12 21 6 8 5 3 10 3 14 12 13 15 19 12 2 16 21 18 18 22 18 18
```

and the number of entries of each at 0x4C0EE8. The text numbers follow
from those two tables (`FUN_0042f120`); worked out, the screens are:

| Screen | Type | Title (fetext) | Entries |
|--------|------|----------------|---------|
| 0 | 12 | ARE YOU SURE YOU WANT TO QUIT? | YES, NO |
| 1 | 1 | MAIN MENU | ONE PLAYER, MULTI-PLAYER, OPTIONS, QUIT APPLICATION (fetext 13-16; the PC replaces the console's 9-12) |
| 2 | 1 | OPTIONS | AUDIO, CONTROLS, CREDITS |
| 3 | 2 | SELECT TEAM | the six teams |
| 4-9 | 4 | (the team pages) | |
| 10, 21, 33 | 8, 9, 8 | LOAD, SAVE ARMY, LOAD ARMY | |
| 14 | 1 | ONE PLAYER | NEW GAME, LOAD GAME |
| 16 | 2 | MULTI-PLAYER | TEAM A-D, DONE, NETWORK |
| 17 | 21 | FIELD CONDITIONS | |
| 19, 32 | 6 | (a pig's page) | PROMOTE, SWAP POSITION, RENAME |
| 20 | 7 | VOLUME CONTROL | MODE, MASTER, SFX, VOICES |
| 24, 39, 43 | 12 | REALLY QUIT?, PLAY TRAINING MISSION?, REALLY QUIT APP? (fetext 148) | YES, NO |
| 25 | 13 | CAREER PATH | HEAVY WEAPONS, ENGINEER, ESPIONAGE, MEDIC |
| 28 | 20 | MULTI-PLAYER SELECT LEVEL | SURVIVAL-NOVICE ... LEVEL SETUP |
| 29 | 21 | RANDOM GENERATED LEVEL | |
| 38 | 14 | SCREEN POS | OKAY |
| 41 | 15 | CONTROLLER SETUP | |
| 45 | 16 | CREDITS | |
| 47-51 | 18, 22 | NETWORK: ... | |

## Pictures

`FUN_0042f4d0` reads `FEbmps/FEbmp.MAD` ([mgl.md](mgl.md)) into a table of
picture pointers (0x51204C...), in groups loaded per screen
(`FUN_00421ad0`: `FUN_0042f4a0` decompresses one into a surface whose
handle goes to 0x511B94...). Each frame, a screen binds surfaces to
**slots** (`DAT_00512880` + 4 × slot) and draws the slots.

## Drawing

`FUN_0041bef0` draws a screen: first the background `pigbkpc1`, then, by
screen type, sprites at fixed positions of the 640 × 480 screen.
`FUN_0041afa0(x, y, surface, rect)` draws part of a picture,
`FUN_0041af70` draws it stretched (a negative width mirrors it) and
`FUN_0041afc0` draws a whole picture (its arguments are only visible in the
assembly). Sprites are queued; **the first one queued ends up on top**.
`FUN_0041e780` draws the type's moving parts on top.

Moving parts use two arrays of offsets, x at `DAT_00512cd0` (1024-wide
units, converted by `FUN_0041adb0`: × 640 / 1024) and y at `DAT_00512950`
(820-high units, `FUN_0041add0`: × 480 / 820). Each screen type starts
from the offsets of tables 0x4C09B8 (x) and 0x4C0A18 (y) and springs to 0.

`DAT_004c0c5c` is a width the panels stretch by (the middle column of the
picture repeated); it is 50 in the file but the text boxes only line up
with the panels when it is 0, so OpenHogs uses 0 (`docs/open-decisions.md`).

### Type 1: the list menu

With *ox* = −10 (the sprite shift) and *oy* = the panel's drop (pixels, in
steps of two), from the top sprite down:

| Sprite | Picture | Position |
|--------|---------|----------|
| lamp *i* (0-3) | `light1`-`light3` (off, half lit, lit), rows 40*i* to 40*i* + 38 | (ox + 478, oy + 176 + 40*i*) |
| bar *i* | `chose1`-`chose6` (flip frames) | (ox + 319 + 12 for *i* = 1, 2; oy + 182 + 40*i* + 2 for *i* ≥ 2) |
| dial | `dial0001`-`dial0012` | (ox + 140, oy + 192) |
| cog | `cog0` | (ox + 44, oy + 192) |
| tall cog | `cogB00` | (ox + 524, oy + 160) |
| left rack | `track` from column 1, stretched to 64 × 638 | (ox + 1 + rack, −272) |
| right rack | the same, mirrored | ending at (ox + 666 − rack, −222) |
| title plate | `title1`-`title6` (flip frames) | (ox + 296, oy + 112) |
| panel top | `fullmenu`, 340 × 50 from its top | (ox + 60, oy − 48) |
| panel | `fullmenu` | (ox + 60, oy) |

### Types 3, 4, 11 and 12: the settings machine (here the yes/no question)

Drawn by the same code (0x41C492). With *box* = the slide of the box
(`DAT_00512cdc`, 1024-wide units, converted) + 7, and *y1* = twice the
plate's drop (`DAT_00512954`) in pixels, from the top sprite down:

| Sprite | Picture | Position |
|--------|---------|----------|
| arms of the plate | `namarm1`, `namarm2` | (80, y1), (435, y1) |
| name plate | `name0`-`name5` (flip frames) | (123, y1 + 16; + 32 at frame 3) |
| tray | `info1.mgl1`-`info6.mgl1` (opening frames) | (box + 368, 98) |
| dial (box open) | `yesdial1` (YES) - `yesdial7` (NO) | (box + 392, + 16 past frame 1; 393) |
| box | `yesno01`-`yesno06`, then `yes` | (box + 280, + 32 for `yes`; 345) |
| pipe | `mainbar0`: rows 0-160; columns 35-48; rows 100-160 | (box + 312, 130); (box + 359, 130); (box + 312, 290) |
| riser | `mainbar1`: rows 0-39, and rows 0-2 repeated from the top | (box + 608, 72); (box + 608, 0 to 72) |
| rack | `track`, mirrored | leaves the screen in type 12 |

For type 12, the update (`FUN_00423e70`) springs the plate down from
−150 (gain 3, damping 30, top speed 6) and the box in from 0x4C8 (gain
10, damping 40, top speed 20), unfolds the box to frame 6 (`yes`), closes
the tray to `info1` and turns the dial to frame 6 × *selected*. The title
is centred in the box (299, 77, width 400) and the answers in boxes at
x 600 and 710, y 658 (1024 × 820 units), which do not line up with the
pictures as composed here: OpenHogs centres them on the plate's glass and
the box's windows (`docs/open-decisions.md`).

## Text

The menus write with `FEText/chars3` (types 0, 1, 2, 4 and 5) or the
CHARS2 set (others). Text boxes are in 1024 × 820 units: the title's box
per type at 0x4C1548 (x), 0x4C15A8 (y), 0x4C1608 (width), 0x4C1668
(height); the entries' boxes are 16-byte records (x, y, width, height) at
0x4C1728, numbered per type from the running totals of 0x4C16C8. Text is
centred in its box (`FUN_00431b80(font, 2)`).

| Type 1 | Box (x, y, width) | On the 640 × 480 screen |
|--------|-------------------|-------------------------|
| title | 518, 227, 210 | centred on x 389.4, top at 132.9 |
| entries 1-4 | 528, 329 / 398 / 468 / 537, 205 | centred on x 394.1, tops at 192.6 / 233.0 / 274.0 / 314.3 |

Colours are font tints (128 = the picture's colour): the title (100, 100,
60), an entry (80, 80, 45), the selected entry (120, 120, 75), a disabled
entry a third of those (`FUN_00428cf0`).

## Animation

`FUN_0041fd30` is the spring of every slide (see `ui::frontEndSpring`):
the velocity becomes ((target − value) × gain / 10 + velocity) × 10 /
damping / 2, the value moves by twice the velocity (at least 1), and on
reaching the target it swings once past it.

`FUN_0041fec0` ticks the screen's **elements** (up to 45): each has a
frame (`DAT_00512c18`), a target or a sequence (`DAT_00512b60`) and a
countdown (`DAT_00512a80`). `FUN_00423e10` sends an element to a frame,
one frame per tick; `FUN_0041fff0` starts a sequence from the pairs
(frame, ticks) at 0x4C0A78 (a negative frame jumps back).

For type 1 (`FUN_00423e70`, `FUN_00427c90`):

- on opening, the racks start 36 pixels out and spring in (gain 4,
  damping 12, top speed 21); then the panel springs down from −650 (gain
  8, damping 17, top speed 80); when it is less than 50 from its place, the
  bars and the title plate flip from frame 2 to frame 6 (= picture 1);
- elements 0-3 are the lamps: the selected one plays the sequence from
  pair 4 (half lit 1 tick, lit 10, half lit 1, off 5, again), the others
  go off;
- element 4 is the dial: it turns to frame 4 × *selected* (minus 1 past 4:
  0, 4, 7, 11) and clicks when it gets there.

### Type 2: the longer list (SELECT TEAM, MULTI-PLAYER)

Built from the settings machine of type 12 (the same plate, tray, box
and pipes, drawn by the foreground `FUN_0041e780`, case 2), plus a
monitor whose screen shows the entries. With *L* the list's drop
(`DAT_00512950`, from 546: it rises; spring gain 10, damping 15, top
speed 15) in pixels, *v* element 0 (moved one step a tick towards
5 × the selected entry) and *s* = 2 × (4*v* + *L*, in pixels):

| Sprite | Picture | Position |
|--------|---------|----------|
| tray, box, pipes | `info1`, `yesno01` (closed), `mainbar0`, `mainbar1` | as type 12 |
| lamp | `lit1`-`lit3` (element 1) | (537, *s* + 202) |
| knob | `selec00`-`05`, frame *v* mod 6 | (348, *s* + 170) |
| bar | `select`: columns 0-40, then from column 30 | (412, *s* + 186), (422, *s* + 186) |
| stem | `counsele` rows 290-294, 25 times every 4 pixels | (385, *L* + 450 ...) |
| cogs | `selcog1`-`6`, frame ((6 − drop) × 2 / 25) mod 6 | (553, *L* + 180) |
| monitor | `counsele` (179 × 306; columns 0-128 and 128-179 around the stretch) | (385, *L* + 160) |
| arms, plate | `namarm1`, `namarm2`, `name0`-`5` | (80, *P*), (435, *P*), (123, *P* + 16) with *P* the plate's drop |

The plate springs from −150 (3, 30, 6) and the box from 0x4C8 (13, 30,
30). The entries are written in text boxes 5-12 (x 687, width 180,
y 369, 407, 447, 489, 527, 567, 607, 647), in the list menu's font and
colours; the title's box is (201, 65, 600). Screen 16 (MULTI-PLAYER)
adds four team rows with `pcchip` / `pcpad` / `com_man`, still to read.

### Screen 16: MULTI-PLAYER's team holders

Screen 16 is a type 2 list with four team holders on the left, drawn
after the monitor (`FUN_0041bef0`, case 2, when the screen is 16). For
team *i* at y 114 + 78*i*: the holder `com_man` (203 × 78) at (*x* + 8, y)
and, centred on its window (x 139, y 38 of the holder), `pcpad` for a
person or `pcchip` for the computer (the team's type, through the table
{1, 0, −1}: nothing when off). *x* springs to 0 (gain 6, damping 15, top
speed 10) when the team plays and to −250 when it is off
(`FUN_00423e70`, case 2). The executable bobs the picture with a sine of
a counter that grows by 70 a tick and writes the team's name with
`FUN_004188c0`.

### Types 20 and 21: the level panel and FIELD CONDITIONS

Type 20 (screen 28, MULTI-PLAYER SELECT LEVEL; `FUN_0041bef0` at
0x41E579), with *x* = horizontal offset − 52 and *L* half the drop
(spring to 0 from 800: gain 8, damping 17, top speed 80; the plate too):

| Sprite | Picture | Position |
|--------|---------|----------|
| two lamps per slot | `lit1`-`lit3` (elements 32-37), columns 8-32 | (*x* + 153, row + *L* + 55), (*x* + 340, same) |
| the chosen slot's arrow | `lselpc2` (26 × 41) but its last column | (*x* + 355, *L* + 130 + 46 × slot, + 18 for the last) |
| panel | `lselpc` (640 × 480) | (8, *L* + 53) |
| plate | `sqname01` (400 × 96) | (124, plate − 12) |

The slots' rows are 73 + 46 × slot (the sixth 18 lower); their words are
in text boxes 108-113 (x 196, width 180; y 242, 321, 400, 479, 555, 665),
in (120, 120, 100) when chosen, (90, 90, 70) otherwise, (45, 45, 40) when
another screen is on top. The original's slots are level types
(SURVIVAL-NOVICE, SURVIVAL-EXPERT, SURVIVAL-STRATEGY, DEATH MATCH,
GENERATE; fetext 106-110) and LEVEL SETUP (111).

Type 21 (screen 17, FIELD CONDITIONS, and 29, RANDOM GENERATED LEVEL)
draws no picture of its own: it is drawn over the level panel and writes
"LABEL: VALUE" (`FUN_00428cf0`, case 21) in text boxes 114-123 (x 570,
width 180; y 257 to 651, every 44), which fall on the panel's dark
monitor (x 332-540, y 133-413 on screen).

### Type 15: CONTROLS

`FUN_0041bef0` draws only element 0's picture, `pckey000`-`pckey006` (a
board turning to face the player, 410 × 480), at (offset + 156, 25).
`FUN_0042dea0` writes six rows from the first visible one: the action's
name (fetext 349 + row: FORWARD, BACK, LEFT, RIGHT, ACTION, FIRE //
SELECT, JUMP, WEAPONS MENU, AIM // LOOK, ANGLE DOWN, ANGLE UP, PAUSE) in
boxes 66-71 (x 280, y 250 + 85 a row) and its key (fetext 361 onwards,
from the key codes at 0x4C0110; 458 when unknown) in boxes 72-77
(x 530); the row being changed blinks.

### Type 16: CREDITS

No picture. `FUN_00428cf0` (case 16) writes rows of two texts, fetext
503 + 2*i* (x 100, width 500) and 504 + 2*i* (x 500), 80 units apart from
the screen's offset, while −51 < y < 901, in (120, 120, 120). The update
(case 16) sets the offset to 1000 − (milliseconds since the screen
opened) / 10, starting again below −9440. The PC's credits are fetext
503-652.

### Types 5 and 6: the campaign's squad and a hog's page

Type 5 (screens 12 and 34; `FUN_0041bef0` at 0x41D285, foreground at
0x41EC55). With *X* the horizontal offset − 10:

| Sprite | Picture | Position |
|--------|---------|----------|
| boards | element 0: `sqpics00`-`sqpics10` while turning, `sqpic` (300 × 480) at frame 6 | (*X* − 22, 20; −12 for `sqpic`), and mirrored, ending at 672 − *X* |
| faces (frame 6) | a picture per hog (slots 0x5128E4 on), `backgr~1` behind | x 57 (+ 462 for the right board), y 74 + 74 × row (the fifth row at 368); the right board holds only three |
| class and rank | `pcHweap`, `snipr`, `sappr`, `pcmedic`, `cmndo` (0x5119B0), pips and stripes (0x5119CC), flags | beside each face |
| selectors | `sqdial01`-`06` (element 1; the right one mirrored at 662 − *X*), lamps `lit1`-`lit3` (elements 12 and 13) | left: (*X* − 22, 2*y* + 28), lamps (*X* + 40, 2*y* + 48) and (*X* + 121, 2*y* + 49); right: lamps (491 − *X*, 572 − *X*); *y* = 37*v* / 5 + 18, *v* elements 4 and 5, five steps a row |
| options | `sqoptsF` (200 × 101) with two lamps, one per option | (428, 2*r* − 91), lamps (429 and 601, 2*r* − 75), *r* = 187, 215, 243 or 324 (− 120 past two options) + the slide (from 1020: spring 12, 20, 40) + 27 |
| monitor | `pigpro` (200 × 191) | (232, 2 × (slide + 156) − 8): from 400, spring 5, 14, 40 |
| plate | `sqname01` | (120, 2 × (drop − 7)) |

Type 6 (screens 19 and 32: PROMOTE, SWAP POSITION, RENAME) is drawn over
the squad: `swap.mgl1` (320 × 256) at (180, 2 × (slide + 130)), rows
0-245 and then rows 212-256 below them, with the arrow `swap01` / `swap02`
(element 17) at (230, 2 × (slide + 130 + 8 × element 16) + 87); the slide
springs from 500 (12, 20, 40). Its words are in boxes 32-34 (x 412,
y 611, 666, 721).

Type 10 (screens 22 and 36: the results between missions) draws no
picture.

### Type 7: VOLUME CONTROL

A screen whose pointer at +0x2C is set is drawn over that screen
(`FUN_00427770` first draws the screen at +0x2C): VOLUME CONTROL keeps
the OPTIONS list menu on screen. Its update (`FUN_00423e70`, case 7)
springs the list's drop (`DAT_00512950`) to −180 (gain 5, damping 14,
top speed 20), lifting OPTIONS about 105 pixels, and its own panel
(`DAT_00512958`, 400 when the list menu opened) to 0 (gain 8, damping 30,
top speed 30). The list menu's update (case 1) brings the list back down
only once `DAT_00512958` is 400 again.

- The panel: `vol00`-`vol06` (368 × 240) at (100, 240 + twice the drop in
  pixels) (`FUN_0041bef0`, case 7). `vol01`-`vol06` rise and unfold;
  `vol00` has the two plain rails (rows 109 and 140, x 160-286) that
  the knobs fit.
- The knobs, once element 8 is at frame 6 (`FUN_0041e780`, case 7):
  `vollit01`-`03` (48 × 48, dark to lit; the frame is elements 9 and 11)
  at x 230 + level × 12.5 (level 0-10), y 320 + the drop + 31 per knob:
  MASTER and SFX.
- The words (`FUN_00428cf0`, case 7): text boxes 35-38 of the table at
  0x4C1728 (the first box of each type is the running sum of the counts at
  0x4C16C8) — (334, 542), (334, 587), (334, 637), (334, 687), 250 × 30 —
  in colour (120, 120, 80) with the settings-machine font. MODE is
  followed by fetext 334 + mode (MONO, STEREO, SURROUND) and VOICES by
  fetext 306 + on (OFF, ON); the selected one's value blinks (tint 20 for
  31 ticks, then 200, out of every 101).

Still to read: the campaign's other screen types (team pages, type 4;
career path, types 0 and 13; loading and saving, types 8 and 9: `pcsav`; level choice, type 20; field conditions, type 21; saving
`pcsav`; the keys `pckey*`), the steam puffs (`FUN_00419fa0`) and the
leaving animations.

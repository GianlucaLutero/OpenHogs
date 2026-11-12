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

and the number of entries of each at 0x4C0EE8. Screen 1 is the main menu
(type 1, four entries: fetext 8 MAIN MENU, then 13-16 ONE PLAYER,
MULTI-PLAYER, OPTIONS, QUIT APPLICATION; the PC replaces the console's
entries 9-12).

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

Still to read: the other screen types (level choice `lselpc`, volume
`vol00`-`vol06`, the team pages `sq*`, the yes/no dial `yesno01`-`06`,
saving `pcsav`, the keys `pckey*`), the steam puffs (`FUN_00419fa0`) and
the leaving animations.

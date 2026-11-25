# The interface in the footage (block `video-ui`)

Notes from the playthrough indexed in [index.md](index.md), compared with
OpenHogs and with the PC executable. Times are *part + time in the part*.

## Part 1: the labels over the hogs and the match messages

**What the footage shows** (part 3, 0:50–1:52):

- every hog on the screen has a label, not only the hog in play: its name
  or its rank in turn (TOTUZZO / RECLUTA for the player's, LE CONT /
  RECLUTA for an enemy), and under it a pink heart and its health;
- the labels are in the colour of the hog's nation (the player's yellow,
  the French enemies' blue), in thin, tall letters;
- a label that would cover another stands beside its hog, with a curved
  arrow in the same colour pointing at it (1:14, 2:05);
- enemies have names too, from their nation's list (LE CONT, GUSTAV);
- the match messages (IL NEMICO CONTRATTACCA...) are in the tall, narrow
  letters of the menus, in the playing nation's colour.

**What the executable says.** The PC version has the same labels:
`FUN_00459b20`, called by the HUD routine `FUN_00457840`, goes through
every hog, keeps those on the screen (alive, or driving a vehicle; an
espionage hog — classes 8–10 — only in its own team's turn), sorts them
from the farthest to the nearest, and draws each with `FUN_0045a5c0`:

| What | Value |
|------|-------|
| font | `FEText/CHARS2` (with `chars2L` and `chars2D`), the label font object `DAT_0051ba54` |
| box | 70 × 48 pixels at (x − 35, y − s/2 − 48), where (x, y) is the hog's projected centre and s = 182400 / depth its square |
| moved box | 78 pixels wide, beside the hog's square: 5 px right or 75 px left of it, 73 px above or 5 px below its centre |
| arrow | `FACETIMS.MAD` entry 4, `arrow000`, 44 × 30, mirrored by giving it a negative size, only while the labels are fully shown |
| text | the hog's name when bit 7 of the match clock in hundredths is set (1.28 s each), its rank otherwise (`0x4CF008` in the campaign, `0x4CF050` elsewhere: gtext numbers); then `/C` colour codes, a heart and the health (`+0x4C >> 7`) |
| health | not shown in the Boot Camp (campaign mission 0, `DAT_0051f17b`) |
| colour | the nation's, VA `0x4CF1E0` (British green, Americans cyan, French blue, Germans white, Russians red, Japanese yellow, Team Lard magenta) |
| fading | `+0xC94` rises by 25 to 255 each tick while the camera stands still (position and angles unchanged, or the overview cameras 7 and 9) and falls by 40 while it moves |

The messages (`FUN_0045b8b0`) are written with the other font object,
`DAT_0051ba58` = `FEText/CHARS3`, centred at y = 140.

**OpenHogs now** draws the labels this way (`ui::HogLabels`,
`OriginalHud::drawHogLabel`) and the messages in CHARS3 (decisions 36,
69 and 148 settled, details left open in decision 175).

**Seen on the way.** The PlayStation's power bar is a horizontal tube at
the bottom of the screen (2:05), where the PC's is the brass gauge
OpenHogs draws from the executable's tables; the damage numbers rise and
fade as decision 67 has them; the label's health runs down a few points a
second after a hit instead of dropping at once (for `video-weapons`).

## Part 2: the pause menu, the instruments, the briefing

**The pause menu** (part 8, 2:06–2:08): a brown-framed panel over the
match with -GIOCO IN PAUSA-, CONTINUA, VOLUME GENERALE and VOLUME
EFFETTI with red bars, VOCI MAIALI ON, VIBRATION ON and ABBANDONA
MISSIONE; the selected item is white. The PC executable has the same
panel (`FUN_0045a9b0`, see [hud.md](../formats/hud.md#pause-menu)) without
the PlayStation's vibration; OpenHogs' plain menu box gave way to it,
with the volumes and the speech switch working and saved (decision 177).

**The instruments** ease in along the executable's own table (`FUN_00458a80`),
which settles decision 64: they wait two ticks, rise and bounce.

**The briefing**: in the footage a loading bar runs along the bottom of
the briefing before the mission can start (part 2, 0:38). The PC draws it
too (`FUN_0045c460`): OpenHogs now fills it and waits for it (decision 176).

# Fonts and menu pictures

How the PC version stores the letters and pictures of its menus and HUD,
and what OpenHogs does with them. The text encoding itself (which byte is
which letter) is in [text.md](text.md).

## Bitmap fonts: `FEText/*.tab` + `*.BMP`

Each font is two files: a table of character rectangles and a picture
holding all the characters (an "atlas").

| Font | Picture | Letter height | Accents | Used by OpenHogs for |
|------|---------|---------------|---------|----------------------|
| `BIG.tab` / `BIG.BMP` | 256 × 160, 4-bit | 32 | yes | titles, menu entries, messages, HUD names and numbers |
| `SMALL.tab` / `SMALL.BMP` | 256 × 48, 4-bit | 12 | yes | small HUD labels (team name, health) |
| `chars3.tab` / `CHARS3.BMP` | 256 × 120, 8-bit | 30 | yes | – |
| `GameChars.tab` / `GameChars.bmp` | 256 × 48, 8-bit | 12 | **no** | – |
| `BigChars.tab` / `BigChars.BMP` | 8-bit | – | **no** | – |

`BIG.tim` and `SMALL.tim` (and `Language/Tims/FONTTIMS.MAD`) hold the same
fonts as PlayStation textures.

### The `.tab` table

A list of 8-byte entries, one per character code (entry `0x22` is `A`):

| Offset | Type | Meaning |
|--------|------|---------|
| 0 | int16 | x of the character in PlayStation video memory |
| 2 | int16 | y |
| 4 | int16 | width in pixels |
| 6 | int16 | height in pixels |

Codes without a character have width and height 0; this includes the space
(code `0x01`), whose width is not stored anywhere (OpenHogs advances by a
third of the letter height, see `docs/open-decisions.md`).

The positions are where the PlayStation kept the picture in its video
memory, not positions in the `.BMP`. The picture's top-left corner is the
smallest x and y used by any character: BIG (960, 320), SMALL (896, 166),
chars3 and GameChars (960, 90), BigChars (0, 0). Subtracting that corner
gives atlas positions; drawing every character that way lines them up
exactly with the picture.

### The picture

A standard uncompressed Windows BMP with a palette. Palette entry 0 is
the background (black in BIG/SMALL/chars3, magenta in GameChars) and is
drawn transparent; the letters are light with a dark outline, so they can
be tinted (OpenHogs multiplies them by the text colour).

The accented capitals of the European versions (À, È, Ì, Ò, Ù, Ç, Ñ, Ä…)
are in BIG, SMALL and chars3, which is why OpenHogs uses those: Italian
words such as MODALITÀ, SÌ, GIÙ render correctly. GameChars and BigChars
lack them.

## Menu pictures: `Language/Tims/MENUTIMS.MAD`

A package (see [mad-mtd.md](mad-mtd.md)) of BMP pictures. Entry 0 is
`menu.bmp` (257 × 274); entries 1 to 67 are the 56 × 34, 8-bit icons of
the weapons and actions, **in the order of the weapon entries of the game
text table**: entry 1 `TROTTER.BMP` is text entry 97 (TROTTER), entry 2
`KNIFE.BMP` is 98, … entry 37 `TNT.BMP` is 133, up to `SURREND.BMP`
(157, SURRENDER) and `SELECT.BMP`. Since OpenHogs numbers weapons by
their text entry, the icon of weapon *id* is entry *id* − 96.

The icons have a near-black background (palette entry 0 is `#000002`),
drawn as it is: it frames the icon in the HUD.

## Full-screen pictures: `FEBmps/FEBMP.MAD` (not decoded yet)

237 `.mgl` pictures: the menu backgrounds (`pigbkpc1`, 640 × 480),
`fullmenu`, the title screens `title1`–`title6`, nations, uniforms, the
newspapers and cards of the campaign. Each is a compressed BMP. What is
known so far of the compression, a byte-oriented LZ scheme:

| First byte | Meaning |
|------------|---------|
| `00`–`1F` | a run of literal bytes: the count, then that many bytes |
| `60`–`7F` | repeat the previous byte `(byte & 0x1F) + 3` times (`61` → 4, `62` → 5) |
| `80`–`9F` | copy 3 bytes from `byte & 0x1F` bytes back |
| `C0`–`DF` | a longer form with one extra byte, not understood yet |

Until it is decoded, the menus are drawn over a map seen from a slowly
circling camera instead (`docs/open-decisions.md`).

## Menu sounds: `FESounds/Fesounds.srl`

A sound bank (see [audio.md](audio.md)) of 27 sounds: `click1`–`click5`,
`cog`, `hiss1`/`hiss2`, `steam1`, the `Indu*` industrial noises,
`COINDROP`, `COINFLIP`, `Crunch`, `Promo`… OpenHogs uses `click1` for
moving in a menu, `click5` for picking and `Indu006` for going back.

## Menu and HUD texts

The words of the menus come from `Language/Text/fetext`, those of the
match from `Language/Text/gtext` (entry numbers below), so a language
pack translates them. Each has an English default in
`src/ui/include/openhogs/ui/UiStrings.h`.

| Table | Entries |
|-------|---------|
| fetext | 6 YES, 7 NO, 8 MAIN MENU, 9 ONE PLAYER, 10 MULTI-PLAYER, 11 OPTIONS, 16 QUIT APPLICATION, 74 VOLUME CONTROL, 76 MASTER, 77 SFX, 78 VOICES, 105 MULTI-PLAYER SELECT LEVEL, 134 START GAME, 330 PLAYER, 331 CPU, 350 BACK, 465 LOADING, PLEASE WAIT., 648 MUSIC |
| fetext | team names: 25 TOMMY'S TROTTERS (British), 26 GARLIC GRUNTS (French), 27 UNCLE HAMS HOGS (American), 28 PIGGYSTROIKA (Russian), 29 SUSHI-SWINE (Japanese), 30 SOW-A-KRAUTS (German), 226 TEAM LARD |
| gtext | 165 VICTORY TO >S!!, 166 IT'S A DRAW!!, 168 GET READY >S..., 169 THE ENEMY RETALIATES... (a computer team's turn), 173 -GAME PAUSED-, 180 ABORT SKIRMISH, 181 CONTINUE, 182 ARE YOU SURE? |
| gtext | ranks: 63 GRUNT, 64 GUNNER, 68 SAPPER, 70 SABOTEUR, 72 SNIPER, 73 SPY, 75 MEDIC, 78 ACE, 79 LEGEND (and the others in between) |

In messages, `>S` is replaced by a name and `>N` by a number; a digit in
between (`>2N`, as in "MISSION >2N : >S") is taken by OpenHogs as a
zero-padded width.

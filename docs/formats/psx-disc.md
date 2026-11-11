# PlayStation disc (European release, SLES-01041)

OpenHogs can take content from **your own** copy of the PlayStation
version: the European release (serial SLES-01041) contains the game in
English, French, German, Italian and Spanish. Reader:
`src/formats/PsxDisc`; pack builder: `src/tools/langpack`.

## Disc image

- Usual dumps are three tracks: track 1 is data, tracks 2 and 3 are CD audio
  (music). Only track 1 is needed; a `.cue` file is not.
- Raw images (`.bin`) have 2352-byte sectors that start with a 12-byte sync
  pattern. PlayStation data sectors are *Mode 2 Form 1*: 2048 bytes of data
  at offset 24 (after the 16-byte header and the 8-byte subheader).
- Plain `.iso` images have 2048-byte sectors.
- Files are found through the standard ISO 9660 file system: primary volume
  descriptor in sector 16, root directory record at offset 156, names with a
  `;1` version suffix.

## Language content on the disc

| Disc path | Content | PC equivalent |
|-----------|---------|---------------|
| `TEXT/G<LANGUAGE>.BIN/.OFS` | in-game text, 272 strings | `Language/Text/gtext.bin/.ofs` (same count) |
| `TEXT/FETEXT/<LANGUAGE>.BIN/.OFS` | menus, 770 strings | `Language/Text/fetext.bin/.ofs` (786 strings) |
| `BRIEF/<LANGUAGE>/LEVEL<n>.BIN` | briefings, raw 15-bit, 512 x 256 | `Language/Tims/Briefing/Level<n>.BMP`, 640 x 480 |
| `BRIEF/<LANGUAGE>/LEVEL1N<k>.BIN` | small pictures of mission 1, raw 15-bit, 202 x 100 | `Level1n<k>.BMP`, 254 x 190 |
| `NEWS/<LANGUAGE>/`, `CARTE/<LANGUAGE>/`, `LOAD/*ITA.MGL`, `FE/MADITA.MAD` | newspapers, cards, title screens, menu graphics | not converted yet (`.MGL` format unknown) |
| `TEXT/FETEXT/CHARSIT.TIM/.TAB` | Italian font | not needed: the PC fonts already have the accents |
| `SPEECH/SKU1/` | speech, **English only** | `Speech/Sku1/` |

`<LANGUAGE>` is `ENGLISH`, `FRENCH`, `GERMAN`, `ITALIAN` or `SPANISH`.
The six speech sets in `SPEECH/SKU1` (`AM`, `EN`, `FR`, `GE`, `JA`, `RU`)
are the national accents of the hog armies, not languages: the
PlayStation release had translated text with English voices.

Raw pictures have no header: their sizes were found by decoding the files
(262,144 bytes = 512 x 256 x 2; 40,400 bytes = 202 x 100 x 2) and checking
the result against the PC pictures, which have the same layout.

## Matching the text tables

The text encoding is the same on both versions (see [text.md](text.md)).
The tables differ in a few entries:

- **gtext** (272 strings): identical except 13 tutorial hints that name the
  pad buttons (`PRESS >X BUTTON...`) where the PC names keys
  (`PRESS SPACE...`), and one mission name left as `TEXT MISSING` in the
  Italian table.
- **fetext** (770 vs 786 strings): 751 strings are identical; the PC has
  extra entries (network game, key names) and different credits.

`openhogs-langpack` pairs the strings of the two **English** tables with a
longest-common-subsequence match, then gives each paired PC string the
translation of its PlayStation partner. Strings without a partner stay in
English and are listed in the report; `content/translations/<lang>.ini`
translates them by hand. With the Italian overrides shipped with OpenHogs,
only people's names and a symbol remain in English.

## Quirk of the PC data

`Language/Tims/Briefing/level24.BMP` on the PC uses the old 12-byte OS/2
BMP header (16-bit width and height) instead of the usual 40-byte one.

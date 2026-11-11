# Text tables and text encoding

Used for: every string shown by the game. Parsers:
`src/formats/TextTable` and `src/formats/GameText`.

## Files

| Table | Content | Strings (English) |
|-------|---------|-------------------|
| `Language/Text/gtext.ofs` + `.bin` | in-game text: missions, weapons, results | 272 |
| `Language/Text/fetext.ofs` + `.bin` | front-end menus | 786 |

- `.ofs`: one little-endian `uint16` per string, the offset of the string
  inside the `.bin` file.
- `.bin`: the strings, each terminated by a zero byte, stored in order.

The engine refers to strings by index, so a translation must keep the same
number of entries in the same order. Offsets are 16-bit, so a `.bin` file is
limited to 64 KiB (the English files use 4 and 8 KiB).

## Encoding

Bytes are indices into the bitmap fonts, which have **upper-case letters
only**:

| Code | Meaning |
|------|---------|
| `0x00` | end of string |
| `0x01` | space |
| `0x02`–`0x3E` | ASCII `0x21`–`0x5D` (`!` … `Z`, `[`, `\`, `]`): ASCII = code + `0x1F` |
| `0x3F`–`0x4B` | icons drawn by the font (dots, symbols, PlayStation buttons) |
| `0x4C`–`0x68` | accented capitals and symbols, see below |

Examples: `A` = `0x22`, `0` = `0x11`, `.` = `0x0F`, `'` = `0x08`.

Accented capitals and symbols (European versions):

| Code | Char | Code | Char | Code | Char | Code | Char |
|------|------|------|------|------|------|------|------|
| `4C` | ¡ | `54` | Ä | `5C` | Î | `64` | Ù |
| `4D` | © | `55` | Ç | `5D` | Ï | `65` | Ú |
| `4E` | ® | `56` | È | `5E` | Ñ | `66` | Û |
| `4F` | ¿ | `57` | É | `5F` | Ò | `67` | Ü |
| `50` | À | `58` | Ê | `60` | Ó | `68` | ß |
| `51` | Á | `59` | Ë | `61` | Ô | | |
| `52` | Â | `5A` | Ì | `62` | Õ | | |
| `53` | Ã | `5B` | Í | `63` | Ö | | |

How this was established: every glyph of the PC font `FEText/BIG.BMP` was
drawn using the rectangles in `FEText/BIG.tab` (one 8-byte entry per code:
x, y, width, height in PlayStation video memory, where the picture starts at
x = 960, y = 320), and the result was checked against real words of the
PlayStation translations: Italian MODALITÀ, SÌ, GIÙ; French FRANÇOIS;
German SANITÄTER, TRÖGE, WÜSTE; Spanish INVASIÓN, MAÑANA, ¡VICTORIA.

OpenHogs writes text as UTF-8 with these characters, and the remaining icon
codes as `{xx}` (two hex digits), so that decoding and re-encoding gives
back the original bytes (verified on all tables of the PC and the five
languages of the PlayStation disc).

## Special entries

- **Entry 0** of both tables is not text: it lists the glyphs a font
  provides, in font order (e.g. `AÀÁÂÃÄBCÇDEÈÉÊË…`). It was the key to
  the accent table above.
- Strings starting with `>` (e.g. `>N`, `>S`, `>L2`) appear to be
  placeholders or control sequences replaced at runtime (names, numbers).

## Extracted form

`openhogs-extract` and `openhogs-text export` write each table as UTF-8
text, one string per line (`openhogs-text import` converts it back):

```
[0040] DEATH VALLEY
[0041] CHILL HILL
```

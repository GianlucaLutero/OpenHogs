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

Bytes are indices into the bitmap font, which has **upper-case letters
only**:

| Code | Meaning |
|------|---------|
| `0x00` | end of string |
| `0x01` | space |
| `0x02`–`0x3B` | ASCII `0x21`–`0x5A` (`!` … `Z`): ASCII = code + `0x1F` |
| `0x3C` and above | extra glyphs (see below) |

Examples: `A` = `0x22`, `0` = `0x11`, `.` = `0x0F`, `'` = `0x08`.

OpenHogs writes codes it cannot map as `{xx}` (two hex digits), so that
decoding and re-encoding gives back the original bytes. This was verified on
both English tables: the re-encoded files are byte-for-byte identical.

## Special entries

- **Entry 0** of both tables is not text: it looks like the list of glyph
  codes the font provides. In `gtext` it contains, besides A–Z, digits and
  punctuation, the codes `0x3C`, `0x3E`–`0x41` and `0x4C`–`0x68`; `fetext`
  also has one string that lists the same extra codes. So the font has
  about 30 extra glyphs beyond A–Z. These
  are probably the accented letters used by the French, German and Spanish
  releases, and are what an Italian translation needs (À, È, É, Ì, Ò, Ù).
  Mapping each code to its letter requires looking at the font texture; this
  is planned for the language-pack work.
- Strings starting with `>` (e.g. `>N`, `>S`, `>L2`) appear to be
  placeholders or control sequences replaced at runtime (names, numbers).

## Extracted form

`openhogs-extract` writes each table as UTF-8 text, one string per line:

```
[0040] DEATH VALLEY
[0041] CHILL HILL
```

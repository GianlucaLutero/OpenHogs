# `.mgl` compressed pictures

The full-screen pictures of Hogs of War (menu backgrounds, title screens,
newspapers, the campaign cards) are stored compressed in files ending in
`.mgl`. Both releases use the same compression; what is inside differs.

Read by `formats::decompressMgl` and `formats::decodeMglPicture`
(`src/formats/include/openhogs/formats/MglPicture.h`).

## Compression

Read from the PC executable (`FUN_004989d0`, see
[executable.md](executable.md)). It is a byte-oriented LZ scheme with
run-length and delta commands. The stream is a series of commands, each
starting with one command byte `b`; a command byte of `00` ends the
stream. "Back" means the bytes already written to the output.

| Command byte | Meaning |
|--------------|---------|
| `01`–`3F` | copy the next `b` bytes as they are (literals) |
| `40`–`4F` | `(b & 0x0F) + 3` bytes continuing the step between the last two bytes (a gradient) |
| `50`–`5F` | `(b & 0x0F) + 2` 16-bit values continuing the step between the last two 16-bit values |
| `60`–`6F` | repeat the last byte `(b & 0x0F) + 3` times |
| `70`–`7F` | repeat the last 16-bit value `(b & 0x0F) + 2` times |
| `80`–`BF` | copy 3 bytes from `(b & 0x3F) + 3` bytes back |
| `C0`–`DF` | copy `((b >> 2) & 7) + 4` bytes from `(((b & 3) << 8) \| next) + 3` bytes back |
| `E0`–`FF` | two more bytes `d`, `l`: copy `l + 5` bytes from `(((b & 0x1F) << 8) \| d) + 3` bytes back |

Copies may overlap the bytes they produce (a distance shorter than the
length repeats a pattern), so they are made byte by byte. The gradient
commands suit the smooth shading of 15-bit pictures.

OpenHogs refuses a stream that reads past its end, copies from before the
start of the output or grows above 16 MB (`core::FormatError`).

## PC: `FEbmps/FEBMP.MAD`

A MAD archive ([mad-mtd.md](mad-mtd.md)) of 237 `.mgl` entries. Each
decompresses to a complete Windows BMP (8 or 24 bits per pixel). Pure
magenta marks transparent pixels: (255, 0, 255) or, in most pictures, its
15-bit form (248, 0, 248); `decodeMglPicture` turns
it into alpha 0.

| Entries | What they are |
|---------|---------------|
| `pigbkpc1` (640 × 480) | the background of every menu |
| `fullmenu`, `lselpc`, dials, plates | the menu panels and their parts (block `original-menus`) |
| `title1`–`title6` | the title plates of the menus |
| nations, uniforms, ranks | the pictures of the team and squad pages |
| newspapers and cards | the campaign's pages between missions |

OpenHogs draws `pigbkpc1` behind the menus (`MenuBackground`), covering
the window with its proportions kept.

## PlayStation disc

The disc keeps its pictures as loose `.mgl` files. They decompress to
raw data without a header, so the size is known only from the file name:

| Files | Content |
|-------|---------|
| `CARTE/*.MGL` | the campaign cards; each decompresses byte for byte to the `.BIN` file next to it (checked on ARSPHY and POLITIC) |
| `LOAD/SPLSH<lang>.MGL` | the colour title screen ("Born to Grill"), raw 15-bit, 640 × 256 |
| `LOAD/TIT<lang>.MGL` | the sepia title screen, raw 15-bit, 640 × 256 |
| `NEWS/<lang>/PAPERS/DAILY{ENG,YAN,FRG,GER,RUS,JAP,LRD}.MGL` | the seven newspapers (British, American, French, German, Russian, Japanese, Team Lard), raw 15-bit, 640 × 256 |
| `NEWS/<lang>/TEXTA/TEXT%03dA.MGL`, `TEXTB/TEXT%03dB.MGL` | the newspaper headlines: 4-bit TIMs ([tim.md](tim.md)) of 164 × 108 and 168 × 108, the left and right halves of headline *n* |
| `LOAD/TITLE2.MGL` | another format, not decoded (not needed) |

The PlayStation screen is 640 × 256 and is shown stretched to the TV's
height. The PC has the same pictures at its own sizes:

| PC file | PlayStation source |
|---------|--------------------|
| `Title/title.bmp` | `LOAD/SPLSH<lang>.MGL` |
| `Title/titlemon.bmp` | `LOAD/TIT<lang>.MGL` |
| `Papers/British.bmp` … `Papers/teamlard.bmp` | `DAILY*.MGL` |
| `Papers/text<n>.bmp` (314 × 204, red key colour) | `TEXT<n>A` + `TEXT<n>B` side by side |

`openhogs-langpack` uses this table to put the translated title screens,
newspapers and headlines of the European disc into a language pack, each
resized to the PC picture it replaces (see the README).

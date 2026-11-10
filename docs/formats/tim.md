# TIM images

Used for: almost all textures (characters, weapons, map objects). The PC
version kept the PlayStation format. Decoder: `src/formats/TimImage`.

In the Steam/GOG data there are about 9,300 TIM images, all inside MAD/MTD
archives: 9,163 are 4-bit and 166 are 8-bit, all with a palette.

## Layout

All numbers are little-endian.

```
uint32  magic = 0x10
uint32  flags        bits 0-2: pixel mode (0 = 4-bit, 1 = 8-bit, 2 = 16-bit, 3 = 24-bit)
                     bit 3:    a palette (CLUT) block follows
[palette block]      only if bit 3 is set
pixel block
```

Palette block and pixel block share the same header:

| Type     | Meaning |
|----------|---------|
| `uint32` | Block length in bytes, header included |
| `uint16` | X position in PS1 video memory (ignored) |
| `uint16` | Y position in PS1 video memory (ignored) |
| `uint16` | Width |
| `uint16` | Height |

- Palette block: `width` colours per row, `height` rows of 16-bit colours.
  Each row is one palette; the extractor uses the first.
- Pixel block: **width counts 16-bit words**, not pixels. One word holds
  4 pixels in 4-bit mode, 2 in 8-bit mode, 1 in 16-bit mode.
- 4-bit pixels: two per byte, **low nibble first**.

## Colours

16-bit colour: bits 0–4 red, 5–9 green, 10–14 blue, bit 15 semi-transparency
flag. Channels are expanded to 8 bits as `(v << 3) | (v >> 2)`.

Transparency follows the PlayStation rule: the colour `0x0000` is fully
transparent, every other colour is opaque. The game relies on this for
cut-out textures such as trees and fences.

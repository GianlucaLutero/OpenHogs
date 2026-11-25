# PlayStation speech (`SPEECH/SKU1/*.IXA`)

The PlayStation discs keep every spoken line in interleaved CD-ROM XA
audio files, one file for a set of lines and one line per channel. The
language pack builder (`openhogs-langpack --speech`, decision 139) decodes
them (`formats::decodeXaChannel`) and writes each line under the PC's own
file name, so the dubbed voices of a localised PAL release take the place
of the PC's English ones.

## Files

Found on the European disc (SLES-01041), whose voices are the PC's English
ones:

| Disc file | Lines | PC files |
|-----------|-------|----------|
| `SPEECH/SKU1/PIGnn/nnAA01.IXA` | 30 of 32 channels | `Speech/Sku1/Pignn/nnAAGGVV.wav` |
| `SPEECH/SKU1/SARGE/SGEN01.IXA` … `SGEN06.IXA` | 32 a file (06: 15) | `Speech/Sku1/Sarge/SGENSSVV.wav` |
| `SPEECH/SKU1/TRAIN1/TR1_EN.IXA` | 28 of 32 | `Speech/Sku1/Train1/tr1_enNN.wav` |
| `SPEECH/SKU1/MUSIC1/MU_EN01.IXA` | music | not used |

- **Hog voices**: nine voices (`nn` 01–09), six accents each (`AA`: AM,
  EN, FR, GE, JA, RU; the PC writes some in lower case, `fr`, `ge`), and
  the line of group `GG` (1–5) and variant `VV` (1–6) is channel
  (GG − 1) × 6 + VV − 1. Channels 30 and 31 are empty.
- **Sergeant**: line i = (SS − 1) × 8 + VV − 1 for section SS (1–22) and
  variant VV (1–8, section 22 has 7) is channel i mod 32 of
  `SGEN0<i / 32 + 1>.IXA`.
- **Boot Camp instructor**: line NN is channel NN − 1. The PlayStation's
  instructor names the pad's buttons, and several of its lines are longer
  or shorter than the PC's (a different recording).

Every line is 4-bit mono ADPCM at 18,900 samples a second (coding byte
0x04). Each channel's sectors stand together in the file's interleave (20
to 31 sectors a channel in the hog files); a line ends in silence, which
the builder trims.

Checked: all 1,823 PC lines found their disc line; the hog and sergeant
lines are as long as the PC's within a quarter of a second (the PC's
files keep a little more silence at the end), and decoded lines resampled
to 22,050 Hz correlate with the PC's at 0.95 to 0.997.

## XA sectors

Mode 2 Form 2 sectors of 2352 bytes: sync (12), header (4), subheader
(file number, channel, submode, coding; 4 bytes, then a copy), 18 sound
groups of 128 bytes, padding. Submode bit 2 marks audio. Coding: bit 0
stereo, bit 2 half rate (18,900 Hz instead of 37,800), bit 4 eight-bit
samples. A sound group has 16 header bytes (each unit's filter in bits
4–5 and range in bits 0–3, at byte 4 + unit) and 112 data bytes: 8 units
of 28 four-bit samples (unit u's sample j in byte 16 + u / 2 + 4j, low
nibble for even u) or 4 units of 8-bit samples. A sample is its value
placed in the top bits of 16, shifted right by the range, plus
(old × K0 + older × K1 + 32) / 64 with (K0, K1) = (0, 0), (60, 0),
(115, −52), (98, −55). The ISO 9660 directory counts 2048 bytes for each
of these sectors, so an `.iso` image (2048-byte sectors) cannot hold them:
speech needs the raw `.bin`.

Source: Martin Korth's *Nocash PSX Specifications* (CDROM XA audio ADPCM),
checked against the disc as above.

## Localised releases

The dubbed releases are SLES-02766 (French), 02767 (German), 02768
(Spanish) and 02769 (Italian), told apart by the boot file in
`SYSTEM.CNF`. None of them was available when the builder was written: it
assumes they keep the same files and channels, warns about any file it
does not find (those lines stay English), and builds a pack with the
speech alone if the disc has no text in the European layout (decision
179).

# Bink videos

The cutscenes of the PC release are Bink 1 videos in `Language/Streams/`
(`InfoLogo.bik`, `Sheff.bik`, `FMV 01.bik` … `FMV 09.bik`; `FMV 02` and
`FMV 09` are 1-byte placeholders). All are revision `f`, 320 × 240 at 25
frames per second, with one stereo 44.1 kHz audio track of the RDFT
variant. When they play: `docs/formats/executable.md`.

OpenHogs decodes them with its own code in `src/video`, written from
FFmpeg's decoder (`libavcodec/bink.c`, `binkdsp.c`, `binkaudio.c`,
`libavformat/bink.c`, LGPL 2.1 or later), which served as the
specification; the codec's constant tables are copied from FFmpeg's
`binkdata.h` with their notice. All the game's videos decode completely
and their sound has the length of their pictures.

## Container

All numbers are little-endian 32-bit unless noted.

| Offset | Content |
|--------|---------|
| 0 | `BIK` and the revision letter (`b` … `i`) |
| 4 | file size − 8 |
| 8 | number of frames |
| 12 | largest frame, in bytes |
| 16 | number of frames again |
| 20, 24 | width, height |
| 28, 32 | frame rate: numerator, denominator |
| 36 | video flags (0x00100000 alpha, 0x00020000 grey) |
| 40 | number of audio tracks *n* |
| 44 | *n* × largest decoded audio size |
| | *n* × (sample rate: 16 bits, flags: 16 bits — 0x2000 stereo, 0x1000 DCT variant, 0x4000 16 bits) |
| | *n* × track id |
| | number of frames × offset of the frame (lowest bit set: key frame) |

Each frame is, for each audio track, a 32-bit size and that many bytes of
audio, then the picture up to the next frame.

## Pictures

Bits are read least significant first. A picture is three planes, Y at
full size and U, V at half size (`h` and later store V first), each
starting on a 32-bit boundary. A plane begins with nine "bundles", each
with a Huffman tree chosen among 16 fixed ones (and, for colours, 16 more
trees for the high nibble, chosen by the previous high nibble): block
types, 16 × 16 block types, colours, patterns, X and Y motion, intra and
inter DC values, run lengths. Before each row of 8 × 8 blocks, each
bundle may add a batch of values (only when its previous ones are used
up). Block types:

| Type | Block |
|------|-------|
| 0 skip | copied from the previous picture |
| 1 scaled | a 16 × 16 block: an 8 × 8 block of type run, intra, fill, pattern or raw drawn at double size |
| 2 motion | copied from the previous picture at an offset |
| 3 run | runs of colours along one of 16 scan orders |
| 4 residue | a moved copy plus small corrections |
| 5 intra | an 8 × 8 DCT block |
| 6 fill | one colour |
| 7 inter | a moved copy plus a DCT block |
| 8 pattern | two colours chosen by 8 rows of 8 bits |
| 9 raw | 64 colours |

DCT coefficients are coded by bit planes, refining lists of candidate
coefficient groups, then multiplied by one of 16 quantisation tables
(11 fractional bits) and transformed with an integer IDCT. Pixels add and
overflow modulo 256 like the original. The YUV values are in the limited
"TV" range (BT.601).

## Sound

Each audio packet starts with the size of the decoded sound, then blocks
(each ending on a 32-bit boundary). The RDFT variant codes the
interleaved stereo samples as one channel at twice the rate, so a block
holds 4,096 frequency coefficients: two packed floats (5-bit exponent,
23-bit mantissa, sign) for the constant and the highest frequency, an
8-bit quantiser index per critical band (25 bands), then runs of 8 × *k*
coefficients sharing a bit width. An inverse real Fourier transform
turns them into 4,096 samples; the first 256 are cross-faded with the
last 256 of the previous block and 3,840 samples are output.

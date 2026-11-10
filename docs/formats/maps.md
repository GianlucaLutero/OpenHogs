# Maps: PMG, POG, PTG (+ MAD, MTD)

Each map in `Maps/` is a set of files with the same name:

| File | Content | Parser |
|------|---------|--------|
| `NAME.PMG` | terrain: heights, lighting, tiles | `src/formats/MapTerrain` |
| `NAME.PTG` | terrain textures | `src/formats/TextureTable` |
| `NAME.POG` | placed objects | `src/formats/MapObjects` |
| `NAME.MAD` | models of the objects | `src/formats/PackageArchive` + `Model` |
| `NAME.MTD` | textures of the models | `src/formats/PackageArchive` + `TimImage` |

Layouts come from the OpenHoW documentation (see Credits in the README);
the conventions marked *verified* were established on the Steam data.

## PMG: terrain (94,208 bytes)

A map is **16 x 16 blocks**; a block is 2048 x 2048 world units with a
**5 x 5 grid of height samples** and **4 x 4 tiles** of 512 x 512 units.
Every PMG file of the game has exactly 256 blocks of 368 bytes.

Block (368 bytes):

| Type | Meaning |
|------|---------|
| `int16` | X offset of the block (-16384 ... 14336) |
| `int16` | Y offset (unreliable, unused) |
| `int16` | Z offset of the block |
| `int16` | unknown |
| 25 x (`int16`, `uint16`) | height and baked lighting (0-255) of each sample, row by row |
| `uint32` | always 0 |
| 16 x tile (16 bytes) | tiles, row by row |

Tile (16 bytes):

| Type | Meaning |
|------|---------|
| `int16` x 3 | always 0 |
| `uint8` | type: bits 0-4 surface (0 mud, 1 grass, 2 metal, 3 wood, 4 water, 5 stone, 6 rock, 7 sand, 8 ice, 9 snow, 10 quagmire, 11 lava), bit 5 water, bit 6 mine, bit 7 wall |
| `uint8` | slip direction (walls) |
| `int16` | always 0 |
| `uint8` | orientation: 1 = flip X, 2 = rotate 90°, 4 = rotate 180° |
| `uint32` | texture index into the PTG |
| `uint8` | always 0 |

**Orientation of rows (verified).** World Y is the height, larger values
are higher. Inside a block, sample row 0 lies at `z = zOffset + 2048` and
each next row is 512 units closer to `zOffset`; columns go towards +X from
`xOffset`. This was determined by comparing object heights from the POG
files with the terrain height below them on four maps: only this
orientation makes objects sit on the ground (median error 64-160 units,
against 500+ for the alternatives).

**Lighting (verified visually).** Dividing the baked lighting by 255 gives
colours close to the original; dividing by 128 over-exposes the textures.

## PTG: terrain textures

```
uint32  count
        count TIM images, all the same size in bytes
```

In the game data every image is a 32 x 32, 4-bit TIM of 576 bytes.

## POG: objects

```
uint16  count
        count records of 94 bytes
        (some files have 2 extra bytes at the end)
```

Record (94 bytes), main fields:

| Offset | Type | Meaning |
|--------|------|---------|
| 0 | `char[16]` | model name (files `NAME.VTX` etc. in the map's MAD) |
| 16 | `char[16]` | always `NULL` |
| 32 | `int16` x 3 | position X, Y (height), Z — same space as the terrain |
| 38 | `int16` | id |
| 40 | `int16` x 3 | rotation around X, Y, Z in 1/4096 of a turn |
| 46 | `int16` | type id |
| 48 | `int16` x 4 | bounding box size and type |
| 56 | `int16`, `uint8` | unknown |
| 59 | `uint8` | team bit mask (01, 02, 04... one bit per team) |
| 60 | `int16` | script type (mission objective role, e.g. 20 = tutorial target) |
| 62 | `uint8` | script group |
| 63 | 19 bytes | script parameters |
| 82 | `int16` x 3 | script position |
| 88 | `uint16` | flags (bit 0 = player hog) |
| 90 | `int16` x 2 | unknown |

**Field offsets from 56 on (verified).** The OpenHoW documentation is
ambiguous about the size of the field at offset 56; the game data settles
it: in `ICE.POG` the 20 hogs have team masks 01, 02, 04 and 08 (five each)
at offset 59, `CAMP.POG` has tutorial script types 20 and 23 at offset 60,
and the hogs' flags at offset 88 are 01 ("player").

Objects whose model is not in the map's MAD are logical objects. The
most common are `GR_ME` and `CO_ME`, which appear to be the start
positions of the hogs; the map viewer shows them as yellow markers.

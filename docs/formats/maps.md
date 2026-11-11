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
| 48 | `int16` x 3 | collision box size along the model's Z, Y, X axes, in steps of 64 units (*verified*, see below) |
| 54 | `int16` | unknown (1 for some bridge parts) |
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

**Collision boxes (verified).** Comparing the three sizes at offset 48
with the models gives a box in steps of 64 world units, stored Z first:
a crate of 190 × 191 × 253 units has (4, 3, 3), a wall 64 × 512 × 2176 has
(32, 8, 1), a bridge deck 1024 × 64 × 511 has (8, 1, 16). The box is centred
on the object's position and turns with its Y angle. For trees it covers
only the trunk (192 units wide), so hogs can walk under the canopy. OpenHogs
uses these boxes for collisions: hogs walk on top of low boxes (bridges,
floors) and are stopped by tall ones (walls, trunks, crates).

Objects whose model is not in the map's MAD are logical objects; the map
viewer shows them as yellow markers.

### Hogs

The hogs of a map are objects named `<class>_ME`, where the two letters are
the prefix of the class's low-detail model in `Chars/british.mad`
(see [characters.md](characters.md)). Counted over the 61 maps:

| Object | Class | Count |
|--------|-------|-------|
| `GR_ME` | grunt | 631 |
| `HV_ME` | heavy weapons | 62 |
| `SB_ME` | saboteur | 17 |
| `ME_ME` | medic | 17 |
| `CO_ME` | commando | 13 |
| `SN_ME` | sniper | 10 |
| `LE_ME` | legend | 8 |
| `SP_ME` | spy | 8 |
| `AC_ME` | ace | 5 |
| `SA_ME` | sapper | 1 |

- **Team**: the bit mask at offset 59 (01, 02, 04, 08, 10, 20: up to six teams).
- **Facing**: the Y angle, as for every object.
- **Height**: the position is the centre of the model, not the feet. Of
  the 772 hogs, 668 are 160–179 units above the terrain; OpenHogs puts the
  feet on the ground. Hogs placed much higher stand on something (a tower
  in `BOOM`, 2272 units up): those keep their height, minus 168.
- **Commando**: the PC files have no commando model (only `COLO` textures
  that no model uses). OpenHogs draws commandos with the saboteur's model
  until it is known what the original does.
- **Open question**: multiplayer maps such as `BOOM` place several hogs of
  different classes on the same spot (five `SN_ME` and five `GR_ME` of team
  2). They are probably alternatives chosen by the game mode, perhaps through
  the type id (offset 46) or the flags (offset 88); OpenHogs currently
  creates all of them.

# Models: VTX, NO2, FAC

Used for: every 3D object (hogs, weapons, trees, buildings...). Parser:
`src/formats/Model`, mesh building: `src/scene/ModelMesh`.

Layouts below come from the OpenHoW documentation (see Credits in the
README) and were checked against the Steam data: every model of the maps
tested (CAMP, BRIDGE, ICE: 147 models, ~4,300 faces) parses with all
indices in range.

A model is three files with the same name, stored together in a `.MAD`
archive (e.g. `CRATE1.VTX`, `CRATE1.NO2`, `CRATE1.FAC` in `Maps/CAMP.MAD`).
All numbers are little-endian.

## VTX: vertices (8 bytes each, no header)

| Type | Meaning |
|------|---------|
| `int16` x 3 | position X, Y, Z (model units) |
| `uint16` | skeleton bone (characters) |

## NO2: normals (16 bytes each, no header)

| Type | Meaning |
|------|---------|
| `float` x 3 | normal X, Y, Z |
| `float` | bone index (duplicate of VTX) |

## FAC: faces

```
char[16]  unused (zero)
uint32    triangle count
          triangles, 32 bytes each
uint32    quad count
          quads, 36 bytes each
```

Triangle (32 bytes) — fields are grouped by kind:

| Type | Meaning |
|------|---------|
| `uint8` x 6 | U, V of corners A, B, C (in texels) |
| `uint16` x 3 | vertex index of A, B, C |
| `uint16` x 3 | normal index of A, B, C |
| `uint16` | unused |
| `uint32` | texture index (entry number in the accompanying `.MTD`) |
| `uint16` x 4 | unknown |

Quad (36 bytes): same, with four corners and without the unused field.
Corners are listed **around the edge**, so a quad splits into the triangles
A-B-C and A-C-D (checked on `CRATE1`).

## Conventions (verified on the maps)

- **Scale**: models are stored at twice the world scale: multiply by 0.5.
  With this factor crates rest exactly on the terrain.
- **Axes**: model Y points **down** (PlayStation convention), world Y points
  up, so Y is mirrored when placing a model. Without it trees are upside
  down.
- **Texture coordinates** are texel positions; OpenHogs divides
  `(u + 0.5)` by the texture width (and `v` likewise) to sample texel
  centres.
- **Texture index** counts the entries of the `.MTD` archive in order,
  repeated names included.

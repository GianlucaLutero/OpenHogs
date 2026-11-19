# Effects: particles, explosions, shadows

How the PC game (`warhogs_.exe`, Steam v1.2b) makes its effects, read with
Ghidra (`tools/ghidra/README.md`). OpenHogs uses the same pictures with
motion of its own (`scene::ParticleSystem`, `render::EffectsRenderer`,
decision 160).

## Pictures

`Language/Tims/expltims.mad` (loaded by `FUN_004852c0` into texture set
`+0x3F0`). A particle's picture is an index into it:

| # | Entry | Use |
|---|-------|-----|
| 0 | `ptp0` | fireball, 64 × 64, on black (drawn added to the scene) |
| 1 | `ptp1` | puff of smoke, 64 × 64 |
| 2 | `ptp2` | water foam, 64 × 64 |
| 3 | `heart` | the heart of the hog label |
| 4–13 | `num0`–`num9` | damage digits (particle type `#`, picture = character − 0x2C) |
| 14 | `ptpa` | red arrow |
| 15 | `Shad_org` | shadow, a black disc 64 × 64 |
| 16 | `pcfx013` | scorch mark, dithered black 64 × 64 |
| 17 | `splodge` | small red specks 16 × 16 |
| 18–26 | `pcfx003`–`pcfx011` | a clod of earth turning, 9 frames of 16 × 16 |

## Particles

An emitter (class 0x135E, `FUN_00487ca0`) owns up to N particles of
40 bytes:

| Offset | Size | Content |
|--------|------|---------|
| 0 | 2 × 3 | position |
| 6 | 2 × 3 | velocity |
| 0x10 | 2 | age, 0–100 (100 = gone) |
| 0x12 | 2 | colour, 15-bit (0x4210 = grey) |
| 0x14 | 1 | base size |
| 0x15 | 1 | growth |
| 0x16 | 1 | picture (table above) |
| 0x17 | 1 | brightness |
| 0x18 | 2 | life |
| 0x1C, 0x1D | 1 each | movement parameters |
| 0x20 | 1 | particle kind (`#` for damage numbers) |

`FUN_00486bc0` sets a particle up from its kind (some 48 kinds: picture,
base size, growth, life, movement); `FUN_00489c40` draws them as
camera-facing sprites of size `((growth × age) / 10 + base × 10) × 6000`
in the renderer's units, their colour scaled by `(100 − age)` so they
fade.

The emitter's type (the third argument of `FUN_00487ad0`, about 95 of
them) decides how many particles it has and how they start. Types seen
in the code: 0x35 damage number, 0x3C a puff of smoke (kind 0x2D; two of
them beside a dead hog's boots), 0x56 a dead hog's explosion on land,
0x5B in water, 0x42 in lava, 3 and 0x26 when lava burns.

Explosions are recipes: types such as 0x56, 0x57, 0x58, 0x59, 0x5B call
`FUN_0048ccc0(recipe)`, and the emitter then plays row *recipe* of the
table at 0x4D61E8 (rows of 143 signed bytes, each multiplied by the byte
at the same column of 0x4D6C88): groups of columns start sub-emitters
(fireballs, smoke, debris) at given times with given counts, sizes and
colours. Recipe 7 is a dead hog's explosion, 8 an explosion in water.
The meaning of every column has not been worked out.

## Shadows

`FUN_0044eba0` lays the shadow picture (texture set `+0x3F0` + 15,
`Shad_org`) on the terrain's triangles under an
object, `size / 3 − height above the ground` wide, so it shrinks as the
object rises. OpenHogs draws a flat disc under each hog instead
(decision 160).

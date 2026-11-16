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
| 56 | `uint16` | health in points; 255 = the default of the object's kind (see "Destructible objects") |
| 58 | `uint8` | bits 0-3: in which games the object appears, by number of players (used when bit 5 is set); bit 6: scenery that cannot be destroyed |
| 59 | `uint8` | team bit mask (01, 02, 04... one bit per team) |
| 60 | `int16` | script type (mission objective role, e.g. 20 = tutorial target) |
| 62 | `uint8` | script group |
| 63 | 19 bytes | script parameters (crates: item, amount) |
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

### Script types and crates

The script type at offset 60 gives an object a role in the mission. The
map loader (`FUN_004a5250`) turns most of them into objectives (a list
built by `FUN_004a7130`: type, group, parameters 0 and 1, and the cell of
the script position). Counted on the PC maps:

| Script | Objects | Role |
|--------|---------|------|
| 1 | `S1W01PPP`, `PILLBOX`, `CO_ME`, `SW2ARM` | destroy: a crate |
| 2 | `LE_ME`, `BOG`, `BIG_GUN` | destroy: a promotion point |
| 4 | `W1R06PPP` | destroy: the player *loses* parameter 0 points |
| 7, 8 | hogs (`GR_ME`, `HV_ME`) | reach: a crate (7) or a promotion point (8) |
| 13 | `BRIDGE_C`, `TENT_S` | destroy the whole group: a crate |
| 14 | `M_TENT1`, `TANK`, `CARRY` | destroy the whole group: a promotion point |
| 19 | `CRATE1`, `CRATE2`, `PROPOINT`, `SIGN` | pickup; in the campaign a crate with a group is also an objective (type 21) |
| 20-23 | `DUMMY`, trees, bridges in `CAMP` | Boot Camp |

### Destructible objects

The executable's object table (pointers at 0x4D9680, 466 names) sorts
model names into kinds, between marker entries: buildings 1-6
(`BIG_GUN`, `M_TENT1`, `M_TENT2`, `PILLBOX`, `SHELTER`, `TENT_S`),
bunkers, gun barrels, pickups 20-25, scenery 28-379, animals, ammunition,
vehicles 445-451 (`AMLAUNCH`, `AM_TANK`, `BOAT`, `TANK`, `CARRY`,
`AIRSHIP1`, `PP1`) and hogs 454-463. The map loader (`FUN_004a5250`)
looks each object's name up there (`FUN_004a6d20`) and creates it with
the class of its kind (object + 0x38: 0x1359 building, 0x135A scenery,
0x1358 vehicle, 0x135B pickup, 0x1357 hog).

Every building, vehicle and scenery object has health at object + 0x4C,
in 1/128 points like a hog's:

| Kind | Table | Values (points) |
|------|-------|-----------------|
| buildings | 0x4C2E08, 8-byte rows | `BIG_GUN` 200, `M_TENT1` 30, `M_TENT2` 40, `PILLBOX` 100, `SHELTER` 100, `TENT_S` 25 |
| vehicles | 0x4D94A0, 12-byte rows | 150, `AIRSHIP1` and `PP1` 1 |
| scenery | 0x4D6D18, one int per entry | 60 for most (the village houses `S1F01PPP` ...), 1 targets and dummies, 10 dugouts, 50 bridges, bushes, small trees, flowers and grass, 80 trees and cacti, 100 walls, fences, gates, machines, 150 `TUN`, 200 `BOOTS` |

When offset 56 of the map object is not 255, it replaces the default
(`health = value << 7`, even 0). The damage method (virtual slot 13,
`0x43F6C0` for buildings, `0x48D990` for scenery) subtracts the damage,
shows the damage number, adds the damage to the attacking hog's total
(hog + 0x1D4) and, once less than one point (127/128) is left, sets the
health to 0 and marks the object destroyed. Scenery whose offset-58 bit 6
is set returns at once and takes no damage. The explosion code that calls
the method has not been read; OpenHogs damages objects as it damages hogs
(decision 114).


Each objective is a 20-byte node hung on its object (object + 0x48),
made by `FUN_004a7130`:

| Offset | Size | Content |
|--------|------|---------|
| 0 | 2 | type (the script type; 21 for campaign crates with a group) |
| 2 | 1 | group (offset 62) |
| 3 | 1 | parameter 0; for types 1, 7 and 13 already through the item table (255 kept) |
| 4 | 1 | parameter 1 |
| 8 | 4 | target cell X: `(script X + 0x4000) >> 9` |
| 12 | 4 | target cell Z, the same way |
| 16 | 4 | next node |

The script position (offsets 82-87) is a point on the ground: its Y is
the terrain height there, in the same axes as the object positions. The
original compares positions as cells of a 32 x 32 grid (512 units).

**Reaching** (`FUN_004a7730`, called as objects move): types 7 to 10 are
done when their own object stands in the target cell; for a hog it has
to be the player's current hog. The reward appears at the hog's feet:

- 7: a crate, health (parameter 0 = 255, sergeant section 5) or weapon
  (sergeant section 8);
- 8: a promotion point (sergeant section 9);
- 10: the hog's promotion points (hog + 0x1de) go down by parameter 0.

**Destroying** (`FUN_004aa170`, called when an object is destroyed or a
hog dies). The reward appears at the centre of the target cell
(`cell * 0x200 + 0xC100` as a 16-bit number), on the ground:

- 1: a crate, health (sergeant section 5) or weapon (section 4);
- 2: a promotion point (section 6);
- 4: promotion points go down by parameter 0 (section 7);
- 13 to 18: only when no other object with a node of the same group is
  left; then 13 acts as 1, 14 as 2 and 16 as 4 (15, 17, 18: nothing);
- 20, 21: the objects whose group is this node's parameter 0 appear
  (crates fall from 0xC00 above, others become visible); 22, 23: the
  same, once the last of the node's group is gone. Objects with script
  21 or 23 start hidden (Boot Camp).

A node of any other type (a hog with somewhere to go that dies) is simply
dropped. Promotion-point pickups (`FUN_00464150` kind 0x17) add 1 to the
hog's points whatever their parameters; objective crates are dropped
with a parachute (`WE_PARA` in `Chars/WEAPONS.MAD`). The sergeant line
is one of lines 1-8 of the section (`FUN_0043b850(section, 1, 8)`).

**Crates (script 19).** Parameter 0 (offset 63) says what the crate
holds and parameter 1 (offset 64) how many:

- 255: health, parameter 1 points of it (`CRATE2`: 10, 15, 20, 25, 50,
  100, 200);
- otherwise an item number, turned into a weapon number (text entry
  96 + it) by the table at 0x4D9DC8 (`0, 1, 2, ... 12, 14, 15 ...`: item
  13 is the flame thrower 14, item 30 the medicine ball 33, item 14 the
  rocket launcher 15);
- object type 23 gives item 0.

`PROPOINT` (promotion point) and `SIGN` objects carry crate parameters
too. Picking a crate up shows gtext 172, ">S FINDS USEFUL PROVISIONS!"
(171 when the hog cannot carry more), with the sound `I_PICKUP`.

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

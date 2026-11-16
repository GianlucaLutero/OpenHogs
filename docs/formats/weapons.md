# Weapons

Used for: weapon names, models and sounds. Rules: `src/game/Weapons`,
`Combat`, `Projectiles`, `World`.

Everything here was checked on the Steam release (v1.2b).

## Names: the game text table

`Language/Text/gtext` lists every item of the game in one block, in this
order (entry number: name):

| # | Name | # | Name | # | Name |
|---|------|---|------|---|------|
| 96 | NONE | 116 | CLUSTER GRENADE | 136 | MINE SHELL |
| 97 | TROTTER (the fist) | 117 | H-EXPL. GRENADE | 137 | GAS SHELL |
| 98 | KNIFE | 118 | ROLLER GRENADE | 138 | FIRE RAIN SHELL |
| 99 | BAYONET | 119 | CONFUSION GAS | 139 | 1000 LBS SHELL |
| 100 | SWORD | 120 | FREEZE GAS | 140 | SHOCK SHELL |
| 101 | CATTLE PROD | 121 | MADNESS GAS | 141 | JETPACK |
| 102 | PISTOL | 122 | POISON GAS | 142 | SUICIDE |
| 103 | RIFLE | 123 | SHRAPNEL GRENADE | 143 | HEALING HANDS |
| 104 | RIFLE BURST | 124 | MORTAR | 144 | SELF HEAL |
| 105 | MACHINE GUN | 125 | BAZOOKA | 145 | PICK POCKET |
| 106 | HEAVY M-GUN | 126 | GRENADE LAUNCHER | 146 | HIDE |
| 107 | SNIPER RIFLE | 127 | AIRBURST | 147 | SHOCKWAVE |
| 108 | SHOTGUN | 128 | SUPER AIRBURST | 148 | SPECIAL OPS |
| 109 | SUPER SHOTGUN | 129 | MEDICINE BALL | 149 | AIRSTRIKE |
| 110 | FLAME THROWER | 130 | HOMING MISSILE | 150 | F-RAIN AIRSTRIKE |
| 111 | ROCKET LAUNCHER | 131 | MINE | 151–153 | vehicles and buildings in/out, eject |
| 112 | GUIDED MISSILE | 132 | ANTI-P MINE | 154–157 | map view, binoculars, skip turn, surrender |
| 113 | MEDICINE DART | 133 | TNT | | |
| 114 | TRANQUILISER | 134 | SUPER TNT | | |
| 115 | GRENADE | 135 | LONG RANGE SHELL | | |

OpenHogs identifies weapons by these entry numbers (`game::WeaponId`), so
the names on screen come from the text table and a language pack
translates them. Entries 89–95 before the block are the vehicles.

Implemented so far: trotter, knife, bayonet, sword, cattle prod, pistol,
rifle, rifle burst, machine gun, heavy machine gun, sniper rifle, shotgun,
super shotgun, rocket launcher, grenade, cluster, high-explosive, roller
and shrapnel grenades, mortar, bazooka, grenade launcher, airburst, super
airburst, mine, anti-personnel mine, TNT, super TNT, suicide, healing
hands, self heal, skip turn, surrender, flame thrower, medicine dart,
tranquiliser, freeze, madness, poison and confusion gas, medicine ball,
pick pocket, hide, shockwave, guided and homing missiles, the six shells
(long range, mine, gas, fire rain, 1000 lbs, shock), jetpack, special
ops, airstrike and F-rain airstrike, vehicle and building in/out, map
view and binoculars. EJECT PIG is not (`docs/open-decisions.md`).
Buildings and vehicles bring their own weapons (`game::Mounts`).
Behaviour not found in the executable (lasting effects, healing amounts,
the ranks' number of uses) follows the Hogs of War Wiki.

## Models: `Chars/WEAPONS.MAD` and `.MTD`

29 models in the usual VTX/NO2/FAC format, textured from `WEAPONS.MTD`
(126 TIMs), in this order (the executable numbers them from 1):

```
1 WE_KNIFE  2 WE_SWORD  3 WE_PROD  4 WE_PIS  5 WE_AUTP  6 WE_RIF  7 WE_TELR
8 WE_MGUN  9 WE_LEWIS  10 WE_BLUND  11 WE_FLAME  12 WE_GREN  13 WE_HEG
14 WE_ROLL  15 WE_TNT  16 WE_SMOKE  17 bazookr  18 mortr  19 airbrst
20 WE_MINE  21 WE_SPADE  22 WE_RADIO  23 WE_TELE  24 WE_PARA  25 RPACK
26 WE_BANG  27 WE_BALL  28 grenlau  29 rocklau
```

### How a hog holds its weapon

The renderer (`Data/_d3d.dll`, `afDrawAnimModel`) draws a hog, then up to
three separate objects with the matrices of its bones: the weapon in hand
(hog field +0x40) with **bone 5**, a hat (+0x44) with bone 2 (the head)
and a pack (+0x48, only `RPACK`, the jetpack's rocket) with bone 1. They
are drawn as rigid objects (`afDrawObj`): the bone numbers stored in the
weapons' vertices (8, 7 or 0) are **not** used. Compared with the hogs'
body parts the objects face the other way: OpenHogs turns them half a
turn around the bone's Y axis (X and Z change sign,
`scene::buildCarriedModelMesh`). Without it every gun points backwards
and the rocket sits on the hog's chest.

### The weapon table (executable)

80-byte rows at 0x4D7300, one per text entry from 96 (row = entry − 96):

| Offset | Meaning |
|--------|---------|
| +0x0C | model in hand (1-based, above; 0 = none) |
| +0x14 | 1 for thrown or launched projectiles |
| +0x15 | 1 if the weapon has an aiming pose |
| +0x20 | animation when used (−1: none, guns fire from the aiming pose) |
| +0x50 | aiming animation (23 handgun, 24 rifle, 25 machine gun, 26 heavy) |
| +0x54 | "getting out" animation (12 handgun ... 18 sword/knife) |

| Weapon | Model | Out | Aim | Use |
|--------|-------|-----|-----|-----|
| TROTTER | – | 17 | – | 21 |
| KNIFE, SWORD | `WE_KNIFE`, `WE_SWORD` | 18 | – | 21 |
| BAYONET | `WE_RIF` | 14 | 25 | 22 |
| PISTOL | `WE_PIS` | 12 | 23 | – |
| RIFLE, RIFLE BURST | `WE_RIF` | 13 | 24 | – |
| MACHINE GUN, HEAVY M-GUN | `WE_MGUN`, `WE_LEWIS` | 14 | 25 | – |
| SNIPER RIFLE | `WE_TELR` | 13 | 24 | – |
| SHOTGUN, SUPER SHOTGUN | `WE_BLUND` | 13 | 24 | – |
| FLAME THROWER | `WE_FLAME` | 14 | 25 | – |
| ROCKET LAUNCHER, GUIDED and HOMING MISSILE | `rocklau` | 15 | 26 | – |
| grenades (all kinds) | `WE_GREN` (`WE_ROLL` roller, `WE_SMOKE` gases) | 17 | – | 19 |
| MORTAR, BAZOOKA, GRENADE LAUNCHER | `mortr`, `bazookr`, `grenlau` | 15 | 26 | – |
| AIRBURST | `airbrst` | 15 | 26 | – |
| MINE, ANTI-P MINE, TNT, SUPER TNT | `WE_MINE`, `WE_TNT`, `WE_BANG` | 17 | – | 77 |

Note: the table is 80 bytes a row but the first fields of a row belong
to the row before: a weapon's aiming and "getting out" animations are the
first two words of the *next* row (offsets +0x50 and +0x54 of its own).
The word at +0x10 is the weapon's ammunition: an entry of the object
table at 0x4D9680 (388 `WE_SUPAB` ... 442 `BULL1`), e.g. 400 for the
pistol, 412 for the grenade, 434 for the rocket launcher, 440 for super
TNT.

### The ammunition table (executable)

40-byte rows at 0x4C2030, one per ammunition object from 388 (row =
object − 388), read by the ammunition's constructor (`FUN_00431f00`) and
its explosion (`FUN_004327e0`):

| Offset | Meaning (as far as read) |
|--------|--------------------------|
| +0x00 | launch speed, scaled by the power (× power >> 12) |
| +0x04 | size of the explosion effect |
| +0x08 | explosion radius (bullets 0; grenades 2600, rockets 3900 to 5200, super TNT 6500) |
| +0x0C | damage in 1/128 points: pistol and rifle 20, burst 15, sniper 40, machine gun 4, heavy 8, shotgun 3 a pellet, grenade 30, high explosive 60, bazooka 40, mortar 50, rocket 50, TNT 50, super TNT 200, guided missile 75, 1000 lbs shell 100 |
| +0x10 | 1 bullet or direct hit, 2 explosive, 8 mine or charge |
| +0x18 | fuse (plus a random 0-7) |
| +0x1C | how it moves (0-4) |

OpenHogs takes the damage and (as 1/8 units) the radius of the weapons
added from this table (decision 121).

The **aiming animations** have 65 frames, one pose per angle: frame 0
points straight up, 32 level, 64 straight down. The executable keeps the
hog on the frame of its aim (`FUN_00469360`: frame = (0x400 − pitch) × 2,
in 1/64ths of a frame, with 0x400 = 90°). When the hog stands still the
weapon is taken out with its "getting out" animation (`FUN_00469090`,
"Started normal holstering") and it is put away while the hog moves.

## Sounds: `Audio/sfxday.srl`

| Sounds | Use in OpenHogs |
|--------|-----------------|
| `L_PISTOL`, `L_RIFLE`, `L_MGUN`, `L_HVYMG`, `L_SHOTG`, `L_BAZOO`, `L_ROC`, `L_MORT`, `L_ARTIL`, `L_FLAME`, `L_MINETR` | launch sound of each weapon |
| `P_PUNCH`, `I_STAB`, `I_SWORD`, `I_SWMISS` | melee hits (and a sword miss, not used yet) |
| `E_1`, `E_1000P` | explosion (`E_1000P` presumably the 1000 lbs shell) |
| `I_BULIT1`, `P_BOUNCE`, `I_SPLASH` | bullet impact, grenade bounce, splash |
| `P_OWW`, `P_MOAN`, `P_COWER1`–`3`, `P_DROWN` | hurt, dying (`P_MOAN` for now), cowering, drowning |

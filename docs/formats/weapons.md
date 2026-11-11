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

Implemented so far: trotter, knife, bayonet, sword, pistol, rifle, rifle
burst, machine gun, heavy machine gun, sniper rifle, shotgun, rocket
launcher, grenade, cluster grenade, high-explosive grenade, mortar,
bazooka, grenade launcher, mine, TNT. The others (gases, healing,
airstrikes, vehicles...) are listed in `docs/open-decisions.md`.

## Models: `Chars/WEAPONS.MAD` and `.MTD`

29 models (`WE_RIF`, `WE_PIS`, `WE_GREN`, `WE_TNT`, `bazookr`, `mortr`...)
in the usual VTX/NO2/FAC format, textured from `WEAPONS.MTD` (126 TIMs).

Like the hogs' bodies, every vertex names a bone of the hogs' skeleton and
is stored relative to it, which tells where the weapon is carried:

| Bone | Weapons | Meaning |
|------|---------|---------|
| 8 (right hand) | `WE_PIS`, `WE_AUTP` | held in the hand |
| 7 (right elbow) | `WE_RIF`, `WE_KNIFE`, part of `WE_TELR` | held along the forearm |
| 0 (pelvis) | grenades, TNT, mines, launchers, `RPACK`... | worn at the belt or on the back |

OpenHogs draws the selected weapon of the active hog this way, posed with
the hog. Whether the original moves belt and back items into the hands
when they are used is not known yet.

## Sounds: `Audio/sfxday.srl`

| Sounds | Use in OpenHogs |
|--------|-----------------|
| `L_PISTOL`, `L_RIFLE`, `L_MGUN`, `L_HVYMG`, `L_SHOTG`, `L_BAZOO`, `L_ROC`, `L_MORT`, `L_ARTIL`, `L_FLAME`, `L_MINETR` | launch sound of each weapon |
| `P_PUNCH`, `I_STAB`, `I_SWORD`, `I_SWMISS` | melee hits (and a sword miss, not used yet) |
| `E_1`, `E_1000P` | explosion (`E_1000P` presumably the 1000 lbs shell) |
| `I_BULIT1`, `P_BOUNCE`, `I_SPLASH` | bullet impact, grenade bounce, splash |
| `P_OWW`, `P_MOAN`, `P_COWER1`–`3`, `P_DROWN` | hurt, dying (`P_MOAN` for now), cowering, drowning |

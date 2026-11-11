# Audio files

Everything here was checked on the Steam release (v1.2b). The game's audio
is in four folders of the installation; no audio is inside the `.MAD`
archives.

| Folder | Contents | Format |
|--------|----------|--------|
| `Audio/` | in-game sound effects (101 files) and two sound banks | WAV, 16-bit mono, mostly 22,050 Hz |
| `FESounds/` | menu (front-end) sounds (64 files) and their sound bank | WAV, mono, mostly 16-bit 44,100 Hz |
| `Speech/Sku1/` | voices of the hogs, the sergeant and the training levels | WAV, 16-bit mono, 22,050 Hz |
| `MUSIC/` | music, `Track02.ogg` to `Track32.ogg` | Ogg Vorbis, stereo, 44,100 Hz |

The sample rates are not uniform. Counted over every file:

| Folder | Files per format |
|--------|------------------|
| `Audio/` | 77 × 22,050 Hz, 19 × 44,100 Hz, 3 × 11,025 Hz, 1 × 9,000 Hz, 1 × 5,000 Hz (all 16-bit) |
| `FESounds/` | 54 × 44,100 Hz, 2 × 22,050 Hz, 3 × 18,900 Hz, 1 × 37,800 Hz (16-bit); 1 × 44,100 Hz and 3 × 8,000 Hz (8-bit) |
| `Speech/` | all 22,050 Hz, 16-bit |

18,900 and 37,800 Hz are PlayStation (XA-ADPCM) rates: those menu sounds
were evidently converted from the console version without resampling. The
mixer converts every rate to the sound card's rate, so OpenHogs plays all
of them at the right pitch.

## WAV

Standard RIFF/WAVE files with an uncompressed PCM `fmt ` chunk and a
`data` chunk. `formats::parseWave` also accepts 8-bit samples, stereo and
extra chunks (`LIST`, `fact`...), so replacement sounds saved by any
audio editor work.

## Sound banks (`.srl`)

Text files with Windows line ends. The game refers to effects by number;
the bank turns a number into a file:

```
099              number of sounds
000              unknown, always 000
000              unknown, always 000
AUDIO\SFXDAY     bank name
0                sound number (0, 1, 2... in order)
AUDIO\AMB_1D.wav file of that sound
1
AUDIO\AMB_2D.wav
...
```

The game's banks write sound numbers without padding (`0`, `1`) and the
count with padding (`099`); the parser accepts both forms everywhere.
Paths are Windows paths whose letter case does not always match the real
files; OpenHogs resolves them through the case-insensitive virtual file
system.

| Bank | Sounds |
|------|--------|
| `Audio/sfxday.srl` | 99 in-game effects (ambience, explosions, footsteps on each terrain, weapons...) |
| `Audio/sfxnight.srl` | identical to `sfxday.srl` in the Steam release |
| `FESounds/Fesounds.srl` | 27 menu sounds |

## Speech

```
Speech/Sku1/Pig01 ... Pig09/   one folder per voice
Speech/Sku1/Sarge/             the sergeant (SGENxxxx.wav, 175 files)
Speech/Sku1/Train1/            training levels (tr1_en01.wav..., 28 files)
```

Hog files are named `PPNNGGVV.wav`:

| Part | Meaning |
|------|---------|
| `PP` | voice number, as in the folder name (`01`) |
| `NN` | accent of the nation: `EN`, `AM`, `FR`/`fr`, `GE`/`ge`, `RU`, `JA` (letter case varies between folders) |
| `GG` | group of lines (`01`–`05`): `01` ready, `02`–`03` battle cries, `04`–`05` cries of pain |
| `VV` | variant inside the group (`01`–`06`) |

Each voice has 30 files per nation (5 groups × 6 variants); Team Lard
(`TL` in the executable) has none. Sergeant files are `SGENssvv.wav`:
22 sections of 8 lines (the last has 7).

## Music

`MUSIC/TrackNN.ogg`. Numbering starts at 02 because the files are rips of
the original CD audio tracks, and track 1 of the CD was the data track.
`Track02.ogg` is the long title theme (2 min 51 s); most other tracks are
short jingles of 10–20 seconds. The executable plays CD tracks: its music
clip *c* is CD track *c* + 3, so `TrackNN.ogg` is clip NN − 3.

## When the game plays what

Read from `warhogs_.exe` with Ghidra; OpenHogs follows it in
`game::MatchSounds` and `gameapp::MatchAudio`.

### Hog voices (`FUN_0043af70`)

The path is built from the hog's voice (profile byte, +1), its nation
(index into the table `EN AM FR GE RU JA TL` at VA 0x4C2988), a group and
a variant. A variant above 6 continues in the next group; a group above 5
starts again at 1. Each hog keeps one counter per kind of line:

| Line | When | Group / variants |
|------|------|------------------|
| Ready | its turn starts (`FUN_00472320` case 7) and its team is played by a person | 0, variants 1–2 with more than 50 health, 3–4 with 20–50, 5–6 under 20; the counter alternates 0/1 |
| Battle cry | it readies a weapon (`FUN_00469360`, most weapons) | 1, variants 1–12 in turn (files `02` and `03`) |
| Pain | it lands after being blown off its feet (`FUN_0046cb50`) | 3, variants 1–12 in turn (files `04` and `05`) |

A hog of the computer says no "ready" line: `P_HMMM` plays instead
(volume 60, pitch 90–105 %). A new line is not started while another is
being said (`FUN_0043bb10`). Falling in water plays `P_DROWN`.

### The sergeant (`FUN_0043b850`, `FUN_00497f80`, `FUN_00498310`)

Section *s*, variant *v* → `SGEN<s+1><v>`; past 8 the variant continues
in the next section. After a turn, a turn is "good" if it hurt or killed
enemies and "bad" if it hurt or killed its own side:

| Sections | Use |
|----------|-----|
| 0 / 2 | one person against the computer: scorn / praise after the person's turn, 8 lines each in turn |
| 1, 3 | extra campaign comments (own squad large and losing / enemy nearly beaten), 1 in 4 |
| 4–9 | crates and pick-ups |
| 10, 11 | mission objects present (vehicles, guns...) |
| 12–13 | several people: a dawdler with 10 to 6 seconds left who has not attacked (turns over 15 s), 16 lines in turn |
| 14 + nation | several people: the team's nation (executable order); lines 1–4 praise, 5–8 scorn |
| 21 | front end |

Boot Camp has its own lines in `Train1` (`FUN_0043b530`).

### Music (`FUN_0043b4c0`)

| When | Track |
|------|-------|
| menus | `Track02` in a loop |
| match start | one of `Track27`–`Track30` at random (volume 70) |
| start of the computer's turn | its nation's tunes in turn: `Track03` + 4 × nation + 0..3 (British 3–6, American 7–10, French 11–14, German 15–18, Russian 19–22, Japanese 23–26) |
| a person won | `Track31` (volume 80) |
| the computer won, or a draw | `Track02`, once |

### Background (`FUN_00491b00`)

No continuous ambience. Each frame three counters advance: every 251
frames there is one chance in 16 of `AMB_1D` or `AMB_2D` (pitch 90–105 %);
otherwise every 101 frames one chance in 2 of `BATT_L1`–`3`, and every
126 frames one chance in 2 of `BATT_S1`–`3` (pitch 90–121 %). All play at
volume 25–40, placed 10,000 units away in a random direction.

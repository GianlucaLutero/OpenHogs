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
| `NN` | accent of the nation: `AM`, `EN`, `JA`, `RU`, `fr`, `ge` |
| `GG` | group of lines (`01`–`05`), presumably the situation (selected, hurt...) |
| `VV` | variant inside the group (`01`–`06`), picked so the line is not always the same |

Each voice has 30 files per nation (5 groups × 6 variants). What each group
is used for will be confirmed when the game logic is implemented.

## Music

`MUSIC/TrackNN.ogg`. Numbering starts at 02 because the files are rips of
the original CD audio tracks, and track 1 of the CD was the data track.
`Track02.ogg` is the long title theme (2 min 51 s); most other tracks are
short jingles of 10–20 seconds. Which track plays where will be mapped
when the menus and levels are implemented.

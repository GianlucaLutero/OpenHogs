# Content packs

This folder holds everything in OpenHogs that can be **replaced or switched
on and off** without touching the original game: language packs (text,
speech, briefing images), music, sound effects, textures and maps.

The original installation is never modified. At start-up OpenHogs builds a
layered view of the files:

```
top     ┌──────────────────────────┐   highest Priority wins
        │ pack "hd-music"          │
        │ pack "lang-it"           │
bottom  │ original game folder     │   used for every file no pack replaces
        └──────────────────────────┘
```

When the game needs a file it takes it from the top-most layer that has it.
A pack therefore only contains the files it changes.

## Layout of a pack

```
content/packs/<id>/
├── pack.ini      manifest (see below)
└── data/         same folder structure as the game installation
```

Example: an Italian language pack replacing one line of the sergeant and
the in-game text:

```
content/packs/lang-it/
├── pack.ini
└── data/
    ├── Language/Text/gtext.bin
    ├── Language/Text/gtext.ofs
    └── Speech/Sku1/Sarge/SGEN0101.wav
```

File names are not case-sensitive, exactly like in the original game.

## pack.ini

| Key         | Required | Meaning |
|-------------|----------|---------|
| Id          | yes      | Unique name: `a-z`, `0-9`, `-`, `_`. Used in `EnabledPacks`. |
| Name        | no       | Human-readable name. |
| Type        | no       | `language`, `audio`, `music`, `textures`, `maps` or `misc` (default). |
| Language    | for `language` packs | Language code, e.g. `it`. |
| Priority    | no       | Whole number, default 0. Higher wins when packs replace the same file. |
| Author, Version, Description | no | Shown to users. |

Start from [`packs/_template`](packs/_template/pack.ini). Folders whose name
starts with `_` are ignored, so the template is never loaded.

## Switching packs on and off

Packs are controlled from `config/openhogs.ini`:

```ini
[Content]
Language     = it                 ; activates every language pack with Language = it
EnabledPacks = hd-music, ps1-ai   ; activates these optional packs
```

- To switch an optional pack off, remove its Id from `EnabledPacks`.
- To go back to the original English text and speech, set `Language = en`.

## Copyright

Do not publish packs that contain files copied from the original game. A
language pack made from your own disc is for your own use; share the tools
or the instructions to build it instead.

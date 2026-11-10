# MAD / MTD archives

Used for: models (`.VTX`, `.NO2`, `.FAC`), textures (`.TIM`), map data
(`.MGL`) and some images (`.BMP`). Parser: `src/formats/PackageArchive`.

By convention `.MAD` archives hold models and `.MTD` archives hold
textures, but both use exactly the same layout.

## Layout

All numbers are little-endian.

```
offset 0: index   N entries of 24 bytes
          data    the files, one after another
```

Index entry:

| Offset | Type       | Meaning |
|--------|------------|---------|
| 0      | `char[16]` | File name, zero padded (e.g. `STF02PPP.VTX`) |
| 16     | `uint32`   | Offset of the file data from the start of the archive |
| 20     | `uint32`   | Size of the file data in bytes |

There is **no entry count**. The index ends where the data of the first
entry begins, so `N = offset_of_first_entry / 24`.

## Observations on the Steam/GOG data (v1.2b)

- 175 archives, 17,842 entries in total.
- About 1,560 entries share their name with another entry of the same
  archive. The extractor keeps all of them (`NAME~2.TIM`, ...).
- `Chars/mcap.mad` is **not** an archive although it has the extension: its
  first "offset" (10808) is not a multiple of 24. It most likely holds the
  motion-capture animation data of the hogs. `PackageArchive::looksLikePackage`
  detects it.

## Validation performed by the parser

- index size is non-zero, a multiple of 24 and inside the file;
- every name is printable ASCII;
- every entry's data lies after the index and inside the file.

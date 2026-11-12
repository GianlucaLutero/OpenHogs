// MglPicture.h
//
// The ".mgl" pictures of the game: the menu pictures of the PC version
// (FEBmps/FEBMP.MAD, 237 BMP files) and, on the PlayStation disc, the
// title screens, newspapers and cards (raw 15-bit pictures or TIMs). An
// .mgl file is the original file compressed with a simple byte-oriented
// LZ scheme; see docs/formats/mgl.md. The decompression follows the
// executable's routine (FUN_004989d0 in warhogs_.exe).

#pragma once

#include "openhogs/formats/TimImage.h"

#include <cstddef>
#include <span>
#include <vector>

namespace openhogs::formats {

// The original file inside an .mgl file. Throws core::FormatError if the
// data is damaged (a copy reaching before the start, a truncated command)
// or would grow beyond 16 MB.
std::vector<std::byte> decompressMgl(std::span<const std::byte> mgl);

// A PC menu picture (an .mgl holding a BMP) as RGBA. Pure magenta
// (255, 0, 255), the colour the front end draws as see-through, becomes
// transparent.
RgbaImage decodeMglPicture(std::span<const std::byte> mgl);

} // namespace openhogs::formats

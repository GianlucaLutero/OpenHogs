// BitmapImage.h
//
// Decoder for Windows .BMP pictures, as used by the PC version for its
// fonts (FEText/*.BMP) and menu pictures (Language/Tims/MENUTIMS.MAD:
// weapon icons and the like). See docs/formats/fonts.md.
//
// Only what the game uses is supported: uncompressed (BI_RGB) pictures
// with 4 or 8 bits per pixel and a palette, or 24 bits per pixel, stored
// bottom-up (the usual case) or top-down.

#pragma once

#include "openhogs/formats/TimImage.h"

#include <cstddef>
#include <span>

namespace openhogs::formats {

enum class BitmapTransparency {
    None,           // every pixel is opaque
    PaletteIndex0,  // palette entry 0 is transparent (the fonts' background)
};

// Decodes a .BMP file into top-row-first RGBA. Throws core::FormatError for
// files that are not BMP or use a variant the game never uses.
RgbaImage decodeBitmap(std::span<const std::byte> data,
                       BitmapTransparency transparency = BitmapTransparency::None);

} // namespace openhogs::formats

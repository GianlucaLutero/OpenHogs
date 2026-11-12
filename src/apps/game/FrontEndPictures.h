// FrontEndPictures.h
//
// The pictures of the original front end: the 237 .mgl pictures of
// FEbmps/FEBMP.MAD (the menu background "pigbkpc1", the brass panels,
// dials and cogs, title plates, nations and uniforms...), decoded on first
// use and kept as textures. Magenta pixels are transparent, as in the
// original. See docs/formats/mgl.md.
//
// Needs a current OpenGL context.

#pragma once

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/formats/PackageArchive.h"
#include "openhogs/render/UiRenderer.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace openhogs::gameapp {

class FrontEndPictures {
public:
    // A missing archive only causes a warning: picture() then finds nothing.
    explicit FrontEndPictures(const core::VirtualFileSystem& vfs);

    // The picture named `name` (without ".mgl", any letter case), or null
    // if there is none or it cannot be decoded.
    const render::UiImage* picture(std::string_view name);

private:
    std::optional<formats::PackageArchive> archive_;
    std::map<std::string, std::unique_ptr<render::UiImage>, std::less<>> loaded_;  // null: failed
};

} // namespace openhogs::gameapp

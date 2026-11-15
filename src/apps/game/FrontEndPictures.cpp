// FrontEndPictures.cpp - see FrontEndPictures.h for an overview.

#include "FrontEndPictures.h"

#include "openhogs/core/PathUtils.h"
#include "openhogs/formats/MglPicture.h"

#include <iostream>

namespace openhogs::gameapp {

namespace {
const char* const kArchive = "FEbmps/FEBMP.MAD";
}

FrontEndPictures::FrontEndPictures(const core::VirtualFileSystem& vfs) {
    try {
        archive_ = formats::PackageArchive::parse(vfs.readFile(kArchive));
    } catch (const std::exception& error) {
        std::cerr << "Warning: no front-end pictures (" << error.what() << ")\n";
    }
}

const render::UiImage* FrontEndPictures::picture(std::string_view name) {
    const std::string key = core::toLower(std::string(name));
    if (const auto found = loaded_.find(key); found != loaded_.end()) {
        return found->second.get();
    }
    std::unique_ptr<render::UiImage> image;
    if (archive_) {
        for (const auto& entry : archive_->entries()) {
            // Most entries end in ".mgl"; a few have another ending (e.g.
            // "info1.mgl1", next to an unrelated "info1.mgl"): those are
            // asked for by their full name.
            const std::string entryName = core::toLower(entry.name);
            if (entryName != key + ".mgl" && entryName != key) continue;
            try {
                image = std::make_unique<render::UiImage>(formats::decodeMglPicture(archive_->entryData(entry)));
            } catch (const std::exception& error) {
                std::cerr << "Warning: cannot decode " << entry.name << " (" << error.what() << ")\n";
            }
            break;
        }
    }
    const auto* result = image.get();
    loaded_.emplace(key, std::move(image));
    return result;
}

} // namespace openhogs::gameapp

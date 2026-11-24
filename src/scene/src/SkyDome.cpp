// SkyDome.cpp - see SkyDome.h for an overview.

#include "openhogs/scene/SkyDome.h"

#include "openhogs/formats/Model.h"
#include "openhogs/formats/PackageArchive.h"
#include "openhogs/scene/ModelMesh.h"

#include <stdexcept>
#include <string>

namespace openhogs::scene {

namespace {

constexpr const char* kDomeArchive = "chars/skydome.mad";
constexpr const char* kDomeHalves[] = {"skydome", "skydomeu"};

} // namespace

SkyDome loadSkyDome(const core::VirtualFileSystem& vfs, std::string_view skyArchive) {
    SkyDome dome;
    const auto pictures = formats::PackageArchive::parse(vfs.readFile(std::string(skyArchive)));
    std::vector<glm::uvec2> textureSizes;
    for (const auto& entry : pictures.entries()) {
        dome.textures.push_back(formats::decodeTim(pictures.entryData(entry)));
        textureSizes.emplace_back(dome.textures.back().width, dome.textures.back().height);
    }

    const auto models = formats::PackageArchive::parse(vfs.readFile(kDomeArchive));
    for (const std::string half : kDomeHalves) {
        const auto* vtx = models.find(half + ".vtx");
        const auto* fac = models.find(half + ".fac");
        if (vtx == nullptr || fac == nullptr) {
            throw std::runtime_error(std::string(kDomeArchive) + " has no model " + half);
        }
        const auto model = formats::Model::parse(models.entryData(*vtx), models.entryData(*fac));
        for (auto& part : buildModelMesh(model, textureSizes)) {
            dome.mesh.push_back(std::move(part));
        }
    }
    return dome;
}

} // namespace openhogs::scene

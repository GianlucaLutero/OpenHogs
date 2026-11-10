// MapScene.cpp - see MapScene.h for an overview.

#include "openhogs/scene/MapScene.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/PathUtils.h"
#include "openhogs/formats/MapTerrain.h"
#include "openhogs/formats/Model.h"
#include "openhogs/formats/PackageArchive.h"
#include "openhogs/formats/TextureTable.h"
#include "openhogs/scene/ModelMesh.h"
#include "openhogs/scene/TerrainMesh.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <exception>
#include <limits>
#include <optional>
#include <set>

namespace openhogs::scene {

namespace {

// Shown instead of a texture that cannot be decoded: a single magenta
// pixel, the usual "missing texture" colour, easy to spot on screen.
formats::RgbaImage makeMissingTexture() {
    return {1, 1, {255, 0, 255, 255}};
}

std::string toUpper(std::string_view text) {
    std::string result(text);
    for (char& c : result) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    }
    return result;
}

float angleToRadians(std::int16_t angle) {
    return static_cast<float>(angle) * glm::two_pi<float>() / formats::kAngleUnitsPerTurn;
}

formats::RgbaImage decodeOrPlaceholder(std::span<const std::byte> tim, const std::string& what,
                                       std::vector<std::string>& warnings) {
    try {
        return formats::decodeTim(tim);
    } catch (const std::exception& error) {
        warnings.push_back(what + ": " + error.what());
        return makeMissingTexture();
    }
}

void loadTerrain(const core::VirtualFileSystem& vfs, const std::string& base, MapScene& scene) {
    const auto terrain = formats::MapTerrain::parse(vfs.readFile(base + ".pmg"));
    scene.terrain = buildTerrainMesh(terrain);

    // Bounds of every vertex, for the camera.
    glm::vec3 low(std::numeric_limits<float>::max());
    glm::vec3 high(std::numeric_limits<float>::lowest());
    for (const auto& part : scene.terrain) {
        for (const auto& vertex : part.vertices) {
            low = glm::min(low, vertex.position);
            high = glm::max(high, vertex.position);
        }
    }
    scene.boundsMin = low;
    scene.boundsMax = high;

    if (!vfs.exists(base + ".ptg")) {
        scene.warnings.push_back(base + ".ptg missing: terrain drawn without textures");
        return;
    }
    const auto data = vfs.readFile(base + ".ptg");
    const auto images = formats::splitTextureTable(data);
    for (std::size_t i = 0; i < images.size(); ++i) {
        scene.terrainTextures.push_back(
            decodeOrPlaceholder(images[i], base + ".ptg texture " + std::to_string(i), scene.warnings));
    }
}

void loadModelTextures(const core::VirtualFileSystem& vfs, const std::string& base, MapScene& scene) {
    if (!vfs.exists(base + ".mtd")) {
        scene.warnings.push_back(base + ".mtd missing: objects drawn without textures");
        return;
    }
    // FAC texture indices count the archive entries in order, so every
    // entry is kept, even repeated names.
    const auto archive = formats::PackageArchive::parse(vfs.readFile(base + ".mtd"));
    for (const auto& entry : archive.entries()) {
        scene.modelTextures.push_back(
            decodeOrPlaceholder(archive.entryData(entry), base + ".mtd/" + entry.name, scene.warnings));
    }
}

void loadObjects(const core::VirtualFileSystem& vfs, const std::string& base, MapScene& scene) {
    if (!vfs.exists(base + ".pog") || !vfs.exists(base + ".mad")) {
        scene.warnings.push_back(base + ": no objects (.pog or .mad missing)");
        return;
    }

    const auto archive = formats::PackageArchive::parse(vfs.readFile(base + ".mad"));
    const auto objects = formats::parseMapObjects(vfs.readFile(base + ".pog"));

    std::vector<glm::uvec2> textureSizes;
    for (const auto& image : scene.modelTextures) {
        textureSizes.emplace_back(image.width, image.height);
    }

    std::set<std::string> failedModels;
    for (const auto& object : objects) {
        const std::string name = toUpper(object.modelName);
        const auto* vtx = archive.find(name + ".VTX");
        const auto* fac = archive.find(name + ".FAC");

        if (vtx == nullptr || fac == nullptr) {
            // No model: a logical object such as a hog start position.
            scene.markers.emplace_back(object.x, object.y, object.z);
            continue;
        }
        if (failedModels.contains(name)) {
            continue;
        }

        if (!scene.models.contains(name)) {
            try {
                const auto* no2 = archive.find(name + ".NO2");
                const auto normals = no2 ? std::optional(archive.entryData(*no2)) : std::nullopt;
                const auto model = formats::Model::parse(archive.entryData(*vtx), archive.entryData(*fac), normals);
                scene.models[name] = buildModelMesh(model, textureSizes);
            } catch (const std::exception& error) {
                scene.warnings.push_back(base + ".mad/" + name + ": " + error.what());
                failedModels.insert(name);
                continue;
            }
        }
        scene.objects.push_back({name, objectTransform(object)});
    }
}

} // namespace

glm::mat4 objectTransform(const formats::MapObject& object) {
    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, glm::vec3(object.x, object.y, object.z));
    // Yaw first (turning on the ground), then the tilts. In the game data
    // almost every object only uses the Y angle.
    transform = glm::rotate(transform, angleToRadians(object.angleY), glm::vec3(0, 1, 0));
    transform = glm::rotate(transform, angleToRadians(object.angleX), glm::vec3(1, 0, 0));
    transform = glm::rotate(transform, angleToRadians(object.angleZ), glm::vec3(0, 0, 1));
    // Models are stored at twice the world scale and with Y pointing down
    // (PlayStation convention), so Y is also mirrored here.
    transform = glm::scale(transform, glm::vec3(formats::kModelToWorldScale, -formats::kModelToWorldScale,
                                                formats::kModelToWorldScale));
    return transform;
}

MapScene loadMapScene(const core::VirtualFileSystem& vfs, std::string_view mapName) {
    MapScene scene;
    scene.name = toUpper(mapName);
    const std::string base = "maps/" + core::toLower(mapName);

    if (!vfs.exists(base + ".pmg")) {
        throw std::runtime_error("Map '" + scene.name + "' not found (no " + base + ".pmg)");
    }

    loadTerrain(vfs, base, scene);
    loadModelTextures(vfs, base, scene);
    loadObjects(vfs, base, scene);
    return scene;
}

std::vector<std::string> listMaps(const core::VirtualFileSystem& vfs) {
    std::set<std::string> names;
    for (const auto& key : vfs.listFiles("maps")) {
        if (key.ends_with(".pmg")) {
            const auto slash = key.rfind('/');
            names.insert(toUpper(key.substr(slash + 1, key.size() - slash - 1 - 4)));
        }
    }
    return {names.begin(), names.end()};
}

} // namespace openhogs::scene
